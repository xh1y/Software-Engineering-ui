#include <QtCore>

#include "batchprocessor.h"
#include "inferenceengine.h"
#include "../utils/configmanager.h"

#include <QDebug>
#include "../utils/logger.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QTextStream>
#include <QStringConverter>
#include <QString>
#include <QStringList>
#include <QChar>
#include <QtConcurrent>
#include <QFuture>
#include <QThread>
#include <QThreadStorage>
#include <QAtomicInt>

namespace optikg {

// 线程本地存储推理引擎实例（每个线程一个，避免重复加载模型）
static QThreadStorage<InferenceEngine*>& getEngineStorage() {
    static QThreadStorage<InferenceEngine*> storage;
    return storage;
}

/**
 * @brief 解析 CSV 行，正确处理引号和转义
 * @param line CSV 行字符串
 * @return 字段列表
 */
static QStringList parseCsvLine(const QString& line) {
    QStringList fields;
    QString currentField;
    bool inQuotes = false;
    bool wasQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar ch = line[i];

        if (ch == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                // 双引号转义
                currentField += '"';
                i++; // 跳过下一个引号
            } else {
                inQuotes = !inQuotes;
                wasQuotes = true;
            }
        } else if (ch == ',' && !inQuotes) {
            // 字段分隔符
            fields.append(currentField);
            currentField.clear();
            wasQuotes = false;
        } else {
            currentField += ch;
        }
    }

    // 添加最后一个字段
    fields.append(currentField);

    // 去除字段首尾的空白（除非整个字段被引号包围）
    for (int i = 0; i < fields.size(); ++i) {
        QString& field = fields[i];
        if (field.isEmpty()) {
            continue;
        }

        // 如果字段以引号开始和结束，则移除它们
        if (field.startsWith('"') && field.endsWith('"')) {
            field = field.mid(1, field.length() - 2);
            // 替换双引号转义
            field.replace("\"\"", "\"");
        }

        // 如果字段没有被引号包围，则去除空白
        // 注意：我们无法精确知道原字段是否被引号包围，这里进行合理假设
        if (!wasQuotes || i != fields.size() - 1) {
            field = field.trimmed();
        }
    }

    return fields;
}

BatchProcessor::BatchProcessor(QObject *parent)
    : QThread(parent)
    , stopRequested_(false) {
    // 注册元类型，用于信号槽传递
    qRegisterMetaType<FileProcessResult>("optikg::FileProcessResult");
}

BatchProcessor::~BatchProcessor() {
    stop();
    quit();
    wait();
}

void BatchProcessor::setFiles(const QStringList& files) {
    files_ = files;
}

void BatchProcessor::setJsonContentField(const QString& fieldName) {
    jsonContentField_ = fieldName;
}

void BatchProcessor::setCsvContentColumn(const QString& columnName) {
    csvContentColumn_ = columnName;
}

void BatchProcessor::setCsvEncoding(const QString& encoding) {
    csvEncoding_ = encoding;
}

void BatchProcessor::setThreshold(float threshold) {
    threshold_ = threshold;
}

void BatchProcessor::setModelPath(const QString& path) {
    modelPath_ = path;
}

void BatchProcessor::stop() {
    stopRequested_ = true;
}

QList<FileProcessResult> BatchProcessor::getResults() const {
    return results_;
}

