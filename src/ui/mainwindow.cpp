#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../utils/appconstants.h"
#include "../utils/stylemanager.h"
#include "../utils/logger.h"

#include "optikg/datamodel.h"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QMessageBox>
#include <QApplication>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDateTime>
#include <QTimer>
#include <QActionGroup>
#include <QInputDialog>
#include <QSet>

// 自定义组件头文件
#include "extractionpanel.h"
#include "graphwidget.h"
#include "historypanel.h"
#include "resulttreewidget.h"
#include "core/inferenceworker.h"
#include "../data/databasemanager.h"
#include "../utils/configmanager.h"
#include "batchprocessdialog.h"
#include "settingsdialog.h"

namespace optikg {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , extractionPanel_(nullptr)
    , graphWidget_(nullptr)
    , historyPanel_(nullptr)
    , resultTree_(nullptr)
    , statusBar_(nullptr)
    , statusLabel_(nullptr)
    , modeLabel_(nullptr)
    , thresholdLabel_(nullptr)
    , countLabel_(nullptr)
    , progressBar_(nullptr)
    , currentWorker_(nullptr) {
    ui->setupUi(this);

    setupUi();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connectSignals();

    // 初始化配置管理器
    ConfigManager::instance().initialize();
    
    loadSettings();

    // 初始化数据库
    if (!DatabaseManager::instance().initialize()) {
        showToast(tr("数据库初始化失败"), true);
    }

    // 加载历史记录
    if (historyPanel_) {
        historyPanel_->loadHistory();
    }

    setWindowTitle(tr("OptiKG - 工业知识抽取系统"));
    resize(AppConstants::UI::WINDOW_WIDTH, AppConstants::UI::WINDOW_HEIGHT);
}

MainWindow::~MainWindow() {
    saveSettings();

    if (currentWorker_) {
        currentWorker_->quit();
        currentWorker_->wait();
    }

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings();
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // 快捷键处理将在setupShortcuts中配置
    QMainWindow::keyPressEvent(event);
}

void MainWindow::setupUi() {
    // 创建中心部件和分割器
    QWidget* centralWidget = new QWidget(this);
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    QSplitter* leftSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    QSplitter* rightSplitter = new QSplitter(Qt::Vertical, mainSplitter);

    // 创建自定义组件
    extractionPanel_ = new ExtractionPanel(leftSplitter);
    graphWidget_ = new GraphWidget(rightSplitter);
    historyPanel_ = new HistoryPanel(leftSplitter);
    resultTree_ = new ResultTreeWidget(rightSplitter);

    // 将组件添加到分割器
    leftSplitter->addWidget(extractionPanel_);
    leftSplitter->addWidget(historyPanel_);
    rightSplitter->addWidget(graphWidget_);
    rightSplitter->addWidget(resultTree_);

    // 设置分割器比例
    leftSplitter->setStretchFactor(0, 3);  // 抽取面板占3/4
    leftSplitter->setStretchFactor(1, 1);  // 历史记录占1/4
    rightSplitter->setStretchFactor(0, 2); // 知识图谱占2/3
    rightSplitter->setStretchFactor(1, 1); // 结果列表占1/3

    mainSplitter->addWidget(leftSplitter);
    mainSplitter->addWidget(rightSplitter);

    // 设置中心布局
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->addWidget(mainSplitter);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    QAction* newAction = fileMenu->addAction(tr("新建(&N)"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onClearClicked);

    QAction* openResultAction = fileMenu->addAction(tr("打开之前的结果(&O)..."));
    openResultAction->setShortcut(QKeySequence::Open);
    connect(openResultAction, &QAction::triggered, this, &MainWindow::onOpenClicked);

    QAction* openTextAction = fileMenu->addAction(tr("打开纯文本文件开始抽取(&T)..."));
    openTextAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(openTextAction, &QAction::triggered, this, &MainWindow::onOpenTextFileClicked);

    fileMenu->addSeparator();
    QAction* saveAction = fileMenu->addAction(tr("保存(&S)"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveClicked);

    QAction* exportJsonAction = fileMenu->addAction(tr("导出为JSON(&J)"));
    connect(exportJsonAction, &QAction::triggered, this, &MainWindow::onExportJsonClicked);

    QAction* exportCsvAction = fileMenu->addAction(tr("导出为CSV(&C)"));
    connect(exportCsvAction, &QAction::triggered, this, &MainWindow::onExportCsvClicked);

    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction(tr("退出(&X)"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu* editMenu = menuBar()->addMenu(tr("编辑(&E)"));
    QAction* extractAction = editMenu->addAction(tr("抽取(&E)"));
    extractAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(extractAction, &QAction::triggered, this, &MainWindow::onExtractClicked);

    QAction* clearAction = editMenu->addAction(tr("清空(&C)"));
    clearAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    connect(clearAction, &QAction::triggered, this, &MainWindow::onClearClicked);

    QMenu* viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    // 缩放操作
    QAction* zoomInAction = viewMenu->addAction(tr("放大(&I)"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, [this]() {
        if (graphWidget_) graphWidget_->zoomIn();
    });

    QAction* zoomOutAction = viewMenu->addAction(tr("缩小(&O)"));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, [this]() {
        if (graphWidget_) graphWidget_->zoomOut();
    });

    QAction* fitViewAction = viewMenu->addAction(tr("适应视图(&F)"));
    fitViewAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(fitViewAction, &QAction::triggered, this, [this]() {
        if (graphWidget_) graphWidget_->fitToView();
    });

    QAction* resetZoomAction = viewMenu->addAction(tr("重置缩放(&R)"));
    resetZoomAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(resetZoomAction, &QAction::triggered, this, [this]() {
        if (graphWidget_) graphWidget_->resetZoom();
    });

    viewMenu->addSeparator();
    // 面板可见性
    QAction* toggleGraphAction = viewMenu->addAction(tr("显示知识图谱(&G)"));
    toggleGraphAction->setCheckable(true);
    toggleGraphAction->setChecked(true);
    connect(toggleGraphAction, &QAction::toggled, this, [this](bool checked) {
        if (graphWidget_) graphWidget_->setVisible(checked);
    });

    QAction* toggleTreeAction = viewMenu->addAction(tr("显示结果列表(&L)"));
    toggleTreeAction->setCheckable(true);
    toggleTreeAction->setChecked(true);
    connect(toggleTreeAction, &QAction::toggled, this, [this](bool checked) {
        if (resultTree_) resultTree_->setVisible(checked);
    });

    QAction* toggleHistoryAction = viewMenu->addAction(tr("显示历史记录(&H)"));
    toggleHistoryAction->setCheckable(true);
    toggleHistoryAction->setChecked(true);
    connect(toggleHistoryAction, &QAction::toggled, this, [this](bool checked) {
        if (historyPanel_) historyPanel_->setVisible(checked);
    });

    viewMenu->addSeparator();
    // 布局操作
    QAction* resetLayoutAction = viewMenu->addAction(tr("重置布局(&E)"));
    resetLayoutAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(resetLayoutAction, &QAction::triggered, this, [this]() {
        // 恢复默认分割器比例
        QList<QSplitter*> splitters = findChildren<QSplitter*>();
        for (auto splitter : splitters) {
            QList<int> sizes;
            for (int i = 0; i < splitter->count(); ++i) {
                sizes.append(100); // 默认大小
            }
            splitter->setSizes(sizes);
        }
    });

    QMenu* toolsMenu = menuBar()->addMenu(tr("工具(&T)"));
    QAction* batchAction = toolsMenu->addAction(tr("批量处理(&B)..."));
    batchAction->setShortcut(QKeySequence("Ctrl+B"));
    connect(batchAction, &QAction::triggered, this, &MainWindow::onBatchProcessClicked);

    QAction* settingsAction = toolsMenu->addAction(tr("设置(&S)..."));
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);

    QMenu* helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    QAction* helpAction = helpMenu->addAction(tr("使用说明(&U)..."));
    helpAction->setShortcut(QKeySequence::HelpContents);
    connect(helpAction, &QAction::triggered, this, &MainWindow::onHelpClicked);

    helpMenu->addSeparator();
    QAction* aboutAction = helpMenu->addAction(tr("关于(&A)..."));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAboutClicked);
}

void MainWindow::setupToolBar() {
    QToolBar* toolBar = addToolBar(tr("工具栏"));
    toolBar->setMovable(false);

    // 抽取按钮 - 使用标准 QPushButton，样式由 QSS 控制
    QPushButton* extractButton = new QPushButton(tr("抽取"));
    extractButton->setObjectName("extractBtn");
    extractButton->setToolTip(tr("抽取 (Ctrl+E)"));
    toolBar->addWidget(extractButton);
    connect(extractButton, &QPushButton::clicked, this, &MainWindow::onExtractClicked);

    // 清空按钮 - 使用标准 QPushButton，样式由 QSS 控制
    QPushButton* clearButton = new QPushButton(tr("清空"));
    clearButton->setObjectName("clearBtn");
    clearButton->setToolTip(tr("清空 (Ctrl+Shift+C)"));
    toolBar->addWidget(clearButton);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);

    toolBar->addSeparator();

    // 放大按钮
    QAction* zoomInAction = toolBar->addAction(tr("放大"));
    zoomInAction->setIcon(QIcon::fromTheme("zoom-in"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, [this]() {
        if (graphWidget_) graphWidget_->zoomIn();
    });

    // 缩小按钮
    QAction* zoomOutAction = toolBar->addAction(tr("缩小"));
    zoomOutAction->setIcon(QIcon::fromTheme("zoom-out"));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, [this]() {
        if (graphWidget_) graphWidget_->zoomOut();
    });

    // 适应视图按钮
    QAction* fitViewAction = toolBar->addAction(tr("适应视图"));
    fitViewAction->setIcon(QIcon::fromTheme("zoom-fit-best"));
    fitViewAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(fitViewAction, &QAction::triggered, this, [this]() {
        if (graphWidget_) graphWidget_->fitToView();
    });

    toolBar->addSeparator();

    // 设置按钮
    QAction* settingsAction = toolBar->addAction(tr("设置"));
    settingsAction->setIcon(QIcon::fromTheme("preferences-system"));
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);

    // 批量处理按钮
    QAction* batchAction = toolBar->addAction(tr("批量处理"));
    batchAction->setIcon(QIcon::fromTheme("document-multiple"));
    batchAction->setShortcut(QKeySequence("Ctrl+B"));
    connect(batchAction, &QAction::triggered, this, &MainWindow::onBatchProcessClicked);

    // 帮助按钮
    QAction* helpAction = toolBar->addAction(tr("帮助"));
    helpAction->setIcon(QIcon::fromTheme("help-contents"));
    connect(helpAction, &QAction::triggered, this, &MainWindow::onHelpClicked);


}

void MainWindow::setupStatusBar() {
    statusBar_ = statusBar();
    statusLabel_ = new QLabel(tr("就绪"));
    statusBar_->addWidget(statusLabel_);

    modeLabel_ = new QLabel(tr("模式: CPU"));
    statusBar_->addPermanentWidget(modeLabel_);

    thresholdLabel_ = new QLabel(tr("阈值: -10.0"));
    statusBar_->addPermanentWidget(thresholdLabel_);

    countLabel_ = new QLabel(tr("实体: 0 关系: 0"));
    statusBar_->addPermanentWidget(countLabel_);



    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);
    progressBar_->setTextVisible(true);
    progressBar_->setVisible(false);
    statusBar_->addPermanentWidget(progressBar_);
}

