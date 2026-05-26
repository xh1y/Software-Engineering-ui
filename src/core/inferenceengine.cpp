#include "inferenceengine.h"

#include <QDebug>
#include "../utils/logger.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <vector>
#include <cmath>

namespace optikg {

InferenceEngine::InferenceEngine() = default;

InferenceEngine::~InferenceEngine() {
    cleanup();
}

void InferenceEngine::unload() {
    cleanup();
}

void InferenceEngine::cleanup() {
    session_.reset();
    tokenizer_.reset();
    env_.reset();
    modelLoaded_ = false;
}

bool InferenceEngine::loadModel(const QString& modelPath) {
    if (modelPath.isEmpty()) {
        Logger::warning("Model path is empty");
        return false;
    }

    if (modelLoaded_ && modelPath_ == modelPath) {
        // 已加载相同模型，直接返回成功
        return true;
    }

    // 如果已加载不同模型，先清理
    if (modelLoaded_) {
        cleanup();
    }

    modelPath_ = modelPath;
    qDebug() << "Loading model from:" << modelPath;

    QFileInfo modelFile(modelPath);
    if (!modelFile.exists()) {
        qCritical() << "Model file does not exist:" << modelPath;
        return false;
    }

    try {
        // 创建 ONNX Runtime 环境
        env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "OptiKG_InferenceEngine");

        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(0); // 使用默认线程数
        sessionOptions.SetInterOpNumThreads(1); // 对于单个会话，通常为1
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

#ifdef _WIN32
        std::wstring w_modelPath = modelPath.toStdWString();
        const ORTCHAR_T *ort_model_path = w_modelPath.c_str();
#else
        std::string s_modelPath = modelPath.toStdString();
        const ORTCHAR_T *ort_model_path = s_modelPath.c_str();
#endif

        session_ = std::make_unique<Ort::Session>(*env_, ort_model_path, sessionOptions);

        // 加载 tokenizer
        QString basePath = QFileInfo(modelPath).absolutePath();
        QString tokenizerPath = QDir(basePath).filePath("tokenizer.json");
        if (!loadTokenizer(tokenizerPath)) {
            qCritical() << "Failed to load tokenizer from:" << tokenizerPath;
            cleanup();
            return false;
        }

        // 加载 metadata
        QString metadataPath = QDir(basePath).filePath("metadata.json");
        MetadataLoadResult metaResult = loadMetadata(metadataPath);
        if (metaResult != MetadataLoadResult::Success) {
            // 根据错误类型决定如何处理
            switch (metaResult) {
                case MetadataLoadResult::FileNotFound:
                    Logger::warning(QString("Metadata file not found: %1. Using default values.").arg(metadataPath));
                    break;
                case MetadataLoadResult::InvalidJson:
                    Logger::warning(QString("Metadata file contains invalid JSON: %1. Using default values.").arg(metadataPath));
                    break;
                case MetadataLoadResult::MissingRequiredFields:
                    Logger::warning(QString("Metadata file missing required fields: %1. Using default values.").arg(metadataPath));
                    break;
                default:
                    Logger::warning(QString("Failed to load metadata from: %1. Using default values.").arg(metadataPath));
                    break;
            }
            // 记录使用的默认值
            Logger::info(QString("Using default metadata values - max_len: %1, threshold: %2")
                        .arg(maxSeqLength_).arg(threshold_));
        }

        size_t numInputNodes = session_->GetInputCount();
        size_t numOutputNodes = session_->GetOutputCount();
        qDebug() << "Model loaded. Inputs:" << numInputNodes << "Outputs:" << numOutputNodes;

        modelLoaded_ = true;
        return true;

    } catch (const Ort::Exception &e) {
        qCritical() << "ONNX Runtime error:" << e.what();
        cleanup();
        return false;
    } catch (const std::exception &e) {
        qCritical() << "Standard error:" << e.what();
        cleanup();
        return false;
    }
}

