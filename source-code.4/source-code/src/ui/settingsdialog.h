#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QLineEdit;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

namespace optikg {

/**
 * @brief 设置对话框
 * 
 * 提供以下配置选项：
 * - 字段映射：JSON 内容字段名、CSV 内容列名、CSV 编码
 * - 模型设置：模型路径、置信度阈值
 * - 批量处理：输出目录、自动导出、默认导出格式
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

private slots:
    /**
     * @brief 应用设置变更
     */
    void onApplyClicked();

    /**
     * @brief 确认并关闭对话框
     */
    void onOkClicked();

    /**
     * @brief 取消并关闭对话框
     */
    void onCancelClicked();

    /**
     * @brief 恢复默认设置
     */
    void onRestoreDefaultsClicked();

    /**
     * @brief 浏览选择模型文件
     */
    void onBrowseModelClicked();

    /**
     * @brief 浏览选择输出目录
     */
    void onBrowseOutputDirClicked();

private:
    /**
     * @brief 设置 UI 布局
     */
    void setupUi();

    /**
     * @brief 创建设置页面
     */
    QWidget* createFieldMappingPage();
    QWidget* createModelPage();
    QWidget* createBatchPage();

    /**
     * @brief 从配置加载设置到 UI
     */
    void loadSettings();

    /**
     * @brief 保存 UI 设置到配置
     */
    void saveSettings();

    // UI 组件 - 字段映射页
    QLineEdit* jsonFieldEdit_ = nullptr;        ///< JSON 内容字段名
    QLineEdit* csvColumnEdit_ = nullptr;        ///< CSV 内容列名
    QComboBox* csvEncodingCombo_ = nullptr;     ///< CSV 编码选择

    // UI 组件 - 模型设置页
    QLineEdit* modelPathEdit_ = nullptr;        ///< 模型路径
    QPushButton* browseModelBtn_ = nullptr;     ///< 浏览模型按钮
    QDoubleSpinBox* thresholdSpin_ = nullptr;   ///< 置信度阈值

    // UI 组件 - 批量处理页
    QLineEdit* outputDirEdit_ = nullptr;        ///< 输出目录
    QPushButton* browseOutputBtn_ = nullptr;    ///< 浏览输出目录按钮
    QCheckBox* autoExportCheck_ = nullptr;      ///< 自动导出选项
    QComboBox* exportFormatCombo_ = nullptr;    ///< 导出格式选择

    // 按钮
    QPushButton* applyBtn_ = nullptr;
    QPushButton* okBtn_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
    QPushButton* restoreBtn_ = nullptr;

    // 标签页
    QTabWidget* tabWidget_ = nullptr;

    // 是否有未保存的更改
    bool hasChanges_ = false;
};

} // namespace optikg
