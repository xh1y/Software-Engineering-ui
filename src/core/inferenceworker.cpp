#include "inferenceworker.h"
#include "../utils/appconstants.h"

#include <QDebug>
#include <QRegularExpression>
#include <QSet>
#include "../utils/logger.h"
#include <QThread>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QChar>
#include <QMap>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

namespace optikg {

    InferenceWorker::InferenceWorker(QObject *parent)
        : QThread(parent)
          , threshold_(-10.0f)
          , stopRequested_(false)
          , env_(ORT_LOGGING_LEVEL_WARNING, "OptiKG") {
    }

    InferenceWorker::~InferenceWorker() {
        stop();
        quit();
        wait();
        cleanup();
    }

    void InferenceWorker::setText(const QString& text) {
        text_ = text;
    }

    void InferenceWorker::setModelPath(const QString& path) {
        qDebug() << "Setting model path to:" << path;
        modelPath_ = path;
    }

    void InferenceWorker::setThreshold(float threshold) {
        threshold_ = threshold;
    }

    void InferenceWorker::stop() {
        stopRequested_ = true;
    }

    void InferenceWorker::run() {
        QElapsedTimer timer;
        timer.start();

        emit progressChanged(10);

        QList<Triple> results;
        try {
            // 如果文本过长，使用分块处理
            if (text_.length() > AppConstants::Model::CHUNK_THRESHOLD) {
                results = inferWithChunking(text_, AppConstants::Model::DEFAULT_CHUNK_SIZE, AppConstants::Model::DEFAULT_OVERLAP_SIZE);
            } else {
                results = runOnnxInference(text_);
            }
        } catch (const std::exception &e) {
            emit errorOccurred(tr("推理错误: %1").arg(e.what()));
            return;
        } catch (...) {
            emit errorOccurred(tr("未知推理错误"));
            return;
        }

        emit progressChanged(90);

        QList<Triple> filteredResults;
        for (const auto &triple: results) {
            if (triple.confidence >= threshold_) {
                filteredResults.append(triple);
            }
        }

        emit progressChanged(100);

        int elapsedMs = timer.elapsed();
        qDebug() << "Inference completed in" << elapsedMs << "ms, found" << filteredResults.size() << "triples";

        emit resultReady(filteredResults);
    }

    bool InferenceWorker::loadModel() {
        if (modelPath_.isEmpty()) {
            Logger::warning("Model path is empty");
            return false;
        }

        qDebug() << "Attempting to load model from:" << modelPath_;

        QFileInfo modelFile(modelPath_);
        if (!modelFile.exists()) {
            qCritical() << "Model file does not exist:" << modelPath_;
            return false;
        }

        try {
            Ort::SessionOptions sessionOptions;
            sessionOptions.SetIntraOpNumThreads(1);
            sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

            qDebug() << "Initializing ONNX Session directly from file path...";

#ifdef _WIN32
            std::wstring w_modelPath = modelPath_.toStdWString();
            const ORTCHAR_T *ort_model_path = w_modelPath.c_str();
#else
            std::string s_modelPath = modelPath_.toStdString();
            const ORTCHAR_T *ort_model_path = s_modelPath.c_str();
#endif

            session_ = std::make_unique<Ort::Session>(env_, ort_model_path, sessionOptions);

            QString basePath = QFileInfo(modelPath_).absolutePath();
            QString tokenizerPath = QDir(basePath).filePath("tokenizer.json");
            if (!loadTokenizer(tokenizerPath)) {
                qCritical() << "Failed to load tokenizer from:" << tokenizerPath;
                return false;
            }

            QString metadataPath = QDir(basePath).filePath("metadata.json");
            if (!loadMetadata(metadataPath)) {
                Logger::warning(QString("Failed to load metadata from: %1").arg(metadataPath));
            }

            size_t numInputNodes = session_->GetInputCount();
            size_t numOutputNodes = session_->GetOutputCount();
            qDebug() << "Model inputs:" << numInputNodes << "outputs:" << numOutputNodes;

            for (size_t i = 0; i < numInputNodes; i++) {
                auto inputName = session_->GetInputNameAllocated(i, allocator_);
                auto typeInfo = session_->GetInputTypeInfo(i);
                auto shapeInfo = typeInfo.GetTensorTypeAndShapeInfo();
                auto shape = shapeInfo.GetShape();
                Q_UNUSED(shape)
                qDebug() << "Input" << i << "name:" << inputName.get();
            }

            modelLoaded_ = true;
            qDebug() << "Model loaded successfully!";
            return true;
        } catch (const Ort::Exception &e) {
            qCritical() << "Failed to load ONNX model (ORT Error):" << e.what();
            return false;
        } catch (const std::exception &e) {
            qCritical() << "Failed to load model (System Error):" << e.what();
            return false;
        }
    }

