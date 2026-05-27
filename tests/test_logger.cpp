#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include "utils/logger.h"

using namespace optikg;

class TestLogger : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // 确保干净状态
        Logger::restoreDefaultMessageHandler();
    }

    void cleanupTestCase() {
        Logger::restoreDefaultMessageHandler();
    }

    void cleanup() {
        Logger::instance().shutdown();
        Logger::restoreDefaultMessageHandler();
    }

    void testSingleton() {
        Logger &a = Logger::instance();
        Logger &b = Logger::instance();
        QCOMPARE(&a, &b);
    }

    void testLevelToString() {
        Logger &logger = Logger::instance();
        // 通过 log 方法间接验证，levelToString 是 private
        // 先初始化一下
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path()));
        logger.setLogLevel(LogLevel::Debug);

        // 验证各日志级别不会崩溃
        Logger::debug("debug msg");
        Logger::info("info msg");
        Logger::warning("warning msg");
        Logger::error("error msg");
        Logger::critical("critical msg");
        // 只要不崩溃就算通过
        QVERIFY(true);
    }

    void testInitializeDefault() {
        Logger &logger = Logger::instance();
        QVERIFY(logger.initialize());
        // 再次初始化应返回 true（已初始化）
        QVERIFY(logger.initialize());
    }

    void testInitializeCustomDirAndFile() {
        Logger &logger = Logger::instance();
        logger.shutdown();  // 确保初始状态

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path(), "test_custom.log"));

        // 检查日志文件是否存在
        QString logPath = tmpDir.filePath("test_custom.log");
        QVERIFY(QFile::exists(logPath));
    }

    void testAlreadyInitialized() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path()));
        // 第二次初始化应直接返回 true
        QVERIFY(logger.initialize());
    }

    void testSetAndGetLogLevel() {
        Logger &logger = Logger::instance();
        logger.setLogLevel(LogLevel::Error);
        QCOMPARE(logger.logLevel(), LogLevel::Error);

        logger.setLogLevel(LogLevel::Debug);
        QCOMPARE(logger.logLevel(), LogLevel::Debug);

        logger.setLogLevel(LogLevel::Warning);
        QCOMPARE(logger.logLevel(), LogLevel::Warning);
    }

    void testLogLevelFiltering() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path()));
        logger.setLogLevel(LogLevel::Warning);

        // Debug 和 Info 级别应被过滤
        Logger::debug("should be filtered");
        Logger::info("should be filtered");

        // Warning 及以上不应被过滤
        Logger::warning("should appear");
        Logger::error("should appear");
        Logger::critical("should appear");

        // 检查日志文件有内容
        QString logPath = tmpDir.filePath("*.log");
        // 文件名是自动生成的，遍历 tmpDir
        QDir dir(tmpDir.path());
        QStringList logs = dir.entryList({"*.log"});
        QVERIFY(!logs.isEmpty());
        QFile logFile(dir.filePath(logs.first()));
        QVERIFY(logFile.open(QIODevice::ReadOnly));
        QString content = logFile.readAll();
        // 过滤的消息不应出现
        QVERIFY(!content.contains("should be filtered"));
        // 不过滤的消息应出现
        QVERIFY(content.contains("should appear"));
    }

    void testLogBeforeInitialization() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        // 不先初始化，直接 log —— 应该自动调用 initialize()
        Logger::info("auto init log");
        // 不应崩溃
        QVERIFY(true);
    }

    void testShutdown() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path()));

        logger.shutdown();
        // 再次 shutdown 不应崩溃
        logger.shutdown();
        QVERIFY(true);
    }

    void testInstallAndRestoreMessageHandler() {
        // 安装 Qt 消息处理器，不开启控制台输出
        Logger::installQtMessageHandler(false);

        // qDebug 不应崩溃，应被重定向到 Logger
        qDebug() << "test message via Qt handler";
        QVERIFY(true);

        // 恢复默认处理器
        Logger::restoreDefaultMessageHandler();
        QVERIFY(true);
    }

    void testInstallMessageHandlerWithConsole() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path()));

        // 安装处理器，开启控制台输出
        Logger::installQtMessageHandler(true);

        qInfo() << "console test info";
        qWarning() << "console test warning";

        Logger::restoreDefaultMessageHandler();
        QVERIFY(true);
    }

    void testQtMessageHandlerWithQCritical() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path()));

        Logger::installQtMessageHandler(false);
        qCritical() << "critical message via Qt";
        Logger::restoreDefaultMessageHandler();
        QVERIFY(true);
    }

    void testLogFileContent() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString logFileName = "content_test.log";
        QVERIFY(logger.initialize(tmpDir.path(), logFileName));
        logger.setLogLevel(LogLevel::Debug);

        Logger::info("specific test message 12345");
        logger.shutdown();

        // 读取日志文件
        QFile logFile(tmpDir.filePath(logFileName));
        QVERIFY(logFile.open(QIODevice::ReadOnly));
        QString content = logFile.readAll();
        QVERIFY(content.contains("specific test message 12345"));
        QVERIFY(content.contains("Log system initialized"));
        QVERIFY(content.contains("Log system shutdown"));
    }

    void testMultipleLogLines() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path(), "multi.log"));
        logger.setLogLevel(LogLevel::Debug);

        for (int i = 0; i < 10; ++i) {
            Logger::info(QString("line %1").arg(i));
        }
        logger.shutdown();

        QFile logFile(tmpDir.filePath("multi.log"));
        QVERIFY(logFile.open(QIODevice::ReadOnly));
        QString content = logFile.readAll();
        for (int i = 0; i < 10; ++i) {
            QVERIFY(content.contains(QString("line %1").arg(i)));
        }
    }

    void testConsoleOutputWithAllMessageTypes() {
        Logger &logger = Logger::instance();
        logger.shutdown();
        Logger::restoreDefaultMessageHandler();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path(), "console_test.log"));
        logger.setLogLevel(LogLevel::Debug);

        // 安装处理器并开启控制台输出
        Logger::installQtMessageHandler(true);

        // 触发所有 Qt 消息类型以覆盖控制台输出分支
        // 先写入一条直接的日志确认 file 正常
        Logger::debug("direct debug log");

        qDebug() << "dbg";
        qInfo() << "inf";
        qWarning() << "wrn";

        Logger::restoreDefaultMessageHandler();

        // 验证日志文件包含直接写入的消息
        QFile logFile(tmpDir.filePath("console_test.log"));
        QVERIFY(logFile.open(QIODevice::ReadOnly));
        QString content = logFile.readAll();
        QVERIFY(content.contains("direct debug log"));
        // Qt 消息通过 qtMessageHandler 也应该出现在日志中
        QVERIFY(content.contains("dbg") || content.contains("inf") || content.contains("wrn"));
    }

    void testLogDebugLevel() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path(), "debug_test.log"));
        logger.setLogLevel(LogLevel::Debug);

        Logger::debug("debug level message");
        Logger::info("info after debug");
        Logger::warning("warning after debug");

        QFile logFile(tmpDir.filePath("debug_test.log"));
        QVERIFY(logFile.open(QIODevice::ReadOnly));
        QString content = logFile.readAll();
        QVERIFY(content.contains("debug level message"));
        QVERIFY(content.contains("info after debug"));
    }

    void testLogCriticalLevel() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path(), "critical_test.log"));
        logger.setLogLevel(LogLevel::Critical);

        // 低于 Critical 的消息应被过滤
        Logger::debug("debug filtered");
        Logger::info("info filtered");
        Logger::warning("warning filtered");
        Logger::error("error filtered");

        // Critical 应通过
        Logger::critical("critical passed");

        QFile logFile(tmpDir.filePath("critical_test.log"));
        QVERIFY(logFile.open(QIODevice::ReadOnly));
        QString content = logFile.readAll();
        QVERIFY(!content.contains("debug filtered"));
        QVERIFY(!content.contains("info filtered"));
        QVERIFY(!content.contains("warning filtered"));
        QVERIFY(!content.contains("error filtered"));
        QVERIFY(content.contains("critical passed"));
    }

    void testShutdownWhenNotInitialized() {
        Logger &logger = Logger::instance();
        logger.shutdown(); // first shutdown
        logger.shutdown(); // second shutdown — should be no-op
        QVERIFY(true);
    }

    void testLogAfterShutdown() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path()));
        logger.shutdown();

        // 日志在 shutdown 后应该自动重新初始化
        Logger::info("after shutdown msg");
        QVERIFY(true);
    }

    void testInitializeMkpathFailure() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        // 创建一个文件，它将阻塞 mkpath 创建同名目录
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString blockerPath = tmpDir.filePath("blocker_file");
        QFile blocker(blockerPath);
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.write("block");
        blocker.close();

        // 尝试在文件路径下创建子目录 → mkpath 会失败
        QString badDir = blockerPath + "/subdir/logs";
        QVERIFY(!logger.initialize(badDir));
        // 初始化失败后不应标记为已初始化
    }

    void testInitializeFileOpenFailure() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // 创建目录但设为只读，导致无法创建日志文件
        QString roDir = tmpDir.filePath("readonly_dir");
        QDir().mkpath(roDir);
        QFile::setPermissions(roDir,
            QFileDevice::ReadOwner | QFileDevice::ExeOwner);

        QVERIFY(!logger.initialize(roDir));

        // 恢复权限以便清理
        QFile::setPermissions(roDir,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }

    void testLogAfterFailedInitialization() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        // 尝试用一个无效路径初始化（应该失败）
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString blockerPath = tmpDir.filePath("blocker_file2");
        QFile blocker(blockerPath);
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.write("block");
        blocker.close();

        QString badDir = blockerPath + "/subdir/logs";
        QVERIFY(!logger.initialize(badDir));

        // 此时 initialized_ = false，再 log 应触发自动初始化 + fallback 到 qDebug
        Logger::info("after failed init");
        QVERIFY(true); // 不应崩溃
    }

    void testQtMessageHandlerAllMessageTypes() {
        Logger::instance().shutdown();
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Install with console output enabled
        Logger::installQtMessageHandler(true);
        QVERIFY(Logger::instance().initialize(tmpDir.path()));

        // Send messages at all Qt levels through the handler
        qDebug() << "handler debug msg";
        qInfo() << "handler info msg";
        qWarning() << "handler warning msg";
        qCritical() << "handler critical msg";

        // Restore and verify no crash
        Logger::restoreDefaultMessageHandler();
        QVERIFY(true);
    }

    void testInitializeWithCustomFileName() {
        Logger &logger = Logger::instance();
        logger.shutdown();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QVERIFY(logger.initialize(tmpDir.path(), "custom_log.txt"));

        // Verify the custom file was created
        QFile customFile(tmpDir.filePath("custom_log.txt"));
        QVERIFY(customFile.exists());
    }
};

QTEST_MAIN(TestLogger)
#include "test_logger.moc"
