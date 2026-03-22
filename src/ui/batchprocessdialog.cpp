#include "batchprocessdialog.h"

#include "../utils/configmanager.h"
#include "../data/databasemanager.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QTextEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTextStream>
#include <QHeaderView>

namespace optikg {

BatchProcessDialog::BatchProcessDialog(QWidget *parent)
    : QDialog(parent)
    , isProcessing_(false)
    , totalFiles_(0)
    , processedFiles_(0)
    , successCount_(0)
    , failCount_(0)
    , totalTriples_(0) {
    
    setWindowTitle(tr("批量处理"));
    setMinimumSize(700, 600);
    resize(800, 700);
    
    setupUi();
    updateButtonStates();
    
    // 加载配置
    ConfigManager& config = ConfigManager::instance();
    autoExportCheck_->setChecked(config.getAutoExport());
    saveToDbCheck_->setChecked(config.getSaveToDatabase());
    QString format = config.getDefaultExportFormat();
    exportFormatCombo_->setCurrentText(format.toUpper());

    // 连接配置变更信号
    connect(autoExportCheck_, &QCheckBox::toggled, this, &BatchProcessDialog::onAutoExportToggled);
    connect(saveToDbCheck_, &QCheckBox::toggled, this, &BatchProcessDialog::onSaveToDbToggled);
}

BatchProcessDialog::~BatchProcessDialog() {
    if (processor_ && processor_->isRunning()) {
        processor_->stop();
        processor_->wait();
    }
    delete processor_;
}

void BatchProcessDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // === 文件列表区域 ===
    auto* listGroup = new QGroupBox(tr("文件列表"), this);
    auto* listLayout = new QVBoxLayout(listGroup);

    fileList_ = new QListWidget(listGroup);
    fileList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(fileList_, &QListWidget::itemSelectionChanged,
            this, &BatchProcessDialog::onFileSelectionChanged);
    listLayout->addWidget(fileList_);

    // 文件列表按钮
    auto* listBtnLayout = new QHBoxLayout();
    addFilesBtn_ = new QPushButton(tr("添加文件..."), listGroup);
    connect(addFilesBtn_, &QPushButton::clicked, this, &BatchProcessDialog::onAddFilesClicked);
    listBtnLayout->addWidget(addFilesBtn_);

    addDirBtn_ = new QPushButton(tr("添加目录..."), listGroup);
    connect(addDirBtn_, &QPushButton::clicked, this, &BatchProcessDialog::onAddDirectoryClicked);
    listBtnLayout->addWidget(addDirBtn_);

    listBtnLayout->addStretch();

    removeBtn_ = new QPushButton(tr("移除选中"), listGroup);
    removeBtn_->setEnabled(false);
    connect(removeBtn_, &QPushButton::clicked, this, &BatchProcessDialog::onRemoveFilesClicked);
    listBtnLayout->addWidget(removeBtn_);

    clearBtn_ = new QPushButton(tr("清空列表"), listGroup);
    connect(clearBtn_, &QPushButton::clicked, this, &BatchProcessDialog::onClearFilesClicked);
    listBtnLayout->addWidget(clearBtn_);

    listLayout->addLayout(listBtnLayout);
    mainLayout->addWidget(listGroup, 2); // 占据 2/5 空间

    // === 选项区域 ===
    auto* optionsGroup = new QGroupBox(tr("处理选项"), this);
    auto* optionsLayout = new QGridLayout(optionsGroup);

    autoExportCheck_ = new QCheckBox(tr("处理完成后自动导出结果"), optionsGroup);
    optionsLayout->addWidget(autoExportCheck_, 0, 0, 1, 2);

    saveToDbCheck_ = new QCheckBox(tr("保存到数据库"), optionsGroup);
    optionsLayout->addWidget(saveToDbCheck_, 1, 0, 1, 2);

    optionsLayout->addWidget(new QLabel(tr("导出格式:"), optionsGroup), 2, 0);
    exportFormatCombo_ = new QComboBox(optionsGroup);
    exportFormatCombo_->addItems({"JSON", "CSV"});
    optionsLayout->addWidget(exportFormatCombo_, 2, 1);