void BatchProcessor::run() {
    results_.clear();
    stopRequested_ = false;
    
    int totalFiles = files_.size();
    int successCount = 0;
    int failCount = 0;
    int totalRecords = 0;
    int processedRecords = 0;
    
    Logger::debug("========================================");
    Logger::debug("BatchProcessor started");
    Logger::debug(QString("Total files: %1").arg(totalFiles));
    Logger::debug(QString("JSON field: %1").arg(jsonContentField_));
    Logger::debug(QString("Model path: %1").arg(modelPath_.isEmpty() ? "(auto-detect)" : modelPath_));
    Logger::debug(QString("Threshold: %1").arg(threshold_));
    Logger::debug("========================================");
    
    // ========== 阶段1：扫描所有文件获取总记录数 ==========
    Logger::debug("\n[阶段1] 扫描文件获取记录数...");
    emit scanProgress(0, totalFiles, tr("开始扫描..."));
    
    QList<int> fileRecordCounts;  // 每个文件的记录数
    
    for (int i = 0; i < totalFiles; ++i) {
        if (stopRequested_) {
            Logger::debug("Scan stopped by user");
            break;
        }
        
        const QString& filePath = files_[i];
        QString fileName = QFileInfo(filePath).fileName();
        emit scanProgress(i + 1, totalFiles, fileName);
        
        // 快速扫描获取记录数（不加载模型）
        int recordCount = 0;
        if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
            recordCount = scanJsonFile(filePath);
        } else if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
            recordCount = scanCsvFile(filePath);
        }
        
        fileRecordCounts.append(recordCount);
        totalRecords += recordCount;
        
        qDebug() << "  Scanned:" << fileName << "->" << recordCount << "records";
    }
    
    emit scanFinished(totalFiles, totalRecords);
    qDebug() << "[阶段1完成] Total records to process:" << totalRecords;
    
    if (totalRecords == 0) {
        Logger::warning("No records found to process!");
        emit batchFinished(totalFiles, 0, totalFiles);
        return;
    }
    
    // ========== 阶段2：初始化推理引擎 ==========
    qDebug() << "\n[阶段2] 初始化推理引擎...";
    std::unique_ptr<InferenceEngine> engine;
    
    // 如果 modelPath_ 为空，尝试自动检测
    if (modelPath_.isEmpty()) {
        ConfigManager& config = ConfigManager::instance();
        modelPath_ = config.autoDetectModelPath();
    }
    
    if (!modelPath_.isEmpty()) {
        engine = std::make_unique<InferenceEngine>();
        qDebug() << "Loading model from:" << modelPath_;
        emit progressChanged(0, totalRecords, tr("正在加载模型..."));
        
        if (!engine->loadModel(modelPath_)) {
            Logger::warning("Failed to load model! Will return empty results.");
        } else {
            qDebug() << "Model loaded successfully!";
            // 设置阈值
            engine->setThreshold(threshold_);
            qDebug() << "Engine threshold set to:" << threshold_;
        }
    } else {
        Logger::warning("No model path found! Please set model path in Settings.");
    }
    
    // ========== 阶段3：处理所有记录 ==========
    qDebug() << "\n[阶段3] 开始处理记录...";
    
    for (int i = 0; i < totalFiles; ++i) {
        if (stopRequested_) {
            qDebug() << "Processing stopped by user";
            break;
        }
        
        const QString& filePath = files_[i];
        QString fileName = QFileInfo(filePath).fileName();
        int fileRecords = fileRecordCounts[i];
        
        qDebug() << "\n  Processing file" << (i + 1) << "/" << totalFiles << ":" << fileName;
        qDebug() << "  Records in file:" << fileRecords;
        
        // 根据文件类型处理（传入进度跟踪参数）
        FileProcessResult result(filePath);
        InferenceEngine* enginePtr = engine ? engine.get() : nullptr;
        
        if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
            result = processJsonFileWithProgress(filePath, enginePtr, processedRecords, totalRecords);
        } else if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
            result = processCsvFileWithProgress(filePath, enginePtr, processedRecords, totalRecords);
        } else {
            result.success = false;
            result.errorMessage = tr("不支持的文件格式");
        }
        
        // 统计结果
        if (result.success) {
            successCount++;
        } else {
            failCount++;
            emit errorOccurred(filePath, result.errorMessage);
        }
        
        results_.append(result);
        emit fileProcessed(result);
        
        qDebug() << "  File result: success=" << result.success 
                 << ", records=" << result.recordCount 
                 << ", triples=" << result.triples.size();
    }
    
    qDebug() << "\n========================================";
    qDebug() << "BatchProcessor finished";
    qDebug() << "Total files:" << totalFiles;
    qDebug() << "Success:" << successCount;
    qDebug() << "Fail:" << failCount;
    qDebug() << "Total records processed:" << processedRecords;
    qDebug() << "========================================";
    
    emit batchFinished(totalFiles, successCount, failCount);
}

