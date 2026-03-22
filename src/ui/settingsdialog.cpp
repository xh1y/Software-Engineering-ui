#include "settingsdialog.h"

#include "../utils/configmanager.h"
#include "../utils/appconstants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

namespace optikg {

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("设置"));
    setMinimumSize(500, 400);
    
    setupUi();
    loadSettings();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // 创建标签页
    tabWidget_ = new QTabWidget(this);
    tabWidget_->addTab(createFieldMappingPage(), tr("字段映射"));
    tabWidget_->addTab(createModelPage(), tr("模型设置"));
    tabWidget_->addTab(createBatchPage(), tr("批量处理"));
    mainLayout->addWidget(tabWidget_);

    // 按钮区域
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    restoreBtn_ = new QPushButton(tr("恢复默认"), this);
    connect(restoreBtn_, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaultsClicked);
    buttonLayout->addWidget(restoreBtn_);

    buttonLayout->addSpacing(20);

    applyBtn_ = new QPushButton(tr("应用"), this);
    connect(applyBtn_, &QPushButton::clicked, this, &SettingsDialog::onApplyClicked);
    buttonLayout->addWidget(applyBtn_);

    okBtn_ = new QPushButton(tr("确定"), this);
    okBtn_->setDefault(true);
    connect(okBtn_, &QPushButton::clicked, this, &SettingsDialog::onOkClicked);
    buttonLayout->addWidget(okBtn_);

    cancelBtn_ = new QPushButton(tr("取消"), this);
    connect(cancelBtn_, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);
    buttonLayout->addWidget(cancelBtn_);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

QWidget* SettingsDialog::createFieldMappingPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    // 说明标签
    auto* infoLabel = new QLabel(
        tr("配置数据文件的字段映射关系。\n"
           "系统将根据这些配置从 JSON/CSV 文件中提取内容。"),
        page);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: gray; margin-bottom: 10px; }");
    layout->addWidget(infoLabel);

    // JSON 设置组
    auto* jsonGroup = new QGroupBox(tr("JSON 文件设置"), page);
    auto* jsonLayout = new QGridLayout(jsonGroup);

    jsonLayout->addWidget(new QLabel(tr("内容字段名:"), jsonGroup), 0, 0);
    jsonFieldEdit_ = new QLineEdit(jsonGroup);
    jsonFieldEdit_->setPlaceholderText(tr("例如: text, content, body"));
    jsonLayout->addWidget(jsonFieldEdit_, 0, 1);

    auto* jsonHelp = new QLabel(
        tr("支持嵌套对象，如: data.content"),
        jsonGroup);
    jsonHelp->setStyleSheet("QLabel { color: gray; font-size: 11px; }");
    jsonLayout->addWidget(jsonHelp, 1, 1);

    layout->addWidget(jsonGroup);

    // CSV 设置组
    auto* csvGroup = new QGroupBox(tr("CSV 文件设置"), page);
    auto* csvLayout = new QGridLayout(csvGroup);

    csvLayout->addWidget(new QLabel(tr("内容列名:"), csvGroup), 0, 0);
    csvColumnEdit_ = new QLineEdit(csvGroup);
    csvColumnEdit_->setPlaceholderText(tr("例如: content, text, description"));
    csvLayout->addWidget(csvColumnEdit_, 0, 1);

    csvLayout->addWidget(new QLabel(tr("文件编码:"), csvGroup), 1, 0);
    csvEncodingCombo_ = new QComboBox(csvGroup);
    csvEncodingCombo_->addItems(AppConstants::File::getCsvEncodings());
    csvEncodingCombo_->setEditable(true);
    csvLayout->addWidget(csvEncodingCombo_, 1, 1);

    layout->addWidget(csvGroup);
    layout->addStretch();

    return page;
}

