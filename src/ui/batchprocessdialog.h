#pragma once

#include <QDialog>
#include <QList>
#include "core/batchprocessor.h"

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
class QProgressBar;
class QLabel;
class QTextEdit;
class QCheckBox;
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

namespace optikg {

/**
 * @brief 批量处理对话框
 * 
 * 功能：
 * - 添加/移除文件
 * - 显示处理进度
 * - 实时显示处理结果
 * - 支持导出结果
 */
class BatchProcessDialog : public QDialog {
    Q_OBJECT

public:
    explicit BatchProcessDialog(QWidget *parent = nullptr);
    ~BatchProcessDialog() override;

protected:
    /**
     * @brief 关闭事件处理
     * @param event 关闭事件
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * @brief 扫描进度更新
     */
    void onScanProgress(int currentFile, int totalFiles, const QString& fileName);
    
    /**
     * @brief 扫描完成
     */
    void onScanFinished(int totalFiles, int totalRecords);
    
    /**
     * @brief 添加文件
     */
    void onAddFilesClicked();

    /**
     * @brief 添加目录
     */
    void onAddDirectoryClicked();

    /**
     * @brief 移除选中的文件
     */
    void onRemoveFilesClicked();

    /**
     * @brief 清空文件列表
     */
    void onClearFilesClicked();

    /**
     * @brief 开始批量处理
     */
    void onStartClicked();

    /**
     * @brief 停止处理
     */
    void onStopClicked();

    /**
     * @brief 导出结果
     */
    void onExportClicked();

    /**
     * @brief 处理进度更新
     * @param current 当前处理序号
     * @param total 总数
     * @param currentFile 当前文件名
     */
    void onProgressChanged(int current, int total, const QString& currentFile);

    /**
     * @brief 单个文件处理完成
     * @param result 处理结果
     */
    void onFileProcessed(const optikg::FileProcessResult& result);

    /**
     * @brief 处理出错
     * @param filePath 文件路径
     * @param error 错误信息
     */
    void onErrorOccurred(const QString& filePath, const QString& error);

    /**
     * @brief 批量处理完成
     * @param totalFiles 总文件数
     * @param successCount 成功数
     * @param failCount 失败数
     */
    void onBatchFinished(int totalFiles, int successCount, int failCount);

    /**
     * @brief 文件列表选择变化
     */
    void onFileSelectionChanged();

    /**
     * @brief 自动导出选项切换
     */
    void onAutoExportToggled(bool checked);

    /**
     * @brief 保存到数据库选项切换
     */
    void onSaveToDbToggled(bool checked);

private:
    /**
     * @brief 设置 UI 布局
     */
    void setupUi();

    /**
     * @brief 更新按钮状态
     */
    void updateButtonStates();

    /**
     * @brief 更新统计信息
     */
    void updateStatistics();

    /**
     * @brief 导出结果为 JSON
     * @param filePath 输出文件路径
     */
    void exportToJson(const QString& filePath);

    /**
     * @brief 导出结果为 CSV
     * @param filePath 输出文件路径
     */
    void exportToCsv(const QString& filePath);

    /**
     * @brief 获取输出文件路径
     * @param format 导出格式
     * @return QString 文件路径，取消返回空字符串
     */
    QString getOutputFilePath(const QString& format);
    void saveToDatabase();

    // UI 组件
    QListWidget* fileList_ = nullptr;           ///< 文件列表
    QPushButton* addFilesBtn_ = nullptr;        ///< 添加文件按钮
    QPushButton* addDirBtn_ = nullptr;          ///< 添加目录按钮
    QPushButton* removeBtn_ = nullptr;          ///< 移除按钮
    QPushButton* clearBtn_ = nullptr;           ///< 清空按钮
    QPushButton* startBtn_ = nullptr;           ///< 开始按钮
    QPushButton* stopBtn_ = nullptr;            ///< 停止按钮
    QPushButton* exportBtn_ = nullptr;          ///< 导出按钮
    QPushButton* closeBtn_ = nullptr;           ///< 关闭按钮

    // 进度和状态
    QProgressBar* progressBar_ = nullptr;       ///< 进度条
    QLabel* statusLabel_ = nullptr;             ///< 状态标签
    QLabel* statsLabel_ = nullptr;              ///< 统计标签
    QTextEdit* logEdit_ = nullptr;              ///< 日志显示

    // 选项
    QCheckBox* autoExportCheck_ = nullptr;      ///< 自动导出选项
    QCheckBox* saveToDbCheck_ = nullptr;        ///< 保存到数据库选项
    QComboBox* exportFormatCombo_ = nullptr;    ///< 导出格式选择
    QLineEdit* outputPathEdit_ = nullptr;       ///< 输出路径
    QPushButton* browseOutputBtn_ = nullptr;    ///< 浏览输出按钮

    // 处理器
    BatchProcessor* processor_ = nullptr;       ///< 批量处理器

    // 状态
    bool isProcessing_ = false;                 ///< 是否正在处理
    int totalFiles_ = 0;                        ///< 总文件数
    int processedFiles_ = 0;                    ///< 已处理文件数
    int successCount_ = 0;                      ///< 成功数
    int failCount_ = 0;                         ///< 失败数
    int totalTriples_ = 0;                      ///< 提取的三元组总数
    
    // 结果
    QList<FileProcessResult> allResults_;       ///< 所有处理结果
};

} // namespace optikg