FileProcessResult BatchProcessor::processJsonFile(const QString& filePath, InferenceEngine* engine) {
    FileProcessResult result(filePath);
    QElapsedTimer timer;
    timer.start();
    
    // 打开文件
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = tr("无法打开文件: %1").arg(file.errorString());
        return result;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    // 解析 JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    // 如果标准解析失败，尝试 JSON Lines 格式（每行一个 JSON 对象）
    QStringList texts;
    
    if (parseError.error != QJsonParseError::NoError) {
        // 尝试按行解析（JSON Lines 格式）
        QTextStream stream(data);
        stream.setEncoding(QStringConverter::Utf8);
        
        int lineNum = 0;
        int successLines = 0;
        
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            lineNum++;
            
            // 跳过空行
            if (line.isEmpty()) {
                continue;
            }
            
            QJsonParseError lineError;
            QJsonDocument lineDoc = QJsonDocument::fromJson(line.toUtf8(), &lineError);
            
            if (lineError.error == QJsonParseError::NoError) {
                successLines++;
                // 从行内提取文本
                if (lineDoc.isObject()) {
                    QJsonObject obj = lineDoc.object();
                    QString text = obj.value(jsonContentField_).toString();
                    if (!text.isEmpty()) {
                        texts.append(text);
                    }
                }
            } else {
                // 记录第一行错误用于调试
                if (successLines == 0 && lineNum == 1) {
                    qDebug() << "First line parse error:" << lineError.errorString();
                }
            }
        }
        
        // 如果成功解析了至少一行，认为是 JSON Lines 格式
        if (successLines > 0) {
            qDebug() << "Parsed as JSON Lines format," << successLines << "lines," << texts.size() << "texts extracted";
        } else {
            // 真正的解析错误
            result.errorMessage = tr("JSON 解析错误: %1 (尝试作为 JSON Lines 也失败)").arg(parseError.errorString());
            return result;
        }
    } else {
    
    // 标准 JSON 格式解析
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        
        // 检查是否为 JSON Lines 格式（包含 records/items/data 数组）
        if (obj.contains("records")) {
            QJsonArray records = obj["records"].toArray();
            for (const QJsonValue& val : records) {
                if (val.isObject()) {
                    QJsonObject record = val.toObject();
                    QString text = record.value(jsonContentField_).toString();
                    if (!text.isEmpty()) {
                        texts.append(text);
                    }
                }
            }
        } else if (obj.contains("items")) {
            QJsonArray items = obj["items"].toArray();
            for (const QJsonValue& val : items) {
                if (val.isObject()) {
                    QJsonObject item = val.toObject();
                    QString text = item.value(jsonContentField_).toString();
                    if (!text.isEmpty()) {
                        texts.append(text);
                    }
                }
            }
        } else if (obj.contains("data")) {
            QJsonArray data_array = obj["data"].toArray();
            for (const QJsonValue& val : data_array) {
                if (val.isObject()) {
                    QJsonObject data_item = val.toObject();
                    QString text = data_item.value(jsonContentField_).toString();
                    if (!text.isEmpty()) {
                        texts.append(text);
                    }
                }
            }
        } else {
            // 单条记录格式
            QString text = obj.value(jsonContentField_).toString();
            if (!text.isEmpty()) {
                texts.append(text);
            }
        }
    } else if (doc.isArray()) {
        // 直接是对象数组
        QJsonArray array = doc.array();
        for (const QJsonValue& val : array) {
            if (val.isObject()) {
                QJsonObject obj = val.toObject();
                QString text = obj.value(jsonContentField_).toString();
                if (!text.isEmpty()) {
                    texts.append(text);
                }
            } else if (val.isString()) {
                // 字符串数组
                texts.append(val.toString());
            }
        }
    }
    }  // 关闭标准 JSON 格式的 else 分支
    
    if (texts.isEmpty()) {
        result.errorMessage = tr("未找到内容字段 '%1'").arg(jsonContentField_);
        qDebug() << "No texts found with field:" << jsonContentField_;
        return result;
    }
    
    qDebug() << "Found" << texts.size() << "texts to process";
    // 显示第一条文本的前 50 个字符作为调试
    if (!texts.isEmpty()) {
        QString preview = texts.first().left(50).replace("\n", " ");
        qDebug() << "First text preview:" << preview << "...";
    }
    
    // 处理每条文本
    result.recordCount = texts.size();
    int textIndex = 0;
    for (const QString& text : texts) {
        if (stopRequested_) {
            break;
        }

        QList<Triple> triples = extractFromText(text, engine);

        // 调试：打印每条文本的提取结果
        if (triples.isEmpty()) {
            qDebug() << "Text" << (textIndex + 1) << ": No triples extracted";
        } else {
            qDebug() << "Text" << (textIndex + 1) << ": Extracted" << triples.size() << "triples";
        }

        // 创建抽取记录（使用0作为处理时间，因为单个文本处理时间未单独计时）
        ExtractionRecord record(text, triples, 0);
        result.extractionRecords.append(record);
        result.triples.append(triples);
        textIndex++;
    }
    
    result.processTimeMs = timer.elapsed();
    result.success = true;
    
    return result;
}

