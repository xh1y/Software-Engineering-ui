#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include "../utils/logger.h"

namespace {
    // 检查是否在主线程，如果不是则记录错误并返回 false
    bool checkMainThread(const QString& operationName) {
        if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
            optikg::Logger::critical(QString("致命错误：试图在非主线程访问数据库！操作: %1").arg(operationName));
            return false;
        }
        return true;
    }
}

namespace optikg {

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , databasePath_() {
}

DatabaseManager::~DatabaseManager() {
    close();
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initialize(const QString& databasePath) {
    if (!checkMainThread("initialize")) {
        return false;
    }
    
    Q_ASSERT_X(QSqlDatabase::isDriverAvailable("QSQLITE"), 
               "DatabaseManager::initialize", "SQLite driver not available");
    
    if (db_.isOpen()) {
        db_.close();
    }

    QString path = databasePath;
    if (path.isEmpty()) {
        // 默认路径：用户目录下的 .optikg 目录
        QString homeDir = QDir::homePath();
        QDir appDir(QDir(homeDir).filePath(".optikg"));
        if (!appDir.exists()) {
            appDir.mkpath(".");
        }
        path = appDir.filePath("optikg.db");
    }

    databasePath_ = path;

    // 打开数据库
    db_ = QSqlDatabase::addDatabase("QSQLITE", "optikg_connection");
    db_.setDatabaseName(databasePath_);

    if (!db_.open()) {
        qCritical() << "Failed to open database:" << db_.lastError().text();
        return false;
    }

    // 创建表
    if (!createTables()) {
        qCritical() << "Failed to create tables";
        db_.close();
        return false;
    }

    qDebug() << "Database initialized at:" << databasePath_;
    return true;
}

void DatabaseManager::close() {
    if (db_.isOpen()) {
        db_.close();
    }
}

bool DatabaseManager::createTables() {
    bool success = true;

    // 启用外键约束
    QSqlQuery query(db_);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        Logger::warning(QString("Failed to enable foreign keys: %1").arg(query.lastError().text()));
    }

    success = success && createExtractionRecordsTable();
    success = success && createEntitiesTable();
    success = success && createRelationsTable();

    return success;
}

bool DatabaseManager::createExtractionRecordsTable() {
    QSqlQuery query(db_);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS extraction_records (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            content TEXT NOT NULL,
            created_at DATETIME NOT NULL,
            process_time_ms INTEGER NOT NULL,
            avg_confidence REAL NOT NULL,
            entity_count INTEGER NOT NULL,
            relation_count INTEGER NOT NULL,
            raw_text TEXT,
            source_file TEXT
        )
    )";

    if (!query.exec(sql)) {
        qCritical() << "Failed to create extraction_records table:" << query.lastError().text();
        return false;
    }

    // 创建索引
    query.exec("CREATE INDEX IF NOT EXISTS idx_records_created_at ON extraction_records(created_at)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_records_confidence ON extraction_records(avg_confidence)");

    return true;
}

bool DatabaseManager::createEntitiesTable() {
    QSqlQuery query(db_);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS entities (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            record_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            type INTEGER NOT NULL,
            start_pos INTEGER,
            end_pos INTEGER,
            confidence REAL NOT NULL,
            FOREIGN KEY (record_id) REFERENCES extraction_records(id) ON DELETE CASCADE
        )
    )";

    if (!query.exec(sql)) {
        qCritical() << "Failed to create entities table:" << query.lastError().text();
        return false;
    }

    query.exec("CREATE INDEX IF NOT EXISTS idx_entities_record_id ON entities(record_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_entities_type ON entities(type)");

    return true;
}

