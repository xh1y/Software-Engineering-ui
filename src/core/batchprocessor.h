#pragma once

#include <QThread>
#include <QString>
#include <QStringList>
#include <QList>
#include "optikg/datamodel.h"

namespace optikg {

// 前向声明
class InferenceEngine;

/**
 * @brief 单个文件的处理结果
 */
struct FileProcessResult {
    QString filePath;           ///< 文件路径
    bool success = false;       ///< 处理是否成功
    QString errorMessage;       ///< 错误信息（失败时）
    QList<Triple> triples;      ///< 提取的三元组结果（合并所有文本）
    int processTimeMs = 0;      ///< 处理耗时（毫秒）
    int recordCount = 0;        ///< 处理的记录数

    // 新增：保存每个文本内容的抽取记录
    QList<ExtractionRecord> extractionRecords; ///< 每个文本内容的抽取记录
    
    FileProcessResult() = default;
    explicit FileProcessResult(const QString& path) : filePath(path) {}
};

/**
 * @brief 批量处理器 - 后台线程处理多个文件
 * 
 * 功能：
 * - 批量处理 JSON/CSV 文件
 * - 支持字段映射配置
 * - 实时进度报告
 * - 可取消操作
 */
class BatchProcessor : public QThread {
    Q_OBJECT

public:
    explicit BatchProcessor(QObject *parent = nullptr);
    ~BatchProcessor() override;

    /**
     * @brief 设置要处理的文件列表
     * @param files 文件路径列表
     */
    void setFiles(const QStringList& files);

    /**
     * @brief 设置 JSON 内容字段名
     * @param fieldName 字段名
     */
    void setJsonContentField(const QString& fieldName);

    /**
     * @brief 设置 CSV 内容列名
     * @param columnName 列名
     */
    void setCsvContentColumn(const QString& columnName);

    /**
     * @brief 设置 CSV 文件编码
     * @param encoding 编码名称
     */
    void setCsvEncoding(const QString& encoding);

    /**
     * @brief 设置置信度阈值
     * @param threshold 阈值
     */
    void setThreshold(float threshold);

    /**
     * @brief 设置模型路径
     * @param path 模型文件路径
     */
    void setModelPath(const QString& path);

    /**
     * @brief 请求停止处理
     */
    void stop();

    /**
     * @brief 获取处理结果
     * @return QList<FileProcessResult> 所有文件的处理结果
     */
    QList<FileProcessResult> getResults() const;

signals:
    /**
     * @brief 整体进度变化（基于记录数）
     * @param currentRecord 当前处理的记录索引
     * @param totalRecords 总记录数
     * @param currentFile 当前处理的文件名
     */
    void progressChanged(int currentRecord, int totalRecords, const QString& currentFile);
    
    /**
     * @brief 扫描进度（预处理阶段）
     * @param currentFile 当前扫描的文件索引
     * @param totalFiles 总文件数
     * @param fileName 文件名
     */
    void scanProgress(int currentFile, int totalFiles, const QString& fileName);
    
    /**
     * @brief 扫描完成，报告总记录数
     * @param totalFiles 总文件数
     * @param totalRecords 总记录数
     */
    void scanFinished(int totalFiles, int totalRecords);

    /**
     * @brief 单个文件处理完成
     * @param result 处理结果
     */
    void fileProcessed(const optikg::FileProcessResult& result);

    /**
     * @brief 处理出错
     * @param filePath 出错的文件路径
     * @param error 错误信息
     */
    void errorOccurred(const QString& filePath, const QString& error);

    /**
     * @brief 所有文件处理完成
     * @param totalFiles 总文件数
     * @param successCount 成功数
     * @param failCount 失败数
     */
    void batchFinished(int totalFiles, int successCount, int failCount);

protected:
    void run() override;

private:
    // ========== 扫描函数（用于统计记录数）==========
    int scanJsonFile(const QString& filePath);
    int scanCsvFile(const QString& filePath);

    // ========== 处理函数（带进度跟踪）==========
    FileProcessResult processJsonFileWithProgress(const QString& filePath,
                                                   int& processedRecords, int totalRecords);
    FileProcessResult processCsvFileWithProgress(const QString& filePath,
                                                  int& processedRecords, int totalRecords);

    QStringList files_;                 ///< 待处理的文件列表
    QString jsonContentField_;          ///< JSON 内容字段名
    QString csvContentColumn_;          ///< CSV 内容列名
    QString csvEncoding_;               ///< CSV 编码
    QString modelPath_;                 ///< 模型路径
    float threshold_ = -10.0f;          ///< 置信度阈值
    
    bool stopRequested_ = false;        ///< 停止请求标志
    QList<FileProcessResult> results_;  ///< 处理结果列表
};

} // namespace optikg

// 声明为元类型，以便在信号槽中使用
Q_DECLARE_METATYPE(optikg::FileProcessResult)
