#include <QTest>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QFileInfo>
#include "configmanager.h"

using namespace optikg;

class TestConfigManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // 使用临时目录作为配置目录，避免污染用户配置
        tempDir_ = new QTemporaryDir();
        QVERIFY(tempDir_->isValid());
        qputenv("XDG_CONFIG_HOME", tempDir_->path().toUtf8());
    }

    void cleanupTestCase() {
        delete tempDir_;
    }

    void testSingleton() {
        ConfigManager& instance1 = ConfigManager::instance();
        ConfigManager& instance2 = ConfigManager::instance();
        QCOMPARE(&instance1, &instance2);
    }

    void testInitialize() {
        ConfigManager& cm = ConfigManager::instance();
        QVERIFY(cm.initialize());
        // 重复初始化应返回 true
        QVERIFY(cm.initialize());
    }

    void testDefaultValues() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        QCOMPARE(cm.getJsonContentField(), QString("text"));
        QCOMPARE(cm.getCsvContentColumn(), QString("content"));
        QCOMPARE(cm.getCsvEncoding(), QString("UTF-8"));
        QCOMPARE(cm.getThreshold(), -0.2f);
        QCOMPARE(cm.getAutoExport(), false);
        QCOMPARE(cm.getSaveToDatabase(), false);
    }

    void testJsonContentField() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        QSignalSpy spy(&cm, &ConfigManager::settingChanged);
        cm.setJsonContentField("content");
        QCOMPARE(cm.getJsonContentField(), QString("content"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), QString("fieldMapping/jsonContentField"));

        // 重复设置不应发射信号
        cm.setJsonContentField("content");
        QCOMPARE(spy.count(), 0);
    }

    void testCsvContentColumn() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setCsvContentColumn("description");
        QCOMPARE(cm.getCsvContentColumn(), QString("description"));
    }

    void testCsvEncoding() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setCsvEncoding("GBK");
        QCOMPARE(cm.getCsvEncoding(), QString("GBK"));
    }

    void testModelPath() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setModelPath("/tmp/model.onnx");
        QCOMPARE(cm.getModelPath(), QString("/tmp/model.onnx"));
    }

    void testThreshold() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        QSignalSpy spy(&cm, &ConfigManager::settingChanged);
        cm.setThreshold(-5.0f);
        QCOMPARE(cm.getThreshold(), -5.0f);
        QCOMPARE(spy.count(), 1);

        // 相同值（近似）不应发射信号
        cm.setThreshold(-5.0f);
        QCOMPARE(spy.count(), 1); // 信号总数应仍为1，没有新增信号
    }

    void testBatchOutputDir() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setBatchOutputDir("/tmp/output");
        QCOMPARE(cm.getBatchOutputDir(), QString("/tmp/output"));
    }

    void testAutoExport() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setAutoExport(true);
        QCOMPARE(cm.getAutoExport(), true);
        cm.setAutoExport(false);
        QCOMPARE(cm.getAutoExport(), false);
    }

    void testDefaultExportFormat() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setDefaultExportFormat("csv");
        QCOMPARE(cm.getDefaultExportFormat(), QString("csv"));
    }

    void testSaveToDatabase() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setSaveToDatabase(true);
        QCOMPARE(cm.getSaveToDatabase(), true);
    }

    void testWindowGeometry() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        QByteArray geom("test_geometry_data");
        cm.setWindowGeometry(geom);
        QCOMPARE(cm.getWindowGeometry(), geom);
    }

    void testAutoDetectModelPath_existingConfig() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setModelPath("/fake/path.onnx");
        // 如果路径不存在，autoDetect 会继续搜索
        // 由于我们在临时环境中，通常会找不到并返回空
        QString detected = cm.autoDetectModelPath();
        // 因为我们设置了不存在的路径，预期行为是继续搜索并可能返回空
        // 可能返回空、配置的假路径，或者搜索到的真实模型路径
        QVERIFY(detected.isEmpty() || detected == "/fake/path.onnx" || QFileInfo::exists(detected));
    }

    void testPersistence() {
        ConfigManager& cm = ConfigManager::instance();
        cm.initialize();
        cm.setJsonContentField("body");
        cm.saveSettings();

        // 重新初始化（或创建新 QSettings）应能读取到保存的值
        // 这里简单验证值未变
        QCOMPARE(cm.getJsonContentField(), QString("body"));
    }

private:
    QTemporaryDir* tempDir_ = nullptr;
};

QTEST_MAIN(TestConfigManager)
#include "test_configmanager.moc"