bool DatabaseManager::createRelationsTable() {
    QSqlQuery query(db_);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS relations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            record_id INTEGER NOT NULL,
            subject_id INTEGER NOT NULL,
            object_id INTEGER NOT NULL,
            relation TEXT NOT NULL,
            relation_type INTEGER NOT NULL,
            confidence REAL NOT NULL,
            FOREIGN KEY (record_id) REFERENCES extraction_records(id) ON DELETE CASCADE,
            FOREIGN KEY (subject_id) REFERENCES entities(id) ON DELETE CASCADE,
            FOREIGN KEY (object_id) REFERENCES entities(id) ON DELETE CASCADE
        )
    )";

    if (!query.exec(sql)) {
        qCritical() << "Failed to create relations table:" << query.lastError().text();
        return false;
    }

    query.exec("CREATE INDEX IF NOT EXISTS idx_relations_record_id ON relations(record_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_relations_relation ON relations(relation)");

    return true;
}

qint64 DatabaseManager::insertExtractionRecord(const ExtractionRecord& record) {
    if (!checkMainThread("insertExtractionRecord")) {
        return -1;
    }
    
    Q_ASSERT_X(db_.isOpen(), "DatabaseManager::insertExtractionRecord", "Database is not open");
    
    if (!db_.isOpen()) {
        Logger::warning("Database not open");
        return -1;
    }

    QSqlQuery query(db_);
    query.prepare(R"(
        INSERT INTO extraction_records
        (content, created_at, process_time_ms, avg_confidence, entity_count, relation_count, raw_text, source_file)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(record.content);
    query.addBindValue(record.createdAt);
    query.addBindValue(record.processTimeMs);
    query.addBindValue(record.avgConfidence);
    query.addBindValue(record.entityCount);
    query.addBindValue(record.relationCount);
    query.addBindValue(record.content); // raw_text 暂用content
    query.addBindValue(""); // source_file 留空

    if (!query.exec()) {
        qCritical() << "Failed to insert extraction record:" << query.lastError().text();
        return -1;
    }

    qint64 recordId = query.lastInsertId().toLongLong();

    // 保存三元组
    if (!record.triples.isEmpty()) {
        saveTriples(recordId, record.triples);
    }

    emit databaseChanged();
    return recordId;
}

void DatabaseManager::saveTriples(qint64 recordId, const QList<Triple>& triples) {
    // 注意：此方法期望调用者已经开始了事务
    // 保存实体
    QHash<QPair<QString, EntityType>, qint64> entityIdMap;

    for (const auto& triple : triples) {
        // 保存subject
        auto subjectKey = qMakePair(triple.subject.name, triple.subject.type);
        if (!entityIdMap.contains(subjectKey)) {
            QSqlQuery query(db_);
            query.prepare(R"(
                INSERT INTO entities (record_id, name, type, start_pos, end_pos, confidence)
                VALUES (?, ?, ?, ?, ?, ?)
            )");
            query.addBindValue(recordId);
            query.addBindValue(triple.subject.name);
            query.addBindValue(static_cast<int>(triple.subject.type));
            query.addBindValue(triple.subject.startPos);
            query.addBindValue(triple.subject.endPos);
            query.addBindValue(triple.subject.confidence);

            if (!query.exec()) {
                throw std::runtime_error(query.lastError().text().toStdString());
            }

            entityIdMap[subjectKey] = query.lastInsertId().toLongLong();
        }

        // 保存object
        auto objectKey = qMakePair(triple.object.name, triple.object.type);
        if (!entityIdMap.contains(objectKey)) {
            QSqlQuery query(db_);
            query.prepare(R"(
                INSERT INTO entities (record_id, name, type, start_pos, end_pos, confidence)
                VALUES (?, ?, ?, ?, ?, ?)
            )");
            query.addBindValue(recordId);
            query.addBindValue(triple.object.name);
            query.addBindValue(static_cast<int>(triple.object.type));
            query.addBindValue(triple.object.startPos);
            query.addBindValue(triple.object.endPos);
            query.addBindValue(triple.object.confidence);

            if (!query.exec()) {
                throw std::runtime_error(query.lastError().text().toStdString());
            }

            entityIdMap[objectKey] = query.lastInsertId().toLongLong();
        }

        // 保存关系
        QSqlQuery query(db_);
        query.prepare(R"(
            INSERT INTO relations (record_id, subject_id, object_id, relation, relation_type, confidence)
            VALUES (?, ?, ?, ?, ?, ?)
        )");
        query.addBindValue(recordId);
        query.addBindValue(entityIdMap[subjectKey]);
        query.addBindValue(entityIdMap[objectKey]);
        query.addBindValue(triple.relation);
        query.addBindValue(static_cast<int>(triple.relationType));
        query.addBindValue(triple.confidence);

        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }
    }
}