    optionsLayout->addWidget(new QLabel(tr("输出路径:"), optionsGroup), 3, 0);
    auto* outputLayout = new QHBoxLayout();
    outputPathEdit_ = new QLineEdit(optionsGroup);
    outputPathEdit_->setPlaceholderText(tr("可选，默认使用配置中的输出目录"));
    outputLayout->addWidget(outputPathEdit_);
    browseOutputBtn_ = new QPushButton(tr("浏览..."), optionsGroup);
    connect(browseOutputBtn_, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this,
            tr("选择输出文件"),
            outputPathEdit_->text(),
            tr("JSON (*.json);;CSV (*.csv);;所有文件 (*.*)"));
        if (!fileName.isEmpty()) {
            outputPathEdit_->setText(fileName);
            // 根据选择的文件类型更新格式
            if (fileName.endsWith(".csv", Qt::CaseInsensitive)) {
                exportFormatCombo_->setCurrentText("CSV");
            } else if (fileName.endsWith(".json", Qt::CaseInsensitive)) {
                exportFormatCombo_->setCurrentText("JSON");
            }
        }
    });
    outputLayout->addWidget(browseOutputBtn_);
    optionsLayout->addLayout(outputLayout, 3, 1);

    optionsLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(optionsGroup);

    // === 进度和状态区域 ===
    auto* statusGroup = new QGroupBox(tr("处理状态"), this);
    auto* statusLayout = new QVBoxLayout(statusGroup);

    // 进度条
    progressBar_ = new QProgressBar(statusGroup);
    progressBar_->setRange(0, 100);
    progressBar_->setTextVisible(true);
    progressBar_->setFormat(tr("%v / %m (%p%)"));
    statusLayout->addWidget(progressBar_);

    // 状态标签
    statusLabel_ = new QLabel(tr("就绪"), statusGroup);
    statusLayout->addWidget(statusLabel_);

    // 统计标签
    statsLabel_ = new QLabel(tr("文件: 0 | 成功: 0 | 失败: 0 | 三元组: 0"), statusGroup);
    statusLayout->addWidget(statsLabel_);

    // 日志显示
    logEdit_ = new QTextEdit(statusGroup);
    logEdit_->setReadOnly(true);
    // TODO: 限制最大行数（需要设置 QTextDocument 的 maximumBlockCount）
    logEdit_->setPlaceholderText(tr("处理日志将显示在这里..."));
    statusLayout->addWidget(logEdit_);

    mainLayout->addWidget(statusGroup, 2);

    // === 底部按钮 ===
    auto* bottomLayout = new QHBoxLayout();
    
    startBtn_ = new QPushButton(tr("开始处理"), this);
    startBtn_->setStyleSheet("QPushButton { font-weight: bold; }");
    connect(startBtn_, &QPushButton::clicked, this, &BatchProcessDialog::onStartClicked);
    bottomLayout->addWidget(startBtn_);

    stopBtn_ = new QPushButton(tr("停止"), this);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &BatchProcessDialog::onStopClicked);
    bottomLayout->addWidget(stopBtn_);

    bottomLayout->addStretch();

    exportBtn_ = new QPushButton(tr("导出结果..."), this);
    exportBtn_->setEnabled(false);
    connect(exportBtn_, &QPushButton::clicked, this, &BatchProcessDialog::onExportClicked);
    bottomLayout->addWidget(exportBtn_);

    closeBtn_ = new QPushButton(tr("关闭"), this);
    connect(closeBtn_, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn_);

    mainLayout->addLayout(bottomLayout);
}