void MainWindow::connectSignals() {
    // 连接抽取面板的信号
    connect(extractionPanel_, &ExtractionPanel::extractClicked,
            this, &MainWindow::onExtractClicked);
    connect(extractionPanel_, &ExtractionPanel::clearClicked,
            this, &MainWindow::onClearClicked);

    // 连接历史记录面板的信号
    connect(historyPanel_, &HistoryPanel::recordClicked,
            this, [this](qint64 id) {
                // 加载记录详情
                loadRecordDetails(id);
            });
    connect(historyPanel_, &HistoryPanel::recordDoubleClicked,
            this, [this](qint64 id) {
                // 加载记录详情
                loadRecordDetails(id);
            });
    connect(historyPanel_, &HistoryPanel::exportClicked,
            this, [this](const QList<qint64>& ids) {
                onExportJsonClicked(); // 暂用导出JSON
            });
    connect(historyPanel_, &HistoryPanel::batchDeleteClicked,
            this, [this](const QList<qint64>& ids) {
                onBatchDeleteRecords(ids);
            });

    // 连接数据库变化信号
    connect(&DatabaseManager::instance(), &DatabaseManager::databaseChanged,
            historyPanel_, &HistoryPanel::loadHistory);

    // 连接结果树的信号
    connect(resultTree_, &ResultTreeWidget::entityClicked,
            this, [this](const QString& name, const QString& type) {
                if (graphWidget_) {
                    graphWidget_->highlightNode(name);
                }
                showToast(tr("实体: %1 (%2)").arg(name, type));
            });
    connect(resultTree_, &ResultTreeWidget::tripleClicked,
            this, [this](const Triple& triple) {
                if (graphWidget_) {
                    graphWidget_->highlightPath(triple.subject.name, triple.object.name);
                }
                showToast(tr("关系: %1 → %2").arg(triple.subject.name, triple.object.name));
            });
}