// TODO: 实现其他方法：updateExtractionRecord, deleteExtractionRecord, getAllExtractionRecords等

bool DatabaseManager::deleteExtractionRecord(qint64 id) {
    if (!checkMainThread("deleteExtractionRecord")) {
        return false;
    }
    
    QSqlQuery query(db_);
    query.prepare("DELETE FROM extraction_records WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qCritical() << "Failed to delete extraction record:" << query.lastError().text();
        return false;
    }

    emit databaseChanged();
    return query.numRowsAffected() > 0;
}

QList<ExtractionRecord> DatabaseManager::getAllExtractionRecords(int limit) {
    if (!checkMainThread("getAllExtractionRecords")) {
        return QList<ExtractionRecord>();
    }
    
    QList<ExtractionRecord> records;

    QSqlQuery query(db_);
    query.prepare(R"(
        SELECT id, content, created_at, process_time_ms, avg_confidence, entity_count, relation_count
        FROM extraction_records
        ORDER BY created_at DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    if (!query.exec()) {
        qCritical() << "Failed to get extraction records:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        ExtractionRecord record;
        record.id = query.value(0).toLongLong();
        record.content = query.value(1).toString();
        record.createdAt = query.value(2).toDateTime();
        record.processTimeMs = query.value(3).toInt();
        record.avgConfidence = query.value(4).toFloat();
        record.entityCount = query.value(5).toInt();
        record.relationCount = query.value(6).toInt();

        // 延迟加载三元组
        record.triples = loadTriples(record.id);

        records.append(record);
    }

    return records;
}

// 其他方法暂时留空实现
bool DatabaseManager::updateExtractionRecord(const ExtractionRecord& record) {
    Q_UNUSED(record);
    checkMainThread("updateExtractionRecord");
    return false;
}

bool DatabaseManager::deleteMultipleRecords(const QList<qint64>& ids) {
    if (!checkMainThread("deleteMultipleRecords")) {
        return false;
    }
    
    if (ids.isEmpty()) return true;

    QString idList = QStringList(QStringList() << "?").join(",");
    QSqlQuery query(db_);
    query.prepare(QString("DELETE FROM extraction_records WHERE id IN (%1)").arg(idList));

    for (qint64 id : ids) {
        query.addBindValue(id);
    }

    if (!query.exec()) {
        qCritical() << "Failed to delete multiple records:" << query.lastError().text();
        return false;
    }

    emit databaseChanged();
    return true;
}

ExtractionRecord DatabaseManager::getExtractionRecord(qint64 id) {
    if (!checkMainThread("getExtractionRecord")) {
        return ExtractionRecord();
    }
    
    ExtractionRecord record;

    QSqlQuery query(db_);
    query.prepare(R"(
        SELECT id, content, created_at, process_time_ms, avg_confidence, entity_count, relation_count
        FROM extraction_records
        WHERE id = ?
    )");
    query.addBindValue(id);

    if (!query.exec() || !query.next()) {
        return record;
    }

    record.id = query.value(0).toLongLong();
    record.content = query.value(1).toString();
    record.createdAt = query.value(2).toDateTime();
    record.processTimeMs = query.value(3).toInt();
    record.avgConfidence = query.value(4).toFloat();
    record.entityCount = query.value(5).toInt();
    record.relationCount = query.value(6).toInt();
    record.triples = loadTriples(id);

    return record;
}

QList<ExtractionRecord> DatabaseManager::searchExtractionRecords(const QString& keyword,
                                                                 const QDateTime& startTime,
                                                                 const QDateTime& endTime,
                                                                 int entityType,
                                                                 int limit) {
    if (!checkMainThread("searchExtractionRecords")) {
        return QList<ExtractionRecord>();
    }
    
    QList<ExtractionRecord> records;

    QString sql = R"(
        SELECT id, content, created_at, process_time_ms, avg_confidence, entity_count, relation_count
        FROM extraction_records
        WHERE 1=1
    )";

    QVector<QVariant> bindValues;

    if (!keyword.isEmpty()) {
        sql += " AND content LIKE ?";
        bindValues.append("%" + keyword + "%");
    }

    if (startTime.isValid()) {
        sql += " AND created_at >= ?";
        bindValues.append(startTime);
    }

    if (endTime.isValid()) {
        sql += " AND created_at <= ?";
        bindValues.append(endTime);
    }

    // 实体类型筛选：只返回包含该类型实体的记录
    if (entityType >= 0) {
        sql += " AND EXISTS (SELECT 1 FROM entities WHERE entities.record_id = extraction_records.id AND entities.type = ?)";
        bindValues.append(entityType);
    }

    sql += " ORDER BY created_at DESC LIMIT ?";
    bindValues.append(limit);

    QSqlQuery query(db_);
    query.prepare(sql);

    for (const auto& value : bindValues) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        qCritical() << "Failed to search extraction records:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        ExtractionRecord record;
        record.id = query.value(0).toLongLong();
        record.content = query.value(1).toString();
        record.createdAt = query.value(2).toDateTime();
        record.processTimeMs = query.value(3).toInt();
        record.avgConfidence = query.value(4).toFloat();
        record.entityCount = query.value(5).toInt();
        record.relationCount = query.value(6).toInt();

        // 加载三元组
        record.triples = loadTriples(record.id);

        records.append(record);
    }

    return records;
}