FileProcessResult BatchProcessor::processCsvFile(const QString& filePath, InferenceEngine* engine) {


    FileProcessResult result(filePath);
    QElapsedTimer timer;
    timer.start();
    
    // 打开文件
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = tr("无法打开文件: %1").arg(file.errorString());
        return result;
    }
    
    // 设置 CSV 编码
    QTextStream stream(&file);
    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    if (!csvEncoding_.isEmpty()) {
        auto converter = QStringConverter::encodingForName(csvEncoding_.toUtf8());
        if (converter) {
            encoding = *converter;
        } else {
            Logger::warning(QString("Unknown CSV encoding '%1', falling back to UTF-8").arg(csvEncoding_));
        }
    }
    stream.setEncoding(encoding);
    
    // 读取表头
    QString headerLine = stream.readLine();
    if (headerLine.isEmpty()) {
        result.errorMessage = tr("CSV 文件为空或没有表头");
        file.close();
        return result;
    }
    
    // 解析表头
    QStringList headers = parseCsvLine(headerLine);
    int contentColumnIndex = -1;

    for (int i = 0; i < headers.size(); ++i) {
        QString header = headers[i];

        if (header.compare(csvContentColumn_, Qt::CaseInsensitive) == 0) {
            contentColumnIndex = i;
            break;
        }
    }
    
    if (contentColumnIndex == -1) {
        result.errorMessage = tr("未找到列 '%1'，可用列: %2")
            .arg(csvContentColumn_)
            .arg(headers.join(", "));
        file.close();
        return result;
    }
    
    // 读取数据行
    QStringList texts;
    int lineNumber = 1; // 从 1 开始，0 是表头
    
    while (!stream.atEnd() && !stopRequested_) {
        QString line = stream.readLine();
        lineNumber++;
        
        if (line.trimmed().isEmpty()) {
            continue;
        }
        
        // 使用改进的 CSV 解析，处理包含逗号的字段
        QStringList fields = parseCsvLine(line);

        if (contentColumnIndex < fields.size()) {
            QString text = fields[contentColumnIndex];

            if (!text.isEmpty()) {
                texts.append(text);
            }
        }
    }
    
    file.close();
    
    // 处理每条文本
    result.recordCount = texts.size();
    for (const QString& text : texts) {
        if (stopRequested_) {
            break;
        }

        QList<Triple> triples = extractFromText(text, engine);

        // 创建抽取记录（使用0作为处理时间，因为单个文本处理时间未单独计时）
        ExtractionRecord record(text, triples, 0);
        result.extractionRecords.append(record);
        result.triples.append(triples);
    }
    
    result.processTimeMs = timer.elapsed();
    result.success = true;
    
    return result;
}