void MainWindow::loadSettings() {
    // 从配置管理器加载设置
    ConfigManager& config = ConfigManager::instance();
    
    // 恢复窗口几何
    QByteArray geometry = config.getWindowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    
    // 更新状态栏显示
    thresholdLabel_->setText(tr("阈值: %1").arg(config.getThreshold()));
}

void MainWindow::saveSettings() {
    // 保存窗口几何到配置管理器
    ConfigManager& config = ConfigManager::instance();
    config.setWindowGeometry(saveGeometry());
    config.saveSettings();
}

void MainWindow::showToast(const QString& message, bool isError) {
    // 使用状态栏显示通知
    if (!statusLabel_) return;

        // 保存当前文本和样式
        QString currentText = statusLabel_->text();
        QString currentStyle = statusLabel_->styleSheet();

        // 设置新消息
        statusLabel_->setText(message);

        // 设置颜色样式
        if (isError) {
            statusLabel_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(AppConstants::UI::ERROR_COLOR));
        } else {
            statusLabel_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(AppConstants::UI::SUCCESS_COLOR));
        }

        // 3秒后恢复原始状态
        QTimer::singleShot(AppConstants::UI::TOAST_TIMEOUT_MS, this, [this, currentText, currentStyle]() {
            if (statusLabel_) {
                statusLabel_->setText(currentText);
                statusLabel_->setStyleSheet(currentStyle);
            }
        });
}

// ============ 槽函数实现 ============