    bool InferenceWorker::loadTokenizer(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            Logger::warning(QString("Cannot open tokenizer file: %1").arg(path));
            return false;
        }
        QByteArray jsonBlob = file.readAll();
        file.close();
        try {
            tokenizer_ = tokenizers::Tokenizer::FromBlobJSON(jsonBlob.toStdString());
            qDebug() << "Tokenizer loaded, vocab size:" << tokenizer_->GetVocabSize();
            return true;
        } catch (const std::exception& e) {
            qCritical() << "Failed to load tokenizer:" << e.what();
            return false;
        }
    }

    bool InferenceWorker::loadMetadata(const QString& metadataPath) {
        QFile file(metadataPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            Logger::warning(QString("Cannot open metadata file: %1").arg(metadataPath));
            return false;
        }

        QByteArray jsonData = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isNull()) {
            Logger::warning("Invalid JSON in metadata file");
            return false;
        }

        QJsonObject root = doc.object();
        maxSeqLength_ = root.value("max_len").toInt(128);
        threshold_ = root.value("threshold").toDouble(-10.0f);

        QJsonObject id2predicate = root.value("id2predicate").toObject();
        for (auto it = id2predicate.begin(); it != id2predicate.end(); ++it) {
            int id = it.key().toInt();
            QString predicate = it.value().toString();
            id2predicate_[id] = predicate;
        }

        qDebug() << "Metadata loaded, max_len:" << maxSeqLength_ << ", threshold:" << threshold_;
        return true;
    }

    void InferenceWorker::cleanup() {
        session_.reset();
        tokenizer_.reset();
        modelLoaded_ = false;
    }

    QList<Triple> InferenceWorker::runOnnxInference(const QString& text) {
        if (!modelLoaded_) {
            if (!loadModel()) {
                throw std::runtime_error("Failed to load model");
            }
        }
        return tokenizeAndPredict(text);
    }

    struct EntitySpan {
        int start;
        int end;
        int type;
        float confidence;
    };

    QList<Triple> InferenceWorker::tokenizeAndPredict(const QString &rawInput) {
        qDebug() << "=== Starting Final Standardized Prediction ===";
        if (!modelLoaded_ && !loadModel()) return QList<Triple>();

        QString text = rawInput.toLower().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(rawInput.toUtf8());
        if (doc.isObject()) text = doc.object().value("text").toString().toLower().trimmed();
        if (text.isEmpty()) return QList<Triple>();

        const int TARGET_LEN = maxSeqLength_;

        // 1. 使用 tokenizers-cpp 进行编码
        std::vector<int32_t> encoded = tokenizer_->Encode(text.toStdString());
        if (encoded.size() > static_cast<size_t>(TARGET_LEN - 2)) {
            encoded.resize(TARGET_LEN - 2);
        }

        int32_t clsId = tokenizer_->TokenToId("[CLS]");
        int32_t sepId = tokenizer_->TokenToId("[SEP]");
        int32_t padId = tokenizer_->TokenToId("[PAD]");

        std::vector<int64_t> inputIdsData;
        inputIdsData.reserve(TARGET_LEN);
        inputIdsData.push_back(static_cast<int64_t>(clsId));
        for (int32_t id : encoded) {
            inputIdsData.push_back(static_cast<int64_t>(id));
        }
        inputIdsData.push_back(static_cast<int64_t>(sepId));
        int actualLen = static_cast<int>(inputIdsData.size());
        while (inputIdsData.size() < static_cast<size_t>(TARGET_LEN)) {
            inputIdsData.push_back(static_cast<int64_t>(padId));
        }

        // 准备 Attention Mask
        std::vector<float> attentionMaskData(TARGET_LEN, 0.0f);
        for (int i = 0; i < actualLen; ++i) {
            attentionMaskData[i] = 1.0f;
        }

        // 调试：打印前10个 token
        QString decodedStr;
        for (int i = 0; i < qMin(10, actualLen); ++i) {
            decodedStr += QString::fromStdString(tokenizer_->IdToToken(static_cast<int32_t>(inputIdsData[i]))) + " ";
        }
        qDebug() << "First 10 decoded tokens:" << decodedStr;
        qDebug() << "Actual sequence length:" << actualLen;

        // 2. 构建 Tensor 并运行 ONNX 推理
        auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> inputShape = {1, TARGET_LEN};

        Ort::Value inputIdsTensor = Ort::Value::CreateTensor<int64_t>(
            memoryInfo, inputIdsData.data(), inputIdsData.size(), inputShape.data(), inputShape.size());
        Ort::Value attentionMaskTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, attentionMaskData.data(), attentionMaskData.size(), inputShape.data(), inputShape.size());

        auto inName0 = session_->GetInputNameAllocated(0, allocator_);
        auto inName1 = session_->GetInputNameAllocated(1, allocator_);
        const char* inputNames[] = { inName0.get(), inName1.get() };
        auto outName0 = session_->GetOutputNameAllocated(0, allocator_);
        auto outName1 = session_->GetOutputNameAllocated(1, allocator_);
        auto outName2 = session_->GetOutputNameAllocated(2, allocator_);
        const char* outputNames[] = { outName0.get(), outName1.get(), outName2.get() };

        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(std::move(inputIdsTensor));
        inputTensors.push_back(std::move(attentionMaskTensor));

        auto outputTensors = session_->Run(Ort::RunOptions{nullptr}, inputNames, inputTensors.data(), 2, outputNames, 3);

        // 3. 解析输出
        auto outShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
        const int STRIDE = static_cast<int>(outShape[3]);

        float* entLogits = outputTensors[0].GetTensorMutableData<float>();
        float* relHLogits = outputTensors[1].GetTensorMutableData<float>();
        float* relTLogits = outputTensors[2].GetTensorMutableData<float>();

        auto getVal = [&](float* data, int head, int i, int j) -> float {
            return data[head * STRIDE * STRIDE + i * STRIDE + j];
        };

        // 4. 提取实体
        std::vector<EntitySpan> subjects, objects;
        for (int i = 0; i < actualLen; i++) {
            for (int j = i; j < actualLen; j++) {
                float sLogit = getVal(entLogits, 0, i, j);
                float oLogit = getVal(entLogits, 1, i, j);
                if (sLogit > threshold_) subjects.push_back({i, j, 0, 1.0f / (1.0f + std::exp(-sLogit))});
                if (oLogit > threshold_) objects.push_back({i, j, 1, 1.0f / (1.0f + std::exp(-oLogit))});
            }
        }

        // 5. 提取关系并组装 Triple（使用 tokenizer Decode 提取实体文本）
        QList<Triple> results;
        for (int rel = 0; rel < 4; rel++) {
            for (const auto& sub : subjects) {
                for (const auto& obj : objects) {
                    float hS = getVal(relHLogits, rel, sub.start, obj.start);
                    float tS = getVal(relTLogits, rel, sub.end, obj.end);
                    if (hS > threshold_ && tS > threshold_) {
                        float conf = sub.confidence * obj.confidence *
                                     (1.0f / (1.0f + std::exp(-hS))) *
                                     (1.0f / (1.0f + std::exp(-tS)));

                        std::vector<int32_t> subIds(inputIdsData.begin() + sub.start, inputIdsData.begin() + sub.end + 1);
                        std::vector<int32_t> objIds(inputIdsData.begin() + obj.start, inputIdsData.begin() + obj.end + 1);
                        QString subText = QString::fromStdString(tokenizer_->Decode(subIds)).trimmed();
                        QString objText = QString::fromStdString(tokenizer_->Decode(objIds)).trimmed();

                        // 后处理：只去掉中文字符之间的 tokenization 空格，保留英文单词间的空格
                        auto cleanTokenSpaces = [](const QString& text) -> QString {
                            QString result;
                            for (int i = 0; i < text.length(); ++i) {
                                if (text[i] == ' ' && i > 0 && i < text.length() - 1) {
                                    QChar prev = text[i - 1];
                                    QChar next = text[i + 1];
                                    // 判断前后是否都是中文字符（CJK Unified Ideographs）
                                    bool prevIsCJK = prev.unicode() >= 0x4E00 && prev.unicode() <= 0x9FFF;
                                    bool nextIsCJK = next.unicode() >= 0x4E00 && next.unicode() <= 0x9FFF;
                                    if (prevIsCJK && nextIsCJK) {
                                        continue; // 跳过这个空格
                                    }
                                }
                                result.append(text[i]);
                            }
                            return result;
                        };

                        subText = cleanTokenSpaces(subText);
                        objText = cleanTokenSpaces(objText);

                        if (subText.isEmpty() || objText.isEmpty()) continue;

                        Entity sEnt(subText, EntityType::Component, sub.confidence);
                        EntityType oType = (rel == 2) ? EntityType::Tool : (rel == 3 ? EntityType::Composition : EntityType::Fault);
                        Entity oEnt(objText, oType, obj.confidence);
                        results.append(Triple(sEnt, oEnt, id2predicate_.value(rel, "相关"), conf));
                    }
                }
            }
        }

        qDebug() << "Success! Found Triples:" << results.size();
        return results;
    }

    QList<Triple> InferenceWorker::inferWithChunking(const QString& text, int chunkSize, int overlapSize) {
        if (text.isEmpty()) {
            return QList<Triple>();
        }

        // 如果文本长度小于块大小，直接使用普通推理
        if (text.length() <= chunkSize) {
            return runOnnxInference(text);
        }

        qDebug() << "Processing long text (length:" << text.length() << ") with chunkSize:" << chunkSize << "overlapSize:" << overlapSize;

        QList<Triple> allResults;

        // 分割文本为块（尽量按句子边界分割）
        int startPos = 0;
        while (startPos < text.length()) {
            int endPos = qMin(startPos + chunkSize, text.length());

            // 如果不在文本末尾，尝试在句子边界处分割
            if (endPos < text.length()) {
                // 查找最近的句子结束符（中文标点）
                int sentenceEnd = text.lastIndexOf(QRegularExpression("[。！？；]"), endPos - 1);
                if (sentenceEnd > startPos + chunkSize * 0.5) { // 确保块不会太小
                    endPos = sentenceEnd + 1; // 包含标点
                }
            }

            QString chunk = text.mid(startPos, endPos - startPos);

            // 运行推理
            QList<Triple> chunkResults = runOnnxInference(chunk);
            qDebug() << "Chunk [" << startPos << "-" << endPos << "], length:" << chunk.length()
                     << "found" << chunkResults.size() << "triples";

            // 转换实体位置为全局位置（需要修改Entity的startPos/endPos）
            for (auto& triple : chunkResults) {
                // 暂时不调整位置，因为当前Entity没有位置信息
                // 未来可以添加位置映射
            }

            allResults.append(chunkResults);

            // 移动到下一个块，考虑重叠
            startPos = endPos - overlapSize;
            if (startPos <= 0) {
                startPos = endPos; // 防止无限循环
            }

            // 进度信息（可以发射信号，但这是私有方法）
            qDebug() << "Progress:" << startPos << "/" << text.length();
        }

        // 简单去重（基于实体名称和关系）
        QList<Triple> deduplicated;
        QSet<QString> seen;

        for (const auto& triple : allResults) {
            QString key = triple.subject.name + "|" + triple.relation + "|" + triple.object.name;
            if (!seen.contains(key)) {
                seen.insert(key);
                deduplicated.append(triple);
            }
        }

        qDebug() << "Long text processing complete. Found" << deduplicated.size()
                 << "unique triples (before dedup:" << allResults.size() << ")";

        return deduplicated;
    }

    QList<Triple> InferenceWorker::simulateInference(const QString &text) {
        Q_UNUSED(text)
        return {};
    }
} // namespace optikg