void BatchProcessDialog::updateButtonStates() {
    bool hasFiles = fileList_->count() > 0;
    
    addFilesBtn_->setEnabled(!isProcessing_);
    addDirBtn_->setEnabled(!isProcessing_);
    removeBtn_->setEnabled(!isProcessing_ && !fileList_->selectedItems().isEmpty());
    clearBtn_->setEnabled(!isProcessing_ && hasFiles);
    startBtn_->setEnabled(!isProcessing_ && hasFiles);
    stopBtn_->setEnabled(isProcessing_);
    exportBtn_->setEnabled(!isProcessing_ && !allResults_.isEmpty());
    browseOutputBtn_->setEnabled(!isProcessing_);
}

void BatchProcessDialog::updateStatistics() {
    statsLabel_->setText(tr("文件: %1 | 成功: %2 | 失败: %3 | 三元组: %4")
        .arg(totalFiles_)
        .arg(successCount_)
        .arg(failCount_)
        .arg(totalTriples_));
}

void BatchProcessDialog::onAddFilesClicked() {
    QStringList files = QFileDialog::getOpenFileNames(this,
        tr("选择要处理的文件"),
        QString(),
        tr("数据文件 (*.json *.csv);;JSON 文件 (*.json);;CSV 文件 (*.csv);;所有文件 (*.*)"));
    
    for (const QString& file : files) {
        // 检查是否已存在
        bool exists = false;
        for (int i = 0; i < fileList_->count(); ++i) {
            if (fileList_->item(i)->text() == file) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            fileList_->addItem(file);
        }
    }
    
    updateButtonStates();
}

void BatchProcessDialog::onAddDirectoryClicked() {
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("选择包含数据文件的目录"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (dir.isEmpty()) {
        return;
    }
    
    // 递归查找 JSON 和 CSV 文件
    QDir directory(dir);
    QStringList filters;
    filters << "*.json" << "*.csv";
    directory.setNameFilters(filters);
    
    QFileInfoList files = directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    
    // TODO: 添加递归子目录选项
    
    for (const QFileInfo& fileInfo : files) {
        QString filePath = fileInfo.absoluteFilePath();
        // 检查是否已存在
        bool exists = false;
        for (int i = 0; i < fileList_->count(); ++i) {
            if (fileList_->item(i)->text() == filePath) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            fileList_->addItem(filePath);
        }
    }
    
    updateButtonStates();
    
    if (files.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("所选目录中没有找到 JSON 或 CSV 文件。"));
    } else {
        logEdit_->append(tr("已从目录添加 %1 个文件: %2").arg(files.size()).arg(dir));
    }
}

void BatchProcessDialog::onRemoveFilesClicked() {
    for (QListWidgetItem* item : fileList_->selectedItems()) {
        delete fileList_->takeItem(fileList_->row(item));
    }
    updateButtonStates();
}

void BatchProcessDialog::onClearFilesClicked() {
    fileList_->clear();
    updateButtonStates();
}

void BatchProcessDialog::onFileSelectionChanged() {
    removeBtn_->setEnabled(!isProcessing_ && !fileList_->selectedItems().isEmpty());
}

void BatchProcessDialog::onStartClicked() {
    if (fileList_->count() == 0) {
        QMessageBox::warning(this, tr("警告"), tr("请先添加要处理的文件。"));
        return;
    }

    // 收集文件列表
    QStringList files;
    for (int i = 0; i < fileList_->count(); ++i) {
        files.append(fileList_->item(i)->text());
    }

    // 重置状态
    totalFiles_ = files.size();
    processedFiles_ = 0;
    successCount_ = 0;
    failCount_ = 0;
    totalTriples_ = 0;
    allResults_.clear();
    logEdit_->clear();

    // 进度条范围将在扫描完成后根据总记录数设置
    progressBar_->setRange(0, 0); // 初始不确定范围
    progressBar_->setValue(0);
    
    isProcessing_ = true;
    updateButtonStates();
    updateStatistics();

    // 创建并配置处理器
    processor_ = new BatchProcessor(this);
    processor_->setFiles(files);
    
    // 从配置读取设置
    ConfigManager& config = ConfigManager::instance();
    processor_->setJsonContentField(config.getJsonContentField());
    processor_->setCsvContentColumn(config.getCsvContentColumn());
    processor_->setCsvEncoding(config.getCsvEncoding());
    processor_->setThreshold(config.getThreshold());
    processor_->setModelPath(config.getModelPath());

    // 连接信号
    connect(processor_, &BatchProcessor::scanProgress,
            this, &BatchProcessDialog::onScanProgress);
    connect(processor_, &BatchProcessor::scanFinished,
            this, &BatchProcessDialog::onScanFinished);
    connect(processor_, &BatchProcessor::progressChanged,
            this, &BatchProcessDialog::onProgressChanged);
    connect(processor_, &BatchProcessor::fileProcessed,
            this, &BatchProcessDialog::onFileProcessed);
    connect(processor_, &BatchProcessor::errorOccurred,
            this, &BatchProcessDialog::onErrorOccurred);
    connect(processor_, &BatchProcessor::batchFinished,
            this, &BatchProcessDialog::onBatchFinished);

    // 启动处理
    processor_->start();
    statusLabel_->setText(tr("正在处理..."));
    logEdit_->append(tr("开始批量处理，共 %1 个文件").arg(totalFiles_));
}

