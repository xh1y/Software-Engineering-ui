#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "utils/appconstants.h"

using namespace optikg;

class TestAppConstants : public QObject {
    Q_OBJECT

private:
    QTemporaryDir* tempDir_ = nullptr;

private slots:
    void initTestCase() {
        tempDir_ = new QTemporaryDir();
        QVERIFY(tempDir_->isValid());
        qputenv("XDG_CONFIG_HOME", tempDir_->path().toUtf8());
    }

    void cleanupTestCase() {
        delete tempDir_;
    }

    void init() {
        // 每个测试前重置到默认值
        AppConstants::resetToDefaults();
    }

    void testDefaultValues() {
        QCOMPARE(AppConstants::chunkSize(), 500);
        QCOMPARE(AppConstants::overlapSize(), 100);
        QCOMPARE(AppConstants::maxChars(), 10000);
    }

    void testInitializeAndLoad() {
        // 修改后保存，再初始化应能加载
        AppConstants::setChunkSize(800);
        AppConstants::setOverlapSize(50);
        AppConstants::setMaxChars(5000);
        AppConstants::saveSettings();

        // 重置到默认值后再初始化
        AppConstants::resetToDefaults();
        QCOMPARE(AppConstants::chunkSize(), 500);

        // 直接重新读取 settings（通过重新初始化）
        // 由于 resetToDefaults 会 saveSettings，之前的值被覆盖了
        // 所以这里测试 initialize 加载已保存的值，需要先保存非默认值
        AppConstants::setChunkSize(600);
        AppConstants::saveSettings();

        // 创建新 QSettings 验证值已持久化
        QSettings settings(AppConstants::AppInfo::CONFIG_ORG, AppConstants::AppInfo::CONFIG_APP);
        QCOMPARE(settings.value("model/chunkSize").toInt(), 600);
    }

    void testSetChunkSize() {
        AppConstants::setChunkSize(1000);
        QCOMPARE(AppConstants::chunkSize(), 1000);

        // 无效值应被忽略
        AppConstants::setChunkSize(50);
        QCOMPARE(AppConstants::chunkSize(), 1000); // 保持原值
    }

    void testSetOverlapSize() {
        AppConstants::setOverlapSize(200);
        QCOMPARE(AppConstants::overlapSize(), 200);

        // 无效值应被忽略
        AppConstants::setOverlapSize(-1);
        QCOMPARE(AppConstants::overlapSize(), 200);
    }

    void testSetMaxChars() {
        AppConstants::setMaxChars(20000);
        QCOMPARE(AppConstants::maxChars(), 20000);

        // 无效值应被忽略
        AppConstants::setMaxChars(500);
        QCOMPARE(AppConstants::maxChars(), 20000);
    }

    void testResetToDefaults() {
        AppConstants::setChunkSize(1234);
        AppConstants::setOverlapSize(567);
        AppConstants::setMaxChars(99999);
        AppConstants::resetToDefaults();

        QCOMPARE(AppConstants::chunkSize(), 500);
        QCOMPARE(AppConstants::overlapSize(), 100);
        QCOMPARE(AppConstants::maxChars(), 10000);
    }

    void testValidateChunkSize() {
        QVERIFY(AppConstants::Model::validateChunkSize(100));
        QVERIFY(AppConstants::Model::validateChunkSize(5000));
        QVERIFY(AppConstants::Model::validateChunkSize(500));
        QVERIFY(!AppConstants::Model::validateChunkSize(99));
        QVERIFY(!AppConstants::Model::validateChunkSize(5001));
        QVERIFY(!AppConstants::Model::validateChunkSize(0));
    }

    void testValidateOverlapSize() {
        QVERIFY(AppConstants::Model::validateOverlapSize(0));
        QVERIFY(AppConstants::Model::validateOverlapSize(1000));
        QVERIFY(AppConstants::Model::validateOverlapSize(100));
        QVERIFY(!AppConstants::Model::validateOverlapSize(-1));
        QVERIFY(!AppConstants::Model::validateOverlapSize(1001));
    }

    void testValidateMaxChars() {
        QVERIFY(AppConstants::Model::validateMaxChars(1000));
        QVERIFY(AppConstants::Model::validateMaxChars(100000));
        QVERIFY(AppConstants::Model::validateMaxChars(5000));
        QVERIFY(!AppConstants::Model::validateMaxChars(999));
        QVERIFY(!AppConstants::Model::validateMaxChars(100001));
    }