void MainWindow::onExtractClicked() {
    if (!extractionPanel_) {
        showToast(tr("抽取面板未初始化"), true);
        return;
    }

    QString text = extractionPanel_->text();
    if (text.isEmpty()) {
        showToast(tr("请输入文本内容"), false);
        return;
    }

    // 如果已有工作线程在运行，先停止
    if (currentWorker_ && currentWorker_->isRunning()) {
        currentWorker_->stop();
        currentWorker_->quit();
        currentWorker_->wait();
        delete currentWorker_;
        currentWorker_ = nullptr;
    }

    // 创建新的推理工作线程
    currentWorker_ = new InferenceWorker(this);
    currentWorker_->setText(text);

    // 使用配置管理器获取模型路径
    ConfigManager& config = ConfigManager::instance();
    QString modelPath = config.getModelPath();

    // 如果配置中没有模型路径，自动检测
    if (modelPath.isEmpty() || !QFile::exists(modelPath)) {
        modelPath = config.autoDetectModelPath();
        qDebug() << "Auto-detected model path:" << modelPath;
    }


    qDebug() << "Model path:" << modelPath;
    currentWorker_->setModelPath(modelPath);

    // 使用配置中的阈值
    float threshold = config.getThreshold();
    currentWorker_->setThreshold(threshold);
    qDebug() << "Using threshold:" << threshold;

    // 连接信号与槽
    connect(currentWorker_, &InferenceWorker::progressChanged,
            this, &MainWindow::onProgressChanged);
    connect(currentWorker_, &InferenceWorker::resultReady,
            this, &MainWindow::onExtractionFinished);
    connect(currentWorker_, &InferenceWorker::errorOccurred,
            this, &MainWindow::onExtractionError);
    connect(currentWorker_, &InferenceWorker::finished,
            currentWorker_, &QObject::deleteLater);
    connect(currentWorker_, &InferenceWorker::finished, this, [this]() {
        currentWorker_ = nullptr;
    });

    // 启动抽取过程
    onExtractionStarted();
    currentWorker_->start();
}

void MainWindow::onClearClicked() {
    if (extractionPanel_) {
        extractionPanel_->clear();
    }
    if (resultTree_) {
        resultTree_->clearResults();
    }
    if (graphWidget_) {
        graphWidget_->clearGraph();
    }

    // 清空当前结果
    currentResults_.clear();

    // 重置计数
    countLabel_->setText(tr("实体: 0 关系: 0"));
    statusLabel_->setText(tr("已清空"));
}

void MainWindow::onSaveClicked() {
    if (currentResults_.isEmpty()) {
        showToast(tr("没有可保存的抽取结果"), true);
        return;
    }

    QString text = extractionPanel_ ? extractionPanel_->text() : QString();
    ExtractionRecord record(text, currentResults_, 0);
    qint64 recordId = DatabaseManager::instance().insertExtractionRecord(record);

    if (recordId > 0) {
        showToast(tr("已保存 %1 个实体和 %2 个关系到数据库")
            .arg(record.entityCount).arg(record.relationCount), false);
        if (historyPanel_) {
            historyPanel_->loadHistory();
        }
    } else {
        showToast(tr("保存到数据库失败"), true);
    }
}

void MainWindow::onBatchProcessClicked() {
    // 打开批量处理对话框
    BatchProcessDialog dialog(this);
    dialog.exec();
}

void MainWindow::onExportJsonClicked() {
    if (currentResults_.isEmpty()) {
        showToast(tr("没有可导出的数据"), false);
        return;
    }

    // 选择保存文件路径
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("导出JSON文件"), "",
        tr("JSON文件 (*.json);;所有文件 (*)"));

    if (fileName.isEmpty()) {
        return; // 用户取消
    }

    // 确保文件扩展名为.json
    if (!fileName.endsWith(".json", Qt::CaseInsensitive)) {
        fileName += ".json";
    }

    // 构建JSON文档
    QJsonArray triplesArray;
    for (const auto& triple : currentResults_) {
        QJsonObject tripleObj;

        // 主语实体
        QJsonObject subjectObj;
        subjectObj["name"] = triple.subject.name;
        subjectObj["type"] = Entity::typeToString(triple.subject.type);
        subjectObj["confidence"] = triple.subject.confidence;
        tripleObj["subject"] = subjectObj;

        // 宾语实体
        QJsonObject objectObj;
        objectObj["name"] = triple.object.name;
        objectObj["type"] = Entity::typeToString(triple.object.type);
        objectObj["confidence"] = triple.object.confidence;
        tripleObj["object"] = objectObj;

        // 关系
        tripleObj["relation"] = triple.relation;
        tripleObj["relation_type"] = Triple::relationTypeToString(triple.relationType);
        tripleObj["confidence"] = triple.confidence;

        triplesArray.append(tripleObj);
    }

    // 创建根对象
    QJsonObject rootObj;
    rootObj["version"] = "1.0";
    rootObj["export_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObj["triple_count"] = currentResults_.size();
    rootObj["triples"] = triplesArray;

    QJsonDocument doc(rootObj);

    // 写入文件
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        showToast(tr("无法写入文件: %1").arg(file.errorString()), true);
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    showToast(tr("成功导出 %1 个三元组到 %2").arg(currentResults_.size()).arg(fileName), false);
}