void BatchProcessDialog::onStopClicked() {
    if (processor_ && processor_->isRunning()) {
        processor_->stop();
        statusLabel_->setText(tr("正在停止..."));
        logEdit_->append(tr("用户请求停止处理"));
    }
}

void BatchProcessDialog::onScanProgress(int currentFile, int totalFiles, const QString& fileName) {
    statusLabel_->setText(tr("正在扫描文件 (%1/%2): %3").arg(currentFile).arg(totalFiles).arg(fileName));
    // 扫描阶段不更新进度条，等待扫描完成后设置总记录数
}

void BatchProcessDialog::onScanFinished(int totalFiles, int totalRecords) {
    progressBar_->setRange(0, totalRecords);
    progressBar_->setValue(0);
    statusLabel_->setText(tr("扫描完成: %1 个文件, %2 条记录，开始处理...").arg(totalFiles).arg(totalRecords));
    logEdit_->append(tr("扫描完成: 共 %1 个文件，%2 条记录").arg(totalFiles).arg(totalRecords));

    // 如果总记录数为0，警告用户
    if (totalRecords == 0) {
        logEdit_->append(tr("⚠ 警告: 未找到任何记录，请检查文件格式和内容字段设置"));
    }
}

void BatchProcessDialog::onProgressChanged(int current, int total, const QString& currentFile) {
    progressBar_->setValue(current);
    QString fileName = QFileInfo(currentFile).fileName();
    statusLabel_->setText(tr("正在处理 (%1/%2): %3").arg(current).arg(total).arg(fileName));
}

void BatchProcessDialog::onFileProcessed(const FileProcessResult& result) {
    processedFiles_++;
    
    if (result.success) {
        successCount_++;
        totalTriples_ += result.triples.size();
        
        QString fileName = QFileInfo(result.filePath).fileName();
        logEdit_->append(tr("✓ %1: 成功，记录 %2 条，提取 %3 个三元组 (%4 ms)")
            .arg(fileName)
            .arg(result.recordCount)  // 显示记录条数！
            .arg(result.triples.size())
            .arg(result.processTimeMs));
        
        // 如果没有提取到三元组，给出具体原因
        if (result.triples.isEmpty() && result.recordCount > 0) {
            // 获取当前模型路径信息
            ConfigManager& config = ConfigManager::instance();
            QString modelPath = config.getModelPath();
            
            if (modelPath.isEmpty()) {
                logEdit_->append(tr("  ⚠ 警告: 未找到模型文件！请在 工具→设置→模型设置 中配置模型路径"));
            } else {
                logEdit_->append(tr("  ⚠ 提示: 处理了 %1 条记录但未提取到三元组，可能是:\n"
                                    "     - 文本内容与模型训练数据不匹配\n"
                                    "     - 置信度阈值过高（当前: %2）\n"
                                    "     - 模型推理异常")
                    .arg(result.recordCount)
                    .arg(config.getThreshold()));
            }
        }
    }
    
    allResults_.append(result);
    updateStatistics();
}

