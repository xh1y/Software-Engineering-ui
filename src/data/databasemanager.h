#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QString>
#include "optikg/datamodel.h"

namespace optikg {

/**
 * @brief 数据库错误类型枚举
 */
enum class DatabaseErrorType {
    Unknown,           ///< 未知错误
    ConnectionError,   ///< 连接错误
    ConstraintError,   ///< 约束冲突
    TimeoutError,      ///< 超时
    DiskFullError,     ///< 磁盘满
    PermissionError,   ///< 权限错误
    SchemaError        ///< 表结构错误
};

class DatabaseManager : public QObject {
    Q_OBJECT

public:
    static DatabaseManager& instance();

    bool initialize(const QString& databasePath = QString());
    void close();

    // 抽取记录管理
    qint64 insertExtractionRecord(const ExtractionRecord& record);
    bool updateExtractionRecord(const ExtractionRecord& record);
    bool deleteExtractionRecord(qint64 id);
    bool deleteMultipleRecords(const QList<qint64>& ids);
    ExtractionRecord getExtractionRecord(qint64 id);
    QList<ExtractionRecord> getAllExtractionRecords(int limit = 100);
    QList<ExtractionRecord> searchExtractionRecords(const QString& keyword,
                                                   const QDateTime& startTime = QDateTime(),
                                                   const QDateTime& endTime = QDateTime(),
                                                   int entityType = -1,
                                                   int limit = 50);

    // 统计信息
    int getRecordCount();
    int getEntityCount();
    int getRelationCount();
    float getAverageConfidence();

    // 批量操作
    bool insertExtractionRecords(const QList<ExtractionRecord>& records);
    bool importFromJson(const QString& filePath);
    bool exportToJson(const QString& filePath, const QList<qint64>& ids = QList<qint64>());
    bool exportToCsv(const QString& filePath, const QList<qint64>& ids = QList<qint64>());

signals:
    void databaseChanged();
    void errorOccurred(const QString& message);  ///< 数据库错误信号

private:
    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool createTables();
    bool createExtractionRecordsTable();
    bool createEntitiesTable();
    bool createRelationsTable();

    void saveTriples(qint64 recordId, const QList<Triple>& triples);
    QList<Triple> loadTriples(qint64 recordId);
    
    // 错误处理和重试机制
    DatabaseErrorType classifyDatabaseError(const QString& errorMessage);
    bool retryInsertExtractionRecords(const QList<ExtractionRecord>& records, int maxRetries = 3);
    
    // 带重试的查询执行
    bool executeWithRetry(const std::function<bool()>& operation, int maxRetries = 3, 
                         const QString& operationName = QString());

    QSqlDatabase db_;
    QString databasePath_;
};

} // namespace optikg