QList<Triple> BatchProcessor::extractFromText(const QString& text, InferenceEngine* engine, int recordIndex) {
    QString prefix = recordIndex >= 0 ? QString("[Record %1] ").arg(recordIndex) : "";
    
    // 提取三元组
    if (text.isEmpty()) {
        qDebug() << prefix << "Skip: empty text";
        return QList<Triple>();
    }
    
    if (stopRequested_) {
        qDebug() << prefix << "Skip: stop requested";
        return QList<Triple>();
    }
    
    QString textPreview = text.left(50).replace("\n", " ");
    qDebug() << prefix << "Processing text:" << textPreview << "...";
    qDebug() << prefix << "Text length:" << text.length();
    
    // 检查 engine
    if (!engine) {
        Logger::warning(QString("%1 No inference engine provided!").arg(prefix));
        Logger::warning(QString("%1 Batch processor threshold: %2").arg(prefix).arg(threshold_));
        return QList<Triple>();
    }

    if (!engine->isModelLoaded()) {
        Logger::warning(QString("%1 Model not loaded!").arg(prefix));
        Logger::warning(QString("%1 Batch processor threshold: %2").arg(prefix).arg(threshold_));
        return QList<Triple>();
    }

    qDebug() << prefix << "Running inference with threshold:" << threshold_;
    qDebug() << prefix << "Text full content (first 200 chars):" << text.left(200).replace("\n", "\\n");
    qDebug() << prefix << "Text contains quotes:" << (text.contains('"') ? "yes" : "no") << (text.contains('\'') ? "yes" : "no");
    qDebug() << prefix << "Text trimmed vs original length:" << text.trimmed().length() << "vs" << text.length();

    auto results = engine->infer(text);
    
    if (results.isEmpty()) {
        qDebug() << prefix << "No triples extracted from this text";
    } else {
        qDebug() << prefix << "Extracted" << results.size() << "triples:";
        for (int i = 0; i < qMin(3, results.size()); ++i) {
            const auto& t = results[i];
            qDebug() << prefix << "  Triple" << (i+1) << ":" 
                     << t.subject.name << "-" << t.relation << "->" << t.object.name
                     << "(conf:" << t.confidence << ")";
        }
        if (results.size() > 3) {
            qDebug() << prefix << "  ... and" << (results.size() - 3) << "more";
        }
    }
    
    return results;
}

// ========== 扫描函数实现 ==========

int BatchProcessor::scanJsonFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return 0;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    // 尝试标准 JSON 解析
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error == QJsonParseError::NoError) {
        if (doc.isArray()) {
            return doc.array().size();
        } else if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("records")) return obj["records"].toArray().size();
            if (obj.contains("items")) return obj["items"].toArray().size();
            if (obj.contains("data")) return obj["data"].toArray().size();
            // 单对象算1条
            if (obj.contains(jsonContentField_)) return 1;
        }
        return 0;
    }
    
    // JSON Lines 格式：数非空行
    int count = 0;
    QTextStream stream(data);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (!line.isEmpty()) {
            // 验证是否是有效 JSON
            QJsonParseError err;
            QJsonDocument::fromJson(line.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError) {
                count++;
            }
        }
    }
    return count;
}

int BatchProcessor::scanCsvFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    
    QTextStream stream(&file);
    QString headerLine = stream.readLine(); // 跳过表头
    if (headerLine.isEmpty()) {
        file.close();
        return 0;
    }
    
    int count = 0;
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (!line.isEmpty()) {
            count++;
        }
    }
    
    file.close();
    return count;
}

// ========== 带进度的处理函数 ==========