void MainWindow::onExportCsvClicked() {
    if (currentResults_.isEmpty()) {
        showToast(tr("没有可导出的数据"), false);
        return;
    }

    // 选择保存文件路径
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("导出CSV文件"), "",
        tr("CSV文件 (*.csv);;所有文件 (*)"));

    if (fileName.isEmpty()) {
        return; // 用户取消
    }

    // 确保文件扩展名为.csv
    if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
        fileName += ".csv";
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        showToast(tr("无法写入文件: %1").arg(file.errorString()), true);
        return;
    }

    QTextStream stream(&file);
    // stream.setEncoding(QStringConverter::Utf8); // Qt6默认使用UTF-8

    // CSV转义函数
    auto escapeCsvField = [](const QString& field) -> QString {
        // 如果字段包含逗号、双引号或换行符，需要用双引号包裹
        if (field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r')) {
            // 将双引号替换为两个双引号
            QString escaped = field;
            escaped.replace("\"", "\"\"");
            return "\"" + escaped + "\"";
        }
        return field;
    };

    // 写入表头
    stream << escapeCsvField("subject_name") << ","
           << escapeCsvField("subject_type") << ","
           << escapeCsvField("relation") << ","
           << escapeCsvField("relation_type") << ","
           << escapeCsvField("object_name") << ","
           << escapeCsvField("object_type") << ","
           << escapeCsvField("confidence") << ","
           << escapeCsvField("subject_confidence") << ","
           << escapeCsvField("object_confidence") << "\n";

    // 写入数据行
    for (const auto& triple : currentResults_) {
        stream << escapeCsvField(triple.subject.name) << ","
               << escapeCsvField(Entity::typeToString(triple.subject.type)) << ","
               << escapeCsvField(triple.relation) << ","
               << escapeCsvField(Triple::relationTypeToString(triple.relationType)) << ","
               << escapeCsvField(triple.object.name) << ","
               << escapeCsvField(Entity::typeToString(triple.object.type)) << ","
               << escapeCsvField(QString::number(triple.confidence, 'f', 4)) << ","
               << escapeCsvField(QString::number(triple.subject.confidence, 'f', 4)) << ","
               << escapeCsvField(QString::number(triple.object.confidence, 'f', 4)) << "\n";
    }

    file.close();

    showToast(tr("成功导出 %1 个三元组到 %2").arg(currentResults_.size()).arg(fileName), false);
}

void MainWindow::onSettingsClicked() {
    // 打开设置对话框
    SettingsDialog dialog(this);
    dialog.exec();
    
    // 设置可能已更改，更新状态栏显示
    ConfigManager& config = ConfigManager::instance();
    thresholdLabel_->setText(tr("阈值: %1").arg(config.getThreshold()));
}

void MainWindow::onExtractionStarted() {
    extractionTimer_.restart();

    progressBar_->setVisible(true);
    progressBar_->setValue(0);
    statusLabel_->setText(tr("正在抽取..."));

    // 禁用抽取面板，防止重复点击
    if (extractionPanel_) {
        extractionPanel_->setEnabled(false);
    }
}

void MainWindow::onExtractionFinished(const QList<optikg::Triple>& results) {
    progressBar_->setVisible(false);
    statusLabel_->setText(tr("抽取完成"));

    int processTimeMs = extractionTimer_.elapsed();

    // 保存到数据库
    if (!results.isEmpty()) {
        QString text = extractionPanel_ ? extractionPanel_->text() : "";
        ExtractionRecord record(text, results, processTimeMs);
        qint64 recordId = DatabaseManager::instance().insertExtractionRecord(record);

        if (recordId > 0) {
            qDebug() << "Record saved with ID:" << recordId;
            // 刷新历史记录面板
            if (historyPanel_) {
                historyPanel_->loadHistory();
            }
        }
    }

    // 保存当前结果
    currentResults_ = results;

    // 更新结果树
    if (resultTree_) {
        resultTree_->setResults(results);
    }

    // 更新知识图谱
    if (graphWidget_) {
        // 将triples转换为图数据
        QList<optikg::GraphNode> nodes;
        QList<optikg::GraphEdge> edges;
        QHash<QString, optikg::GraphNode> nodeMap; // 实体名 -> 节点
        QSet<QPair<QString, QString>> edgeSet; // 用于边去重

        // 收集所有唯一实体和边
        for (const auto& triple : results) {
            QString subjectKey = triple.subject.name + "_" + QString::number(static_cast<int>(triple.subject.type));
            QString objectKey = triple.object.name + "_" + QString::number(static_cast<int>(triple.object.type));

            if (!nodeMap.contains(subjectKey)) {
                optikg::GraphNode node(triple.subject);
                node.id = subjectKey;
                nodes.append(node);
                nodeMap.insert(subjectKey, node);
            }
            if (!nodeMap.contains(objectKey)) {
                optikg::GraphNode node(triple.object);
                node.id = objectKey;
                nodes.append(node);
                nodeMap.insert(objectKey, node);
            }

            // 边去重
            QPair<QString, QString> edgeKey = qMakePair(subjectKey, objectKey);
            if (!edgeSet.contains(edgeKey)) {
                edgeSet.insert(edgeKey);
                edges.append(optikg::GraphEdge(subjectKey, objectKey, triple.relation, triple.confidence));
            }
        }

        graphWidget_->setGraphData(nodes, edges);
    }

    // 更新状态栏计数
    countLabel_->setText(tr("实体: %1 关系: %2").arg(results.size() * 2).arg(results.size()));

    // 重新启用抽取面板
    if (extractionPanel_) {
        extractionPanel_->setEnabled(true);
    }

    showToast(tr("抽取完成，共 %1 个三元组").arg(results.count()), false);
}

