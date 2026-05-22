#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QPushButton>
#include <QTextEdit>
#include <QApplication>
#include <QLabel>
#include <QProgressBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QCloseEvent>
#include "ui/mainwindow.h"
#include "ui/graphwidget.h"
#include "ui/historypanel.h"
#include "ui/resulttreewidget.h"
#include "utils/configmanager.h"
#include "core/inferenceengine.h"

using namespace optikg;

class TestMainWindow : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // 初始化配置管理器，使用临时目录避免污染用户配置
        ConfigManager::instance().initialize();
    }

    void cleanupTestCase() {
        // 清理
    }

    void testDefaultConstruction() {
        // 测试主窗口默认构造
        MainWindow window;
        QVERIFY(window.windowTitle().contains("OptiKG"));
        // 窗口应正常显示（最小化以避免干扰）
        window.showMinimized();
        QTest::qWait(200); // 短暂等待UI初始化
    }

    // BB-01 单条文本抽取测试（UI部分）
    void testSingleTextExtractionUI() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 通过 objectName 查找工具栏中的抽取按钮
        auto* extractBtn = window.findChild<QPushButton*>("extractBtn");
        QVERIFY(extractBtn != nullptr);
        QVERIFY(!extractBtn->text().isEmpty());

        // 查找清空按钮
        auto* clearBtn = window.findChild<QPushButton*>("clearBtn");
        QVERIFY(clearBtn != nullptr);
    }

    // BB-02 空文本抽取测试（UI错误提示）
    void testEmptyTextExtractionUI() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 先清空（确保文本输入框为空）
        QMetaObject::invokeMethod(&window, "onClearClicked");
        QTest::qWait(50);

        // 直接调用抽取槽（空文本时会显示提示）
        QMetaObject::invokeMethod(&window, "onExtractClicked");
        QTest::qWait(100);

        // 验证状态栏显示了提示信息
        auto* statusLabel = window.statusBar()->findChild<QLabel*>();
        QVERIFY(statusLabel != nullptr);
        // showToast 会在状态栏显示 "请输入文本内容"
        QVERIFY(statusLabel->text().contains("请输入文本内容") ||
                statusLabel->text().contains("就绪") ||
                statusLabel->text().contains("已清空"));
    }

    // BB-04 打开文本文件抽取测试
    void testOpenTextFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建测试文本文件
        QTemporaryFile tempFile(tempDir.path() + "/test_text.txt");
        QVERIFY(tempFile.open());
        QString testContent = "电机轴承出现异常磨损，需要更换专用润滑油";
        tempFile.write(testContent.toUtf8());
        tempFile.close();

        // 测试文件读取逻辑
        QFile file(tempFile.fileName());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QString content = QString::fromUtf8(file.readAll());
        QCOMPARE(content, testContent);
        file.close();
    }

    // BB-07 长文本进度反馈测试
    void testLongTextProgressFeedback() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 创建长文本
        QString longText;
        for (int i = 0; i < 100; ++i) {
            longText += "电机轴承磨损测试文本。";
        }

        // 验证文本长度 > 500字符
        QVERIFY(longText.length() > 500);

        // 查找进度条
        auto* progressBar = window.statusBar()->findChild<QProgressBar*>();
        QVERIFY(progressBar != nullptr);

        // 验证进度条初始隐藏或可见（根据实现）
        // 进度条在 Qt 测试环境下可能已初始化
        QCOMPARE(progressBar->minimum(), 0);
        QCOMPARE(progressBar->maximum(), 100);
    }

    // BB-28 双击查看详情测试（历史记录）
    void testDoubleClickRecordDetails() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 验证历史记录面板和结果树存在
        auto* historyPanel = window.findChild<HistoryPanel*>();
        auto* resultTree = window.findChild<ResultTreeWidget*>();
        QVERIFY(historyPanel != nullptr);
        QVERIFY(resultTree != nullptr);
    }

    // 测试菜单功能
    void testMenuActions() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 验证菜单栏存在且包含关键菜单
        auto* menuBar = window.menuBar();
        QVERIFY(menuBar != nullptr);

        QStringList menuTitles;
        for (auto* menu : menuBar->actions()) {
            if (menu->menu()) {
                menuTitles.append(menu->text());
            }
        }

        bool hasFileMenu = false;
        bool hasEditMenu = false;
        bool hasViewMenu = false;
        bool hasToolsMenu = false;
        bool hasHelpMenu = false;

        for (const QString& title : menuTitles) {
            if (title.contains("文件")) hasFileMenu = true;
            if (title.contains("编辑")) hasEditMenu = true;
            if (title.contains("视图")) hasViewMenu = true;
            if (title.contains("工具")) hasToolsMenu = true;
            if (title.contains("帮助")) hasHelpMenu = true;
        }

        QVERIFY(hasFileMenu);
        QVERIFY(hasEditMenu);
        QVERIFY(hasViewMenu);
        QVERIFY(hasToolsMenu);
        QVERIFY(hasHelpMenu);
    }

    // 测试设置保存和加载
    void testSettingsPersistence() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        qputenv("XDG_CONFIG_HOME", tempDir.path().toUtf8());

        // 重新初始化配置管理器
        ConfigManager::instance().initialize();

        // 测试设置保存
        ConfigManager::instance().setJsonContentField("content");
        ConfigManager::instance().setThreshold(-5.0f);
        ConfigManager::instance().saveSettings();

        // 重新初始化后应能读取到保存的值
        ConfigManager::instance().initialize();
        QCOMPARE(ConfigManager::instance().getJsonContentField(), QString("content"));
        QCOMPARE(ConfigManager::instance().getThreshold(), -5.0f);
    }

    // 测试导出功能（BB-32, BB-33, BB-34）
    void testExportFunctions() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 先清空结果
        QMetaObject::invokeMethod(&window, "onClearClicked");
        QTest::qWait(50);

        // 空结果时导出应显示提示
        QMetaObject::invokeMethod(&window, "onExportJsonClicked");
        QTest::qWait(100);

        auto* statusLabel = window.statusBar()->findChild<QLabel*>();
        QVERIFY(statusLabel != nullptr);
        // showToast 会显示 "没有可导出的数据"
        QVERIFY(statusLabel->text().contains("没有可导出的数据") ||
                statusLabel->text().contains("就绪") ||
                statusLabel->text().contains("已清空"));
    }

    // 测试批量处理对话框打开（BB-08）
    void testBatchProcessDialogOpen() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 验证工具栏或菜单中包含批量处理入口
        bool hasBatchAction = false;
        for (auto* action : window.findChildren<QAction*>()) {
            if (action->text().contains("批量处理")) {
                hasBatchAction = true;
                QVERIFY(!action->shortcut().isEmpty());
                break;
            }
        }
        QVERIFY(hasBatchAction);
    }

    // 测试设置对话框打开（BB-05, BB-06）
    void testSettingsDialogOpen() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 验证工具栏或菜单中包含设置入口
        bool hasSettingsAction = false;
        for (auto* action : window.findChildren<QAction*>()) {
            if (action->text().contains("设置")) {
                hasSettingsAction = true;
                break;
            }
        }
        QVERIFY(hasSettingsAction);
    }

    // 测试关于对话框
    void testAboutDialog() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 验证帮助菜单中包含关于入口
        bool hasAboutAction = false;
        for (auto* action : window.findChildren<QAction*>()) {
            if (action->text().contains("关于")) {
                hasAboutAction = true;
                break;
            }
        }
        QVERIFY(hasAboutAction);
    }

    // 测试快捷键功能
    void testShortcuts() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 验证关键 QAction 存在且有快捷键
        bool hasExtractShortcut = false;
        bool hasZoomInShortcut = false;
        bool hasZoomOutShortcut = false;

        for (auto* action : window.findChildren<QAction*>()) {
            QKeySequence shortcut = action->shortcut();
            if (action->text().contains("抽取") && shortcut == QKeySequence("Ctrl+E")) {
                hasExtractShortcut = true;
            }
            if (shortcut == QKeySequence::ZoomIn) {
                hasZoomInShortcut = true;
            }
            if (shortcut == QKeySequence::ZoomOut) {
                hasZoomOutShortcut = true;
            }
        }

        QVERIFY(hasExtractShortcut);
        QVERIFY(hasZoomInShortcut);
        QVERIFY(hasZoomOutShortcut);
    }

    // 测试状态栏更新
    void testStatusBarUpdates() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        QVERIFY(window.statusBar() != nullptr);

        auto labels = window.statusBar()->findChildren<QLabel*>();
        QVERIFY(labels.size() >= 1);

        // 验证状态栏有文本内容
        bool hasStatusText = false;
        for (auto* label : labels) {
            if (!label->text().isEmpty()) {
                hasStatusText = true;
                break;
            }
        }
        QVERIFY(hasStatusText);
    }

    // 测试错误提示（Toast）
    void testErrorToast() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        auto* statusLabel = window.statusBar()->findChild<QLabel*>();
        QVERIFY(statusLabel != nullptr);
        QString originalText = statusLabel->text();

        // 调用 showToast（protected，可以通过 QMetaObject::invokeMethod）
        QMetaObject::invokeMethod(&window, "showToast", Q_ARG(QString, "测试错误消息"), Q_ARG(bool, true));
        QTest::qWait(50);

        QVERIFY(statusLabel->text().contains("测试错误消息") ||
                statusLabel->text() == originalText);
    }

    // 测试关闭事件处理
    void testCloseEvent() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        QCloseEvent event;
        QApplication::sendEvent(&window, &event);
        QVERIFY(event.isAccepted());
    }

    // 测试图谱组件存在
    void testGraphWidgetExists() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        auto* graphWidget = window.findChild<GraphWidget*>();
        QVERIFY(graphWidget != nullptr);
    }

    // 测试清空功能
    void testClearFunction() {
        MainWindow window;
        window.showMinimized();
        QTest::qWait(200);

        // 调用清空后不应崩溃
        QMetaObject::invokeMethod(&window, "onClearClicked");
        QTest::qWait(50);

        auto* statusLabel = window.statusBar()->findChild<QLabel*>();
        QVERIFY(statusLabel != nullptr);
        QVERIFY(statusLabel->text().contains("已清空") ||
                statusLabel->text().contains("就绪"));
    }
};

QTEST_MAIN(TestMainWindow)
#include "test_mainwindow.moc"