QWidget* SettingsDialog::createModelPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    // 说明标签
    auto* infoLabel = new QLabel(
        tr("配置知识抽取模型的相关参数。"),
        page);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: gray; margin-bottom: 10px; }");
    layout->addWidget(infoLabel);

    // 模型设置组
    auto* modelGroup = new QGroupBox(tr("模型配置"), page);
    auto* modelLayout = new QGridLayout(modelGroup);

    modelLayout->addWidget(new QLabel(tr("模型路径:"), modelGroup), 0, 0);
    auto* pathLayout = new QHBoxLayout();
    modelPathEdit_ = new QLineEdit(modelGroup);
    modelPathEdit_->setPlaceholderText(tr("选择 ONNX 模型文件路径"));
    pathLayout->addWidget(modelPathEdit_);
    browseModelBtn_ = new QPushButton(tr("浏览..."), modelGroup);
    connect(browseModelBtn_, &QPushButton::clicked, this, &SettingsDialog::onBrowseModelClicked);
    pathLayout->addWidget(browseModelBtn_);
    modelLayout->addLayout(pathLayout, 0, 1);

    modelLayout->addWidget(new QLabel(tr("置信度阈值:"), modelGroup), 1, 0);
    thresholdSpin_ = new QDoubleSpinBox(modelGroup);
    thresholdSpin_->setRange(AppConstants::Model::THRESHOLD_MIN, AppConstants::Model::THRESHOLD_MAX);
    thresholdSpin_->setDecimals(2);
    thresholdSpin_->setSingleStep(AppConstants::Model::THRESHOLD_STEP);
    thresholdSpin_->setToolTip(tr("阈值越高，过滤越严格"));
    modelLayout->addWidget(thresholdSpin_, 1, 1);

    auto* thresholdHelp = new QLabel(
        tr("默认值: %1，值越高过滤越严格").arg(AppConstants::Model::DEFAULT_THRESHOLD),
        modelGroup);
    thresholdHelp->setStyleSheet("QLabel { color: gray; font-size: 11px; }");
    modelLayout->addWidget(thresholdHelp, 2, 1);

    layout->addWidget(modelGroup);
    layout->addStretch();

    return page;
}

QWidget* SettingsDialog::createBatchPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    // 说明标签
    auto* infoLabel = new QLabel(
        tr("配置批量处理功能的默认行为。"),
        page);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: gray; margin-bottom: 10px; }");
    layout->addWidget(infoLabel);

    // 输出设置组
    auto* outputGroup = new QGroupBox(tr("输出设置"), page);
    auto* outputLayout = new QGridLayout(outputGroup);

    outputLayout->addWidget(new QLabel(tr("默认输出目录:"), outputGroup), 0, 0);
    auto* dirLayout = new QHBoxLayout();
    outputDirEdit_ = new QLineEdit(outputGroup);
    outputDirEdit_->setPlaceholderText(tr("选择批量处理结果输出目录"));
    dirLayout->addWidget(outputDirEdit_);
    browseOutputBtn_ = new QPushButton(tr("浏览..."), outputGroup);
    connect(browseOutputBtn_, &QPushButton::clicked, this, &SettingsDialog::onBrowseOutputDirClicked);
    dirLayout->addWidget(browseOutputBtn_);
    outputLayout->addLayout(dirLayout, 0, 1);

    outputLayout->addWidget(new QLabel(tr("默认导出格式:"), outputGroup), 1, 0);
    exportFormatCombo_ = new QComboBox(outputGroup);
    exportFormatCombo_->addItems({"JSON", "CSV"});
    outputLayout->addWidget(exportFormatCombo_, 1, 1);

    layout->addWidget(outputGroup);

    // 行为设置组
    auto* behaviorGroup = new QGroupBox(tr("行为设置"), page);
    auto* behaviorLayout = new QVBoxLayout(behaviorGroup);

    autoExportCheck_ = new QCheckBox(tr("处理完成后自动导出结果"), behaviorGroup);
    autoExportCheck_->setToolTip(tr("批量处理完成后自动保存结果到输出目录"));
    behaviorLayout->addWidget(autoExportCheck_);

    layout->addWidget(behaviorGroup);
    layout->addStretch();

    return page;
}