void MainWindow::onExtractionError(const QString& error) {
    progressBar_->setVisible(false);
    statusLabel_->setText(tr("抽取失败"));

    // 重新启用抽取面板
    if (extractionPanel_) {
        extractionPanel_->setEnabled(true);
    }

    showToast(tr("抽取失败: %1").arg(error), true);
}

void MainWindow::onProgressChanged(int value) {
    progressBar_->setValue(value);
}

void MainWindow::onBatchDeleteRecords(const QList<qint64>& ids) {
    if (ids.isEmpty()) {
        return;
    }

    if (DatabaseManager::instance().deleteMultipleRecords(ids)) {
        showToast(tr("已删除 %1 条记录").arg(ids.size()), false);
        // 数据库变化信号会触发historyPanel刷新
    } else {
        showToast(tr("删除记录失败"), true);
    }
}

void MainWindow::onOpenClicked() {
    Logger::info("[onOpenClicked] 打开文件对话框");
    
    // 选择JSON文件导入
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("打开JSON文件"), "",
        tr("JSON文件 (*.json);;所有文件 (*)"));

    if (fileName.isEmpty()) {
        Logger::info("[onOpenClicked] 用户取消选择文件");
        return;
    }
    
    Logger::info(QString("[onOpenClicked] 选择的文件: %1").arg(fileName));

    QFile file(fileName);
    Logger::info(QString("[onOpenClicked] 尝试打开文件"));
    if (!file.open(QIODevice::ReadOnly)) {
        QString errorMsg = file.errorString();
        Logger::error(QString("[onOpenClicked] 无法打开文件: %1").arg(errorMsg));
        showToast(tr("无法打开文件: %1").arg(errorMsg), true);
        return;
    }
    
    Logger::info(QString("[onOpenClicked] 文件打开成功，开始读取数据"));
    QByteArray data = file.readAll();
    Logger::info(QString("[onOpenClicked] 文件大小: %1 字节").arg(data.size()));
    file.close();

    Logger::info("[onOpenClicked] 开始解析JSON");
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = parseError.errorString();
        Logger::error(QString("[onOpenClicked] JSON解析错误: %1").arg(errorMsg));
        showToast(tr("JSON解析错误: %1").arg(errorMsg), true);
        return;
    }
    Logger::info("[onOpenClicked] JSON解析成功");

    if (!doc.isObject()) {
        Logger::error("[onOpenClicked] JSON格式错误：根元素不是对象");
        showToast(tr("JSON格式错误：根元素不是对象"), true);
        return;
    }

    Logger::info("[onOpenClicked] 开始提取三元组数据");
    QJsonObject rootObj = doc.object();
    QJsonArray triplesArray;

    // 首先尝试直接获取triples字段
    if (rootObj.contains("triples")) {
        triplesArray = rootObj["triples"].toArray();
        Logger::info(QString("[onOpenClicked] 从'triples'字段提取到 %1 个三元组").arg(triplesArray.size()));
    }

    // 如果是批量导出的格式 {"results": [{"triples": [...]}, ...]}
    if (triplesArray.isEmpty() && rootObj.contains("results")) {
        Logger::info("[onOpenClicked] 尝试从'results'字段提取");
        QJsonArray resultsArray = rootObj["results"].toArray();
        for (const QJsonValue& resultValue : resultsArray) {
            if (resultValue.isObject()) {
                QJsonObject resultObj = resultValue.toObject();
                if (resultObj.contains("triples")) {
                    QJsonArray resultTriples = resultObj["triples"].toArray();
                    // 合并所有triples
                    for (const QJsonValue& tripleValue : resultTriples) {
                        triplesArray.append(tripleValue);
                    }
                }
            }
        }
        Logger::info(QString("[onOpenClicked] 从'results'字段提取到 %1 个三元组").arg(triplesArray.size()));
    }

    if (triplesArray.isEmpty()) {
        Logger::warning("[onOpenClicked] 文件中没有找到三元组数据");
        showToast(tr("文件中没有找到三元组数据"), false);
        return;
    }

    Logger::info(QString("[onOpenClicked] 开始解析 %1 个三元组").arg(triplesArray.size()));
    QList<optikg::Triple> importedTriples;
    int parseErrorCount = 0;
    for (int i = 0; i < triplesArray.size(); ++i) {
        const QJsonValue& value = triplesArray[i];
        QJsonObject tripleObj = value.toObject();
        
        if (tripleObj.isEmpty()) {
            Logger::warning(QString("[onOpenClicked] 第 %1 个三元组为空对象").arg(i));
            parseErrorCount++;
            continue;
        }

        // 解析主语
        QJsonObject subjectObj = tripleObj["subject"].toObject();
        if (subjectObj.isEmpty()) {
            Logger::warning(QString("[onOpenClicked] 第 %1 个三元组缺少主语").arg(i));
            parseErrorCount++;
            continue;
        }
        
        optikg::Entity subject;
        subject.name = subjectObj["name"].toString();
        subject.type = Entity::stringToType(subjectObj["type"].toString());
        subject.confidence = subjectObj["confidence"].toDouble();

        // 解析宾语
        QJsonObject objectObj = tripleObj["object"].toObject();
        if (objectObj.isEmpty()) {
            Logger::warning(QString("[onOpenClicked] 第 %1 个三元组缺少宾语").arg(i));
            parseErrorCount++;
            continue;
        }
        
        optikg::Entity object;
        object.name = objectObj["name"].toString();
        object.type = Entity::stringToType(objectObj["type"].toString());
        object.confidence = objectObj["confidence"].toDouble();

        // 解析关系
        optikg::Triple triple;
        triple.subject = subject;
        triple.object = object;
        triple.relation = tripleObj["relation"].toString();
        triple.relationType = Triple::stringToRelationType(tripleObj["relation_type"].toString());
        triple.confidence = tripleObj["confidence"].toDouble();

        importedTriples.append(triple);
    }
    
    Logger::info(QString("[onOpenClicked] 成功解析 %1 个三元组，失败 %2 个").arg(importedTriples.size()).arg(parseErrorCount));

    // 更新当前结果
    Logger::info("[onOpenClicked] 开始更新UI");
    currentResults_ = importedTriples;

    // 更新结果树
    if (resultTree_) {
        Logger::info("[onOpenClicked] 更新结果树");
        resultTree_->setResults(importedTriples);
    } else {
        Logger::warning("[onOpenClicked] resultTree_ 为空");
    }

    // 更新知识图谱
    if (graphWidget_) {
        // 将triples转换为图数据
        QList<optikg::GraphNode> nodes;
        QList<optikg::GraphEdge> edges;
        QHash<QString, optikg::GraphNode> nodeMap; // 实体名 -> 节点
        QSet<QPair<QString, QString>> edgeSet; // 用于边去重

        // 收集所有唯一实体和边
        for (const auto& triple : importedTriples) {
            QString subjectKey = triple.subject.name + "_" + QString::number(static_cast<int>(triple.subject.type));
            QString objectKey = triple.object.name + "_" + QString::number(static_cast<int>(triple.object.type));

            if (!nodeMap.contains(subjectKey)) {
                optikg::GraphNode node(triple.subject);
                node.id = subjectKey;
                nodes.append(node);
                nodeMap.insert(subjectKey, node);
            }
            if (!nodeMap.contains(objectKey)) {
                optikg::GraphNode node(triple.object);
                node.id = objectKey;
                nodes.append(node);
                nodeMap.insert(objectKey, node);
            }

            // 边去重
            QPair<QString, QString> edgeKey = qMakePair(subjectKey, objectKey);
            if (!edgeSet.contains(edgeKey)) {
                edgeSet.insert(edgeKey);
                edges.append(optikg::GraphEdge(subjectKey, objectKey, triple.relation, triple.confidence));
            }
        }

        Logger::info(QString("[onOpenClicked] 设置图谱数据: %1 个节点, %2 条边").arg(nodes.size()).arg(edges.size()));
        graphWidget_->setGraphData(nodes, edges);
    } else {
        Logger::warning("[onOpenClicked] graphWidget_ 为空");
    }

    // 更新状态栏计数
    if (countLabel_) {
        countLabel_->setText(tr("实体: %1 关系: %2").arg(importedTriples.size() * 2).arg(importedTriples.size()));
    }
    if (statusLabel_) {
        statusLabel_->setText(tr("已导入 %1 个三元组").arg(importedTriples.size()));
    }

    Logger::info(QString("[onOpenClicked] 成功导入 %1 个三元组").arg(importedTriples.size()));
    showToast(tr("成功导入 %1 个三元组").arg(importedTriples.size()), false);
}