FileProcessResult BatchProcessor::processJsonFileWithProgress(
    const QString& filePath, InferenceEngine* engine,
    int& processedRecords, int totalRecords) {
    
    FileProcessResult result(filePath);
    QElapsedTimer timer;
    timer.start();
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = tr("无法打开文件: %1").arg(file.errorString());
        return result;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    // 解析 JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    QStringList texts;
    bool isJsonLines = false;
    
    if (parseError.error != QJsonParseError::NoError) {
        // JSON Lines 格式
        isJsonLines = true;
        QTextStream stream(data);
        stream.setEncoding(QStringConverter::Utf8);
        
        while (!stream.atEnd() && !stopRequested_) {
            QString line = stream.readLine().trimmed();
            if (line.isEmpty()) continue;
            
            QJsonParseError err;
            QJsonDocument lineDoc = QJsonDocument::fromJson(line.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError && lineDoc.isObject()) {
                QString text = lineDoc.object().value(jsonContentField_).toString();
                if (!text.isEmpty()) texts.append(text);
            }
        }
    } else {
        // 标准 JSON
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("records")) {
                for (const auto& v : obj["records"].toArray()) {
                    QString text = v.toObject().value(jsonContentField_).toString();
                    if (!text.isEmpty()) texts.append(text);
                }
            } else if (obj.contains("items")) {
                for (const auto& v : obj["items"].toArray()) {
                    QString text = v.toObject().value(jsonContentField_).toString();
                    if (!text.isEmpty()) texts.append(text);
                }
            } else if (obj.contains("data")) {
                for (const auto& v : obj["data"].toArray()) {
                    QString text = v.toObject().value(jsonContentField_).toString();
                    if (!text.isEmpty()) texts.append(text);
                }
            } else if (obj.contains(jsonContentField_)) {
                texts.append(obj.value(jsonContentField_).toString());
            }
        } else if (doc.isArray()) {
            for (const auto& v : doc.array()) {
                if (v.isObject()) {
                    QString text = v.toObject().value(jsonContentField_).toString();
                    if (!text.isEmpty()) texts.append(text);
                } else if (v.isString()) {
                    texts.append(v.toString());
                }
            }
        }
    }
    
    if (texts.isEmpty()) {
        qDebug() << "No texts extracted from file:" << filePath;
        qDebug() << "JSON content field configured:" << jsonContentField_;
        result.errorMessage = tr("未找到内容字段 '%1'").arg(jsonContentField_);
        return result;
    }
    
    // 处理每条文本（带进度）
    result.recordCount = texts.size();
    QString fileName = QFileInfo(filePath).fileName();

    // 原子计数器用于进度更新
    QAtomicInt processedLocal(processedRecords);

    // 捕获配置参数（避免悬垂引用）
    QString currentModelPath = modelPath_;
    float currentThreshold = threshold_;
    QString configModelPath = currentModelPath.isEmpty() ?
                              ConfigManager::instance().autoDetectModelPath() : currentModelPath;

    // Lambda 处理单个文本，返回抽取记录
    auto processText = [&, currentThreshold, configModelPath, fileName](const QString& text) -> ExtractionRecord {
        if (stopRequested_) {
            return ExtractionRecord();
        }

        QElapsedTimer recordTimer;
        recordTimer.start();

        // 使用线程本地存储的推理引擎（每个线程只加载一次模型）
        auto& storage = getEngineStorage();
        if (!storage.hasLocalData()) {
            std::unique_ptr<InferenceEngine> threadEngine = std::make_unique<InferenceEngine>();
            if (!configModelPath.isEmpty() && !threadEngine->loadModel(configModelPath)) {
                Logger::warning("Failed to load model in thread-local engine");
                return ExtractionRecord();
            }
            threadEngine->setThreshold(currentThreshold);
            storage.setLocalData(threadEngine.release());
        } else {
            // 确保阈值设置正确
            InferenceEngine* threadEngine = storage.localData();
            if (threadEngine && threadEngine->isModelLoaded()) {
                threadEngine->setThreshold(currentThreshold);
            }
        }

        InferenceEngine* threadEngine = storage.localData();
        if (!threadEngine || !threadEngine->isModelLoaded()) {
            return ExtractionRecord();
        }

        // 执行推理
        QList<Triple> triples = threadEngine->infer(text);

        // 更新进度
        int newProcessed = processedLocal.fetchAndAddOrdered(1) + 1;
        emit progressChanged(newProcessed, totalRecords, fileName);

        // 创建抽取记录
        return ExtractionRecord(text, triples, recordTimer.elapsed());
    };

    // 并行处理文本（限制并发线程数防止内存过高）
    QThreadPool pool;
    pool.setMaxThreadCount(2); // 限制为2个并发线程，减少内存占用
    QList<ExtractionRecord> allRecords = QtConcurrent::blockingMapped(&pool, texts, processText);

    // 保存抽取记录并合并三元组
    for (const ExtractionRecord& record : allRecords) {
        result.extractionRecords.append(record);
        result.triples.append(record.triples);
    }

    // 更新 processedRecords
    processedRecords = processedLocal.loadRelaxed();
    
    result.processTimeMs = timer.elapsed();
    result.success = true;
    
    return result;
}