void BatchProcessDialog::onErrorOccurred(const QString& filePath, const QString& error) {
    failCount_++;
    QString fileName = QFileInfo(filePath).fileName();
    logEdit_->append(tr("✗ %1: 失败 - %2").arg(fileName).arg(error));
    updateStatistics();
}

void BatchProcessDialog::onBatchFinished(int totalFiles, int successCount, int failCount) {
    isProcessing_ = false;
    updateButtonStates();

    statusLabel_->setText(tr("处理完成"));
    logEdit_->append(tr("\n批量处理完成：总计 %1，成功 %2，失败 %3")
        .arg(totalFiles).arg(successCount).arg(failCount));

    // 自动导出
    if (autoExportCheck_->isChecked() && successCount > 0) {
        QString outputPath = outputPathEdit_->text();
        
        // 如果未指定输出路径，使用配置中的默认目录
        if (outputPath.isEmpty()) {
            ConfigManager& config = ConfigManager::instance();
            QString outputDir = config.getBatchOutputDir();
            if (outputDir.isEmpty()) {
                outputDir = QDir::homePath();
            }
            
            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            QString format = exportFormatCombo_->currentText().toLower();
            outputPath = QDir(outputDir).filePath(
                QString("batch_result_%1.%2").arg(timestamp).arg(format));
        }

        // 执行导出
        if (outputPath.endsWith(".csv", Qt::CaseInsensitive)) {
            exportToCsv(outputPath);
        } else {
            exportToJson(outputPath);
        }
        
        logEdit_->append(tr("自动导出到: %1").arg(outputPath));
    }

    // 保存到数据库
    if (saveToDbCheck_->isChecked() && successCount > 0) {
        saveToDatabase();
    }

    // 显示完成提示
    QMessageBox::information(this, tr("处理完成"),
        tr("批量处理已完成！\n\n总计: %1 个文件\n成功: %2\n失败: %3\n提取三元组: %4")
        .arg(totalFiles).arg(successCount).arg(failCount).arg(totalTriples_));
}

void BatchProcessDialog::onExportClicked() {
    QString format = exportFormatCombo_->currentText();
    QString filePath = getOutputFilePath(format);
    
    if (filePath.isEmpty()) {
        return; // 用户取消
    }

    if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
        exportToCsv(filePath);
    } else {
        exportToJson(filePath);
    }
}

QString BatchProcessDialog::getOutputFilePath(const QString& format) {
    QString filter = format == "CSV" 
        ? tr("CSV 文件 (*.csv)") 
        : tr("JSON 文件 (*.json)");
    
    QString defaultPath = outputPathEdit_->text();
    if (defaultPath.isEmpty()) {
        ConfigManager& config = ConfigManager::instance();
        QString outputDir = config.getBatchOutputDir();
        if (outputDir.isEmpty()) {
            outputDir = QDir::homePath();
        }
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        defaultPath = QDir(outputDir).filePath(
            QString("batch_result_%1.%2").arg(timestamp).arg(format.toLower()));
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("导出结果"),
        defaultPath,
        filter + tr(";;所有文件 (*.*)"));
    
    return fileName;
}