bool InferenceEngine::loadTokenizer(const QString& path) {
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

MetadataLoadResult InferenceEngine::loadMetadata(const QString& metadataPath) {
    QFile file(metadataPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::warning(QString("Cannot open metadata file: %1").arg(metadataPath));
        return MetadataLoadResult::FileNotFound;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull()) {
        Logger::warning("Invalid JSON in metadata file");
        return MetadataLoadResult::InvalidJson;
    }

    QJsonObject root = doc.object();
    
    // 检查必需的字段是否存在
    if (!root.contains("max_len") && !root.contains("threshold")) {
        Logger::warning("Metadata file missing required fields (max_len or threshold)");
        // 不返回错误，使用默认值继续
    }
    
    // 记录原始值，用于比较
    int oldMaxSeqLength = maxSeqLength_;
    float oldThreshold = threshold_;
    
    maxSeqLength_ = root.value("max_len").toInt(128);
    threshold_ = root.value("threshold").toDouble(-10.0f);
    
    // 验证加载的值是否合理
    if (maxSeqLength_ <= 0 || maxSeqLength_ > 2048) {
        Logger::warning(QString("Invalid max_len value in metadata: %1, using default").arg(maxSeqLength_));
        maxSeqLength_ = 128;
    }
    
    QJsonObject id2predicate = root.value("id2predicate").toObject();
    for (auto it = id2predicate.begin(); it != id2predicate.end(); ++it) {
        int id = it.key().toInt();
        QString predicate = it.value().toString();
        id2predicate_[id] = predicate;
    }

    Logger::info(QString("Metadata loaded - max_len: %1 (was %2), threshold: %3 (was %4), predicates: %5")
                .arg(maxSeqLength_).arg(oldMaxSeqLength)
                .arg(threshold_).arg(oldThreshold)
                .arg(id2predicate_.size()));
    return MetadataLoadResult::Success;
}

QList<Triple> InferenceEngine::infer(const QString& text) {
    Q_ASSERT_X(!text.isNull(), "InferenceEngine::infer", "text is null");
    
    if (!modelLoaded_) {
        Logger::warning("Model not loaded, cannot infer");
        return QList<Triple>();
    }

    qDebug() << "Running inference on text (length:" << text.length() << ")";
    auto results = runInference(text);
    qDebug() << "Inference complete, found" << results.size() << "triples";
    return results;
}

struct EntitySpan {
    int start;
    int end;
    int type;
    float confidence;
};

QList<Triple> InferenceEngine::runInference(const QString& rawInput) {
    Q_ASSERT_X(tokenizer_ != nullptr, "InferenceEngine::runInference", "tokenizer is null");
    Q_ASSERT_X(session_ != nullptr, "InferenceEngine::runInference", "session is null");
    
    qDebug() << "Running inference...";

    QString text = rawInput.toLower().trimmed();
    if (text.isEmpty()) {
        return QList<Triple>();
    }

    // 尝试解析 JSON 输入
    QJsonDocument doc = QJsonDocument::fromJson(rawInput.toUtf8());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        // 验证 text 字段是否存在
        if (!obj.contains("text")) {
            Logger::warning("JSON input missing 'text' field");
            // 尝试其他可能的字段名
            const QStringList possibleFields = {"content", "input", "sentence", "data"};
            for (const QString& field : possibleFields) {
                if (obj.contains(field)) {
                    text = obj.value(field).toString().toLower().trimmed();
                    Logger::info(QString("Using alternative field '%1' for text input").arg(field));
                    break;
                }
            }
        } else {
            text = obj.value("text").toString().toLower().trimmed();
        }
    }

    if (text.isEmpty()) {
        return QList<Triple>();
    }

    const int TARGET_LEN = maxSeqLength_;
    Q_ASSERT_X(TARGET_LEN > 0 && TARGET_LEN <= 2048, "InferenceEngine::runInference", 
               "Invalid maxSeqLength_");

    // 1. 编码（带智能截断）
    std::vector<int32_t> encoded = tokenizer_->Encode(text.toStdString());
    if (encoded.size() > static_cast<size_t>(TARGET_LEN - 2)) {
        // 记录截断前的信息
        size_t originalSize = encoded.size();
        Logger::warning(QString("Input text too long (%1 tokens), truncating to %2 tokens. "
                               "Consider using inferLongText() for long inputs.")
                       .arg(originalSize).arg(TARGET_LEN - 2));
        
        // 智能截断：尝试在句子边界处截断
        size_t truncatePos = TARGET_LEN - 2;
        // 向后查找句号、问号、感叹号等句子边界
        for (size_t i = truncatePos; i > truncatePos / 2; --i) {
            if (i < encoded.size()) {
                std::vector<int32_t> tokenIds = {encoded[i]};
                QString token = QString::fromStdString(tokenizer_->Decode(tokenIds)).trimmed();
                // 检查是否是句子结束标记
                if (token.contains(QRegularExpression("[。！？；.!?;]"))) {
                    truncatePos = i + 1;  // 包含标点符号
                    Logger::info(QString("Truncated at sentence boundary, keeping %1 tokens").arg(truncatePos));
                    break;
                }
            }
        }
        encoded.resize(truncatePos);
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
    int actualLen = qMin(static_cast<int>(inputIdsData.size()), TARGET_LEN);
    while (inputIdsData.size() < static_cast<size_t>(TARGET_LEN)) {
        inputIdsData.push_back(static_cast<int64_t>(padId));
    }

    // Attention Mask
    std::vector<float> attentionMaskData(TARGET_LEN, 0.0f);
    for (int i = 0; i < actualLen; ++i) {
        attentionMaskData[i] = 1.0f;
    }

    // 2. 构建 Tensor 并运行推理
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

    auto outputTensors = session_->Run(
        Ort::RunOptions{nullptr}, 
        inputNames, 
        inputTensors.data(), 
        2, 
        outputNames, 
        3);

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
            if (sLogit > threshold_) {
                subjects.push_back({i, j, 0, 1.0f / (1.0f + std::exp(-sLogit))});
            }
            if (oLogit > threshold_) {
                objects.push_back({i, j, 1, 1.0f / (1.0f + std::exp(-oLogit))});
            }
        }
    }

    // 5. 提取关系并组装 Triple
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

                    std::vector<int32_t> subIds(
                        inputIdsData.begin() + sub.start, 
                        inputIdsData.begin() + sub.end + 1);
                    std::vector<int32_t> objIds(
                        inputIdsData.begin() + obj.start, 
                        inputIdsData.begin() + obj.end + 1);
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
                    EntityType oType = (rel == 2) ? EntityType::Tool : 
                                       (rel == 3 ? EntityType::Composition : EntityType::Fault);
                    Entity oEnt(objText, oType, obj.confidence);
                    results.append(Triple(sEnt, oEnt, id2predicate_.value(rel, "相关"), conf));
                }
            }
        }
    }

    qDebug() << "Inference complete. Found" << results.size() << "triples";
    return results;
}