int DatabaseManager::getRecordCount() {
    if (!checkMainThread("getRecordCount")) {
        return 0;
    }
    
    QSqlQuery query(db_);
    if (query.exec("SELECT COUNT(*) FROM extraction_records") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getEntityCount() {
    if (!checkMainThread("getEntityCount")) {
        return 0;
    }
    
    QSqlQuery query(db_);
    if (query.exec("SELECT COUNT(*) FROM entities") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getRelationCount() {
    if (!checkMainThread("getRelationCount")) {
        return 0;
    }
    
    QSqlQuery query(db_);
    if (query.exec("SELECT COUNT(*) FROM relations") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

float DatabaseManager::getAverageConfidence() {
    if (!checkMainThread("getAverageConfidence")) {
        return 0.0f;
    }
    
    QSqlQuery query(db_);
    if (query.exec("SELECT AVG(avg_confidence) FROM extraction_records") && query.next()) {
        return query.value(0).toFloat();
    }
    return 0.0f;
}

QList<Triple> DatabaseManager::loadTriples(qint64 recordId) {
    Q_ASSERT_X(recordId > 0, "DatabaseManager::loadTriples", "Invalid recordId");
    
    QList<Triple> triples;

    if (!db_.isOpen()) {
        Logger::warning("Database not open");
        return triples;
    }

    // 查询该记录的所有关系
    QSqlQuery query(db_);
    query.prepare(R"(
        SELECT
            r.id, r.relation, r.relation_type, r.confidence,
            s.id AS subject_id, s.name AS subject_name, s.type AS subject_type,
            s.start_pos AS subject_start, s.end_pos AS subject_end, s.confidence AS subject_confidence,
            o.id AS object_id, o.name AS object_name, o.type AS object_type,
            o.start_pos AS object_start, o.end_pos AS object_end, o.confidence AS object_confidence
        FROM relations r
        JOIN entities s ON r.subject_id = s.id
        JOIN entities o ON r.object_id = o.id
        WHERE r.record_id = ?
        ORDER BY r.id
    )");
    query.addBindValue(recordId);

    if (!query.exec()) {
        qCritical() << "Failed to load triples:" << query.lastError().text();
        return triples;
    }

    while (query.next()) {
        Triple triple;

        // 设置主体实体
        triple.subject.id = query.value("subject_id").toLongLong();
        triple.subject.name = query.value("subject_name").toString();
        triple.subject.type = static_cast<EntityType>(query.value("subject_type").toInt());
        triple.subject.startPos = query.value("subject_start").toInt();
        triple.subject.endPos = query.value("subject_end").toInt();
        triple.subject.confidence = query.value("subject_confidence").toFloat();

        // 设置客体实体
        triple.object.id = query.value("object_id").toLongLong();
        triple.object.name = query.value("object_name").toString();
        triple.object.type = static_cast<EntityType>(query.value("object_type").toInt());
        triple.object.startPos = query.value("object_start").toInt();
        triple.object.endPos = query.value("object_end").toInt();
        triple.object.confidence = query.value("object_confidence").toFloat();

        // 设置关系
        triple.relation = query.value("relation").toString();
        triple.relationType = static_cast<RelationType>(query.value("relation_type").toInt());
        triple.confidence = query.value("confidence").toFloat();

        triples.append(triple);
    }

    return triples;
}

bool DatabaseManager::importFromJson(const QString& filePath) {
    Q_UNUSED(filePath);
    return false;
}

bool DatabaseManager::exportToJson(const QString& filePath, const QList<qint64>& ids) {
    Q_UNUSED(filePath);
    Q_UNUSED(ids);
    return false;
}

bool DatabaseManager::exportToCsv(const QString& filePath, const QList<qint64>& ids) {
    Q_UNUSED(filePath);
    Q_UNUSED(ids);
    return false;
}

// 错误分类辅助函数
DatabaseErrorType DatabaseManager::classifyDatabaseError(const QString& errorMessage) {
    QString lowerMsg = errorMessage.toLower();
    
    if (lowerMsg.contains("connection") || lowerMsg.contains("unable to open") || 
        lowerMsg.contains("network") || lowerMsg.contains("closed")) {
        return DatabaseErrorType::ConnectionError;
    }
    if (lowerMsg.contains("constraint") || lowerMsg.contains("unique") || 
        lowerMsg.contains("foreign key") || lowerMsg.contains("primary key")) {
        return DatabaseErrorType::ConstraintError;
    }
    if (lowerMsg.contains("timeout") || lowerMsg.contains("busy") || 
        lowerMsg.contains("locked")) {
        return DatabaseErrorType::TimeoutError;
    }
    if (lowerMsg.contains("disk full") || lowerMsg.contains("no space") || 
        lowerMsg.contains("database is full")) {
        return DatabaseErrorType::DiskFullError;
    }
    if (lowerMsg.contains("permission") || lowerMsg.contains("access") || 
        lowerMsg.contains("denied") || lowerMsg.contains("readonly")) {
        return DatabaseErrorType::PermissionError;
    }
    if (lowerMsg.contains("no such table") || lowerMsg.contains("column") || 
        lowerMsg.contains("schema")) {
        return DatabaseErrorType::SchemaError;
    }
    
    return DatabaseErrorType::Unknown;
}

// 带重试机制的操作执行
bool DatabaseManager::executeWithRetry(const std::function<bool()>& operation, int maxRetries, 
                                       const QString& operationName) {
    int attempt = 0;
    while (attempt < maxRetries) {
        if (operation()) {
            return true;
        }
        
        attempt++;
        if (attempt < maxRetries) {
            // 指数退避策略
            int delayMs = qMin(100 * (1 << attempt), 5000);  // 最大5秒
            Logger::warning(QString("Operation '%1' failed (attempt %2/%3), retrying in %4ms...")
                           .arg(operationName).arg(attempt).arg(maxRetries).arg(delayMs));
            QThread::msleep(delayMs);
        }
    }
    
    Logger::error(QString("Operation '%1' failed after %2 attempts").arg(operationName).arg(maxRetries));
    return false;
}

// 重试插入记录
bool DatabaseManager::retryInsertExtractionRecords(const QList<ExtractionRecord>& records, int maxRetries) {
    Logger::info(QString("Retrying batch insert (%1 records, max %2 attempts)...")
                .arg(records.size()).arg(maxRetries));
    
    return executeWithRetry([this, &records]() -> bool {
        if (!db_.transaction()) {
            return false;
        }
        
        try {
            for (const ExtractionRecord& record : records) {
                QSqlQuery query(db_);
                query.prepare(R"(
                    INSERT INTO extraction_records
                    (content, created_at, process_time_ms, avg_confidence, entity_count, relation_count, raw_text, source_file)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                )");

                query.addBindValue(record.content);
                query.addBindValue(record.createdAt);
                query.addBindValue(record.processTimeMs);
                query.addBindValue(record.avgConfidence);
                query.addBindValue(record.entityCount);
                query.addBindValue(record.relationCount);
                query.addBindValue(record.content);
                query.addBindValue("");

                if (!query.exec()) {
                    throw std::runtime_error(query.lastError().text().toStdString());
                }

                qint64 recordId = query.lastInsertId().toLongLong();

                if (!record.triples.isEmpty()) {
                    saveTriples(recordId, record.triples);
                }
            }

            db_.commit();
            emit databaseChanged();
            return true;
            
        } catch (const std::exception& e) {
            db_.rollback();
            throw;
        }
    }, maxRetries, "batchInsert");
}

bool DatabaseManager::insertExtractionRecords(const QList<ExtractionRecord>& records) {
    if (!db_.isOpen()) {
        Logger::warning("Database not open");
        return false;
    }

    if (records.isEmpty()) {
        return true; // 空列表视为成功
    }

    // 开始事务
    db_.transaction();

    try {
        for (const ExtractionRecord& record : records) {
            QSqlQuery query(db_);
            query.prepare(R"(
                INSERT INTO extraction_records
                (content, created_at, process_time_ms, avg_confidence, entity_count, relation_count, raw_text, source_file)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            )");

            query.addBindValue(record.content);
            query.addBindValue(record.createdAt);
            query.addBindValue(record.processTimeMs);
            query.addBindValue(record.avgConfidence);
            query.addBindValue(record.entityCount);
            query.addBindValue(record.relationCount);
            query.addBindValue(record.content); // raw_text 暂用content
            query.addBindValue(""); // source_file 留空

            if (!query.exec()) {
                throw std::runtime_error(query.lastError().text().toStdString());
            }

            qint64 recordId = query.lastInsertId().toLongLong();

            // 保存三元组
            if (!record.triples.isEmpty()) {
                saveTriples(recordId, record.triples);
            }
        }

        db_.commit();
        emit databaseChanged();
        return true;
    } catch (const std::exception& e) {
        db_.rollback();
        QString errorMsg = QString::fromUtf8(e.what());
        
        // 分析错误类型
        DatabaseErrorType errorType = classifyDatabaseError(errorMsg);
        
        switch (errorType) {
            case DatabaseErrorType::ConnectionError:
                Logger::critical(QString("Database connection error during batch insert: %1").arg(errorMsg));
                // 连接错误可能需要重新初始化数据库
                emit errorOccurred(tr("数据库连接错误，请检查数据库状态"));
                break;
                
            case DatabaseErrorType::ConstraintError:
                Logger::error(QString("Database constraint violation: %1").arg(errorMsg));
                emit errorOccurred(tr("数据约束冲突，可能存在重复数据"));
                break;
                
            case DatabaseErrorType::TimeoutError:
                Logger::warning(QString("Database timeout, will retry: %1").arg(errorMsg));
                // 超时错误可以尝试重试
                return retryInsertExtractionRecords(records);
                
            case DatabaseErrorType::DiskFullError:
                Logger::critical(QString("Database disk full: %1").arg(errorMsg));
                emit errorOccurred(tr("磁盘空间不足，无法保存数据"));
                break;
                
            default:
                Logger::error(QString("Failed to insert extraction records: %1").arg(errorMsg));
                emit errorOccurred(tr("保存数据失败: %1").arg(errorMsg));
                break;
        }
        
        return false;
    }
}

} // namespace optikg