void BatchProcessDialog::exportToJson(const QString& filePath) {
    QJsonObject rootObj;
    rootObj["export_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObj["total_files"] = totalFiles_;
    rootObj["success_count"] = successCount_;
    rootObj["fail_count"] = failCount_;
    rootObj["total_triples"] = totalTriples_;

    QJsonArray resultsArray;
    for (const auto& result : allResults_) {
        if (!result.success) {
            continue; // 跳过失败的
        }

        QJsonObject resultObj;
        resultObj["file_path"] = result.filePath;
        resultObj["file_name"] = QFileInfo(result.filePath).fileName();
        resultObj["process_time_ms"] = result.processTimeMs;
        resultObj["record_count"] = result.recordCount;
        resultObj["triple_count"] = result.triples.size();

        QJsonArray triplesArray;
        for (const auto& triple : result.triples) {
            QJsonObject tripleObj;
            
            QJsonObject subjectObj;
            subjectObj["name"] = triple.subject.name;
            subjectObj["type"] = Entity::typeToString(triple.subject.type);
            subjectObj["confidence"] = triple.subject.confidence;
            tripleObj["subject"] = subjectObj;

            QJsonObject objectObj;
            objectObj["name"] = triple.object.name;
            objectObj["type"] = Entity::typeToString(triple.object.type);
            objectObj["confidence"] = triple.object.confidence;
            tripleObj["object"] = objectObj;

            tripleObj["relation"] = triple.relation;
            tripleObj["relation_type"] = Triple::relationTypeToString(triple.relationType);
            tripleObj["confidence"] = triple.confidence;

            triplesArray.append(tripleObj);
        }
        resultObj["triples"] = triplesArray;
        resultsArray.append(resultObj);
    }
    rootObj["results"] = resultsArray;

    QJsonDocument doc(rootObj);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("错误"),
            tr("无法写入文件: %1").arg(file.errorString()));
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    logEdit_->append(tr("结果已导出到: %1").arg(filePath));
    QMessageBox::information(this, tr("导出成功"),
        tr("结果已成功导出到:\n%1").arg(filePath));
}

void BatchProcessDialog::exportToCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("错误"),
            tr("无法写入文件: %1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    
    // 写入 CSV 表头
    stream << "source_file,record_index,subject_name,subject_type,subject_confidence,"
           << "relation,relation_type,confidence,"
           << "object_name,object_type,object_confidence\n";

    int recordIndex = 0;
    for (const auto& result : allResults_) {
        if (!result.success) {
            continue;
        }

        QString fileName = QFileInfo(result.filePath).fileName();
        
        for (const auto& triple : result.triples) {
            stream << "\"" << fileName << "\","
                   << recordIndex++ << ","
                   << "\"" << triple.subject.name << "\","
                   << Entity::typeToString(triple.subject.type) << ","
                   << triple.subject.confidence << ","
                   << "\"" << triple.relation << "\","
                   << Triple::relationTypeToString(triple.relationType) << ","
                   << triple.confidence << ","
                   << "\"" << triple.object.name << "\","
                   << Entity::typeToString(triple.object.type) << ","
                   << triple.object.confidence << "\n";
        }
    }

    file.close();

    logEdit_->append(tr("结果已导出到: %1").arg(filePath));
    QMessageBox::information(this, tr("导出成功"),
        tr("结果已成功导出到:\n%1").arg(filePath));
}

void BatchProcessDialog::closeEvent(QCloseEvent *event) {
    if (isProcessing_) {
        auto reply = QMessageBox::question(this, tr("确认关闭"),
            tr("正在处理中，确定要关闭吗？这将取消当前处理。"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            onStopClicked();
            if (processor_) {
                processor_->wait(3000); // 等待最多 3 秒
            }
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void BatchProcessDialog::saveToDatabase() {
    // 收集所有抽取记录
    QList<ExtractionRecord> allRecords;

    for (const FileProcessResult& result : allResults_) {
        if (!result.success) {
            continue;
        }

        // 添加该文件的所有抽取记录
        allRecords.append(result.extractionRecords);
    }

    if (allRecords.isEmpty()) {
        logEdit_->append(tr("没有可保存的抽取记录"));
        return;
    }

    // 保存到数据库
    DatabaseManager& db = DatabaseManager::instance();
    bool success = db.insertExtractionRecords(allRecords);

    if (success) {
        logEdit_->append(tr("成功保存 %1 条抽取记录到数据库").arg(allRecords.size()));
    } else {
        logEdit_->append(tr("保存到数据库失败"));
    }
}

void BatchProcessDialog::onAutoExportToggled(bool checked) {
    ConfigManager::instance().setAutoExport(checked);
}

void BatchProcessDialog::onSaveToDbToggled(bool checked) {
    ConfigManager::instance().setSaveToDatabase(checked);
}

} // namespace optikg