FileProcessResult BatchProcessor::processCsvFileWithProgress(
    const QString& filePath, InferenceEngine* engine,
    int& processedRecords, int totalRecords) {
    
    FileProcessResult result(filePath);
    QElapsedTimer timer;
    timer.start();
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = tr("无法打开文件: %1").arg(file.errorString());
        return result;
    }
    
    // 设置 CSV 编码
    QTextStream stream(&file);
    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    if (!csvEncoding_.isEmpty()) {
        auto converter = QStringConverter::encodingForName(csvEncoding_.toUtf8());
        if (converter) {
            encoding = *converter;
        } else {
            Logger::warning(QString("Unknown CSV encoding '%1', falling back to UTF-8").arg(csvEncoding_));
        }
    }
    stream.setEncoding(encoding);
    
    QString headerLine = stream.readLine();
    if (headerLine.isEmpty()) {
        result.errorMessage = tr("CSV 文件为空或没有表头");
        file.close();
        return result;
    }
    
    // 查找列索引
    QStringList headers = parseCsvLine(headerLine);
    int contentColumnIndex = -1;
    for (int i = 0; i < headers.size(); ++i) {
        QString header = headers[i];
        if (header.compare(csvContentColumn_, Qt::CaseInsensitive) == 0) {
            contentColumnIndex = i;
            break;
        }
    }
    
    if (contentColumnIndex == -1) {
        qDebug() << "CSV column not found:" << csvContentColumn_;
        qDebug() << "Available headers:" << headers;
        result.errorMessage = tr("未找到列 '%1'").arg(csvContentColumn_);
        file.close();
        return result;
    }
    
    // 读取所有数据行
    QStringList texts;
    int lineNum = 0;

    while (!stream.atEnd() && !stopRequested_) {
        QString line = stream.readLine();
        if (line.trimmed().isEmpty()) continue;

        // 使用改进的 CSV 解析
        QStringList fields = parseCsvLine(line);
        if (contentColumnIndex < fields.size()) {
            QString text = fields[contentColumnIndex];
            if (!text.isEmpty()) {
                texts.append(text);
                lineNum++;
            }
        }
    }

    file.close();

    if (texts.isEmpty()) {
        result.success = true; // 空文件也算成功
        result.processTimeMs = timer.elapsed();
        return result;
    }


    // 原子计数器用于进度更新
    QAtomicInt processedLocal(processedRecords);
    QString fileName = QFileInfo(filePath).fileName();

    // 捕获配置参数（避免悬垂引用）
    QString currentModelPath = modelPath_;
    float currentThreshold = threshold_;
    QString configModelPath = currentModelPath.isEmpty() ?
                              ConfigManager::instance().autoDetectModelPath() : currentModelPath;

    // Lambda 处理单个文本，返回抽取记录
    auto processText = [&, currentThreshold, configModelPath, fileName](const QString& text) -> ExtractionRecord {
        if (stopRequested_) {
            return ExtractionRecord();
        }

        QElapsedTimer recordTimer;
        recordTimer.start();

        // 使用线程本地存储的推理引擎（每个线程只加载一次模型）
        auto& storage = getEngineStorage();
        if (!storage.hasLocalData()) {
            std::unique_ptr<InferenceEngine> threadEngine = std::make_unique<InferenceEngine>();
            if (!configModelPath.isEmpty() && !threadEngine->loadModel(configModelPath)) {
                Logger::warning("Failed to load model in thread-local engine");
                return ExtractionRecord();
            }
            threadEngine->setThreshold(currentThreshold);
            storage.setLocalData(threadEngine.release());
        } else {
            // 确保阈值设置正确
            InferenceEngine* threadEngine = storage.localData();
            if (threadEngine && threadEngine->isModelLoaded()) {
                threadEngine->setThreshold(currentThreshold);
            }
        }

        InferenceEngine* threadEngine = storage.localData();
        if (!threadEngine || !threadEngine->isModelLoaded()) {
            return ExtractionRecord();
        }

        // 执行推理
        QList<Triple> triples = threadEngine->infer(text);

        // 更新进度
        int newProcessed = processedLocal.fetchAndAddOrdered(1) + 1;
        emit progressChanged(newProcessed, totalRecords, fileName);

        // 创建抽取记录
        return ExtractionRecord(text, triples, recordTimer.elapsed());
    };

    // 并行处理文本（限制并发线程数防止内存过高）
    QThreadPool pool;
    pool.setMaxThreadCount(2); // 限制为2个并发线程，减少内存占用
    QList<ExtractionRecord> allRecords = QtConcurrent::blockingMapped(&pool, texts, processText);

    // 保存抽取记录并合并三元组
    for (const ExtractionRecord& record : allRecords) {
        result.extractionRecords.append(record);
        result.triples.append(record.triples);
    }

    // 更新 processedRecords 和 recordCount
    processedRecords = processedLocal.loadRelaxed();
    result.recordCount = texts.size();

    result.processTimeMs = timer.elapsed();
    result.success = true;

    return result;
}

} // namespace optikg