void MainWindow::onOpenTextFileClicked() {
    // 选择纯文本文件导入到输入框
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("打开纯文本文件"), "",
        tr("文本文件 (*.txt);;所有文件 (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showToast(tr("无法打开文件: %1").arg(file.errorString()), true);
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    file.close();

    // 设置到抽取面板
    if (extractionPanel_) {
        extractionPanel_->setText(content);
        showToast(tr("已加载文本文件，共 %1 个字符").arg(content.length()), false);
    }
}

void MainWindow::loadRecordDetails(qint64 recordId) {
    // 从数据库加载记录
    ExtractionRecord record = DatabaseManager::instance().getExtractionRecord(recordId);
    if (record.id == -1) {
        showToast(tr("记录 %1 不存在").arg(recordId), true);
        return;
    }

    // 更新抽取面板文本
    if (extractionPanel_) {
        extractionPanel_->setText(record.content);
    }

    // 更新当前结果
    currentResults_ = record.triples;

    // 更新结果树
    if (resultTree_) {
        resultTree_->setResults(record.triples);
    }

    // 更新知识图谱
    if (graphWidget_) {
        // 将triples转换为图数据
        QList<optikg::GraphNode> nodes;
        QList<optikg::GraphEdge> edges;
        QHash<QString, optikg::GraphNode> nodeMap; // 实体名 -> 节点
        QSet<QPair<QString, QString>> edgeSet; // 用于边去重

        // 收集所有唯一实体和边
        for (const auto& triple : record.triples) {
            QString subjectKey = triple.subject.name + "_" + QString::number(static_cast<int>(triple.subject.type));
            QString objectKey = triple.object.name + "_" + QString::number(static_cast<int>(triple.object.type));

            if (!nodeMap.contains(subjectKey)) {
                optikg::GraphNode node(triple.subject);
                node.id = subjectKey;
                nodes.append(node);
                nodeMap.insert(subjectKey, node);
            }
            if (!nodeMap.contains(objectKey)) {
                optikg::GraphNode node(triple.object);
                node.id = objectKey;
                nodes.append(node);
                nodeMap.insert(objectKey, node);
            }

            // 边去重：避免重复的源-目标对
            QPair<QString, QString> edgeKey = qMakePair(subjectKey, objectKey);
            if (!edgeSet.contains(edgeKey)) {
                edgeSet.insert(edgeKey);
                edges.append(optikg::GraphEdge(subjectKey, objectKey, triple.relation, triple.confidence));
            }
        }

        graphWidget_->setGraphData(nodes, edges);
    }

    // 更新状态栏
    if (statusLabel_) {
        statusLabel_->setText(tr("加载记录 #%1 (%2)").arg(recordId).arg(record.createdAt.toString("yyyy-MM-dd hh:mm")));
    }
    if (countLabel_) {
        countLabel_->setText(tr("实体: %1 关系: %2").arg(record.entityCount).arg(record.relationCount));
    }

    showToast(tr("已加载记录 #%1").arg(recordId), false);
}

void MainWindow::onHelpClicked() {
    QString helpText = tr(
        "<h3>OptiKG 使用说明</h3>"
        "<p>OptiKG 是一个工业知识图谱抽取系统，支持从中文文本中抽取实体和关系，并可视化展示。</p>"

        "<h4>主要功能</h4>"
        "<ul>"
        "<li><b>文本输入：</b>在左侧文本框中输入维修描述文本（最多1000字符）</li>"
        "<li><b>知识抽取：</b>点击“抽取”按钮，系统将自动识别文本中的实体和关系</li>"
        "<li><b>图谱可视化：</b>右侧展示抽取结果的知识图谱，可缩放、平移、高亮节点</li>"
        "<li><b>结果列表：</b>右侧下方显示详细的三元组列表，点击可高亮对应节点</li>"
        "<li><b>历史记录：</b>左侧下方保存所有抽取记录，可双击加载历史记录</li>"
        "</ul>"

        "<h4>操作指南</h4>"
        "<ul>"
        "<li><b>抽取：</b>Ctrl+E 或点击工具栏抽取按钮</li>"
        "<li><b>清空：</b>Ctrl+Shift+C 或点击工具栏清空按钮</li>"
        "<li><b>批量处理：</b>Ctrl+B 批量处理CSV文件</li>"
        "<li><b>导出：</b>文件菜单中可导出JSON或CSV格式</li>"
        "<li><b>图谱操作：</b>鼠标滚轮缩放，拖拽平移，右键菜单</li>"
        "</ul>"

        "<h4>CSV批量处理</h4>"
        "<p>支持批量处理CSV文件，自动识别编码格式（UTF-8、GBK等），正确处理包含逗号的字段。</p>"
        "<p>CSV格式要求：第一列为文本内容，后续列为可选标签。</p>"

        "<h4>系统设置</h4>"
        "<ul>"
        "<li><b>模型路径：</b>自动检测或手动指定模型文件</li>"
        "<li><b>置信度阈值：</b>调整实体和关系的筛选阈值</li>"
        "</ul>"

        "<p><b>提示：</b>使用前请确保已正确配置模型文件路径。</p>"
    );

    QMessageBox helpBox(this);
    helpBox.setWindowTitle(tr("OptiKG 使用说明"));
    helpBox.setText(helpText);
    helpBox.setIcon(QMessageBox::Information);
    helpBox.setStandardButtons(QMessageBox::Ok);
    helpBox.setDefaultButton(QMessageBox::Ok);
    helpBox.setTextFormat(Qt::RichText);
    helpBox.exec();
}

void MainWindow::onAboutClicked() {
    QString aboutText = tr(
        "<h3>Optikg 知识图谱抽取工具</h3>"
        "<p>版本: 1.0.0</p>"
        "<p>基于深度学习模型的知识图谱抽取系统，支持实体识别和关系抽取。</p>"
        "<p>功能特性：</p>"
        "<ul>"
        "<li>中文文本实体抽取（部件、故障、工具、组成）</li>"
        "<li>关系抽取与三元组构建</li>"
        "<li>知识图谱可视化展示</li>"
        "<li>历史记录管理与导出</li>"
        "</ul>"
        "<p>开发团队：Optikg Team</p>"
        "<p>© 2026 版权所有</p>"
    );

    QMessageBox::about(this, tr("关于 Optikg"), aboutText);
}

} // namespace optikg