void SettingsDialog::loadSettings() {
    ConfigManager& config = ConfigManager::instance();

    // 字段映射
    jsonFieldEdit_->setText(config.getJsonContentField());
    csvColumnEdit_->setText(config.getCsvContentColumn());
    
    QString encoding = config.getCsvEncoding();
    int encodingIndex = csvEncodingCombo_->findText(encoding, Qt::MatchFixedString);
    if (encodingIndex >= 0) {
        csvEncodingCombo_->setCurrentIndex(encodingIndex);
    } else {
        csvEncodingCombo_->setCurrentText(encoding);
    }

    // 模型设置
    modelPathEdit_->setText(config.getModelPath());
    thresholdSpin_->setValue(config.getThreshold());

    // 批量处理
    outputDirEdit_->setText(config.getBatchOutputDir());
    autoExportCheck_->setChecked(config.getAutoExport());
    
    QString format = config.getDefaultExportFormat().toUpper();
    int formatIndex = exportFormatCombo_->findText(format);
    if (formatIndex >= 0) {
        exportFormatCombo_->setCurrentIndex(formatIndex);
    }

    hasChanges_ = false;
}

void SettingsDialog::saveSettings() {
    ConfigManager& config = ConfigManager::instance();

    // 字段映射
    config.setJsonContentField(jsonFieldEdit_->text().trimmed());
    config.setCsvContentColumn(csvColumnEdit_->text().trimmed());
    config.setCsvEncoding(csvEncodingCombo_->currentText());

    // 模型设置
    config.setModelPath(modelPathEdit_->text().trimmed());
    config.setThreshold(static_cast<float>(thresholdSpin_->value()));

    // 批量处理
    config.setBatchOutputDir(outputDirEdit_->text().trimmed());
    config.setAutoExport(autoExportCheck_->isChecked());
    config.setDefaultExportFormat(exportFormatCombo_->currentText().toLower());

    config.saveSettings();
    hasChanges_ = false;
}

void SettingsDialog::onApplyClicked() {
    saveSettings();
    QApplication::beep();
}

void SettingsDialog::onOkClicked() {
    saveSettings();
    accept();
}

void SettingsDialog::onCancelClicked() {
    if (hasChanges_) {
        auto reply = QMessageBox::question(this, tr("确认取消"),
            tr("有未保存的更改，确定要取消吗？"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    reject();
}

void SettingsDialog::onRestoreDefaultsClicked() {
    auto reply = QMessageBox::question(this, tr("恢复默认设置"),
        tr("确定要恢复所有设置到默认值吗？"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }

    // 恢复默认值
    jsonFieldEdit_->setText(AppConstants::Mapping::DEFAULT_JSON_FIELD);
    csvColumnEdit_->setText(AppConstants::Mapping::DEFAULT_CSV_COLUMN);
    csvEncodingCombo_->setCurrentText(AppConstants::Mapping::DEFAULT_CSV_ENCODING);
    modelPathEdit_->clear();
    thresholdSpin_->setValue(AppConstants::Model::DEFAULT_THRESHOLD);
    outputDirEdit_->clear();
    autoExportCheck_->setChecked(false);
    exportFormatCombo_->setCurrentText(AppConstants::Mapping::DEFAULT_EXPORT_FORMAT);

    hasChanges_ = true;
}

void SettingsDialog::onBrowseModelClicked() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择模型文件"),
        QString(),
        tr("ONNX 模型 (*.onnx);;所有文件 (*.*)"));
    
    if (!fileName.isEmpty()) {
        modelPathEdit_->setText(fileName);
        hasChanges_ = true;
    }
}

void SettingsDialog::onBrowseOutputDirClicked() {
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("选择输出目录"),
        outputDirEdit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        outputDirEdit_->setText(dir);
        hasChanges_ = true;
    }
}

} // namespace optikg