QList<Triple> InferenceEngine::inferLongText(const QString& text, int chunkSize, int overlapSize) {
    if (text.isEmpty()) {
        return QList<Triple>();
    }

    // 如果文本长度小于块大小，直接使用普通推理
    if (text.length() <= chunkSize) {
        return infer(text);
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
        QList<Triple> chunkResults = infer(chunk);
        qDebug() << "Chunk [" << startPos << "-" << endPos << "], length:" << chunk.length()
                 << "found" << chunkResults.size() << "triples";

        // 转换实体位置为全局位置（需要修改Entity的startPos/endPos）
        for (auto& triple : chunkResults) {
            // 暂时不调整位置，因为当前Entity没有位置信息
            // 未来可以添加位置映射
        }

        allResults.append(chunkResults);

        // 如果已经处理到文本末尾，退出循环
        if (endPos >= text.length()) {
            break;
        }

        // 移动到下一个块，考虑重叠
        startPos = endPos - overlapSize;
        if (startPos <= 0 || startPos >= endPos) {
            startPos = endPos; // 防止无限循环或回退
        }

        // 进度信息
        qDebug() << "Progress:" << startPos << "/" << text.length();
    }

    // 智能去重（基于实体名称规范化、关系语义和置信度）
    QList<Triple> deduplicated;
    
    // 辅助函数：规范化实体名称用于比较
    auto normalizeEntity = [](const QString& name) -> QString {
        QString normalized = name.toLower().trimmed();
        // 移除常见的多余空格和标点
        normalized = normalized.replace(QRegularExpression("\\s+"), " ");
        normalized = normalized.replace(QRegularExpression("^[\\s.。,，!！?？]+"), "");
        normalized = normalized.replace(QRegularExpression("[\\s.。,，!！?？]+$"), "");
        return normalized;
    };
    
    // 辅助函数：计算字符串相似度（简单的编辑距离比率）
    auto calculateSimilarity = [](const QString& s1, const QString& s2) -> qreal {
        if (s1 == s2) return 1.0;
        if (s1.isEmpty() || s2.isEmpty()) return 0.0;
        
        // 使用最长公共子序列的简化版本
        int maxLen = qMax(s1.length(), s2.length());
        int commonChars = 0;
        for (const QChar& c : s1) {
            if (s2.contains(c)) {
                commonChars++;
            }
        }
        return static_cast<qreal>(commonChars) / maxLen;
    };
    
    // 使用多维度键来去重
    QMap<QString, Triple> uniqueTriples;  // 键 -> 最佳三元组
    const qreal SIMILARITY_THRESHOLD = 0.85;  // 相似度阈值
    
    for (const auto& triple : allResults) {
        QString normSubject = normalizeEntity(triple.subject.name);
        QString normObject = normalizeEntity(triple.object.name);
        QString normRelation = triple.relation.trimmed().toLower();
        
        // 创建规范化键
        QString normKey = normSubject + "|" + normRelation + "|" + normObject;
        
        bool foundDuplicate = false;
        // 检查是否已存在相似的三元组
        for (auto it = uniqueTriples.begin(); it != uniqueTriples.end(); ++it) {
            const Triple& existing = it.value();
            QString existingNormSub = normalizeEntity(existing.subject.name);
            QString existingNormObj = normalizeEntity(existing.object.name);
            QString existingNormRel = existing.relation.trimmed().toLower();
            
            // 计算相似度
            qreal subSim = calculateSimilarity(normSubject, existingNormSub);
            qreal objSim = calculateSimilarity(normObject, existingNormObj);
            qreal relSim = (normRelation == existingNormRel) ? 1.0 : 0.0;
            
            // 如果实体和关系都足够相似，认为是重复
            if (subSim >= SIMILARITY_THRESHOLD && objSim >= SIMILARITY_THRESHOLD && relSim > 0) {
                foundDuplicate = true;
                // 保留置信度更高的
                if (triple.confidence > existing.confidence) {
                    uniqueTriples[it.key()] = triple;
                    Logger::debug(QString("Replaced duplicate triple with higher confidence: %1 -> %2 -> %3")
                                 .arg(triple.subject.name).arg(triple.relation).arg(triple.object.name));
                }
                break;
            }
        }
        
        if (!foundDuplicate) {
            uniqueTriples.insert(normKey, triple);
        }
    }
    
    deduplicated = uniqueTriples.values();

    Logger::info(QString("Long text processing complete. Found %1 unique triples (before dedup: %2, removed: %3)")
                .arg(deduplicated.size())
                .arg(allResults.size())
                .arg(allResults.size() - deduplicated.size()));

    return deduplicated;
}

} // namespace optikg