    void testUIConstants() {
        QCOMPARE(AppConstants::UI::WINDOW_WIDTH, 1200);
        QCOMPARE(AppConstants::UI::WINDOW_HEIGHT, 800);
        QCOMPARE(AppConstants::UI::TOAST_TIMEOUT_MS, 3000);
        QCOMPARE(AppConstants::UI::PROGRESS_MIN, 0);
        QCOMPARE(AppConstants::UI::PROGRESS_MAX, 100);
    }

    void testModelConstants() {
        QCOMPARE(AppConstants::Model::CHUNK_THRESHOLD, 450);
        QCOMPARE(AppConstants::Model::DEFAULT_THRESHOLD, -0.2f);
        QCOMPARE(AppConstants::Model::THRESHOLD_MIN, -100.0f);
        QCOMPARE(AppConstants::Model::THRESHOLD_MAX, 0.0f);
        QCOMPARE(AppConstants::Model::THRESHOLD_STEP, 1.0f);
    }

    void testFileConstants() {
        QCOMPARE(AppConstants::File::JSON_EXTENSION, QString(".json"));
        QCOMPARE(AppConstants::File::CSV_EXTENSION, QString(".csv"));
        QVERIFY(!AppConstants::File::getCsvEncodings().isEmpty());
        QVERIFY(AppConstants::File::getCsvEncodings().contains("UTF-8"));
        QVERIFY(!AppConstants::File::getExportFormats().isEmpty());
    }

    void testMappingConstants() {
        QCOMPARE(QString(AppConstants::Mapping::DEFAULT_JSON_FIELD), QString("text"));
        QCOMPARE(QString(AppConstants::Mapping::DEFAULT_CSV_COLUMN), QString("content"));
        QCOMPARE(QString(AppConstants::Mapping::DEFAULT_CSV_ENCODING), QString("UTF-8"));
        QCOMPARE(QString(AppConstants::Mapping::DEFAULT_EXPORT_FORMAT), QString("JSON"));
    }

    void testAppInfoConstants() {
        QCOMPARE(AppConstants::AppInfo::APP_NAME, QString("OptiKG"));
        QCOMPARE(AppConstants::AppInfo::APP_VERSION, QString("1.0.0"));
        QCOMPARE(AppConstants::AppInfo::ORG_NAME, QString("OptiKG"));
    }

    void testEntityTypeMap() {
        auto typeMap = AppConstants::getEntityTypeMap();
        QVERIFY(typeMap.contains("部件"));
        QVERIFY(typeMap.contains("故障"));
        QVERIFY(typeMap.contains("工具"));
        QVERIFY(typeMap.contains("组成"));

        QCOMPARE(typeMap["部件"].displayName, QString("部件"));
        QCOMPARE(typeMap["部件"].colorHex, QString("#FF6B6B"));
    }

    void testGraphConstants() {
        QCOMPARE(AppConstants::Graph::SCENE_RECT_X, -400.0);
        QCOMPARE(AppConstants::Graph::SCENE_RECT_Y, -300.0);
        QCOMPARE(AppConstants::Graph::SCENE_RECT_WIDTH, 800.0);
        QCOMPARE(AppConstants::Graph::SCENE_RECT_HEIGHT, 600.0);
        QCOMPARE(AppConstants::Graph::FORCE_ITERATIONS, 100);
    }

    void testDatabaseConstants() {
        QCOMPARE(AppConstants::Database::HISTORY_LIMIT, 100);
    }

    void testModelSearchPaths() {
        QStringList paths = AppConstants::Model::getModelSearchPaths("/tmp/app");
        QVERIFY(!paths.isEmpty());
        QVERIFY(paths.contains("/tmp/app/model/model_fp32.onnx"));
        QVERIFY(paths.contains("/tmp/app/../model/model_fp32.onnx"));
    }

    void testModelStaticRefs() {
        // DEFAULT_CHUNK_SIZE 等返回引用，应指向当前值
        AppConstants::resetToDefaults();
        int& cs = AppConstants::Model::DEFAULT_CHUNK_SIZE();
        QCOMPARE(cs, 500);
        cs = 700;
        QCOMPARE(AppConstants::chunkSize(), 700);
        AppConstants::resetToDefaults();
    }
};

QTEST_MAIN(TestAppConstants)
#include "test_appconstants.moc"
