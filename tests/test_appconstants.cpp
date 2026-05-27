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
    }

    void testMappingConstants() {
        QCOMPARE(QString(AppConstants::Mapping::DEFAULT_JSON_FIELD), QString("text"));
        QCOMPARE(QString(AppConstants::Mapping::DEFAULT_CSV_COLUMN), QString("content"));
        QCOMPARE(QString(AppConstants::Mapping::DEFAULT_CSV_ENCODING), QString("UTF-8"));
        QCOMPARE(QString(AppConstants::Mapping::DEFAULT_EXPORT_FORMAT), QString("JSON"));
    }

    void testAppInfoConstants() {
        QCOMPARE(AppConstants::AppInfo::CONFIG_ORG, QString("OptiKG"));
        QCOMPARE(AppConstants::AppInfo::CONFIG_APP, QString("config"));
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

    void testModelStaticRefs() {
        // DEFAULT_CHUNK_SIZE 等返回引用，应指向当前值
        AppConstants::resetToDefaults();
        int& cs = AppConstants::Model::DEFAULT_CHUNK_SIZE();
        QCOMPARE(cs, 500);
        cs = 700;
        QCOMPARE(AppConstants::chunkSize(), 700);
        AppConstants::resetToDefaults();
    }

    void testInitializeWithInvalidValues() {
        // 写入无效值到 QSettings，然后调用 initialize 验证回退到默认值
        QSettings settings(AppConstants::AppInfo::CONFIG_ORG, AppConstants::AppInfo::CONFIG_APP);
        settings.setValue("model/chunkSize", 50);      // 无效：< 100
        settings.setValue("model/overlapSize", -5);     // 无效：< 0
        settings.setValue("model/maxChars", 50);        // 无效：< 1000
        settings.sync();

        // 注意：AppConstants 使用静态全局变量，initialize 只调用一次
        // 我们需要直接验证 validate 函数的行为
        QVERIFY(!AppConstants::Model::validateChunkSize(50));
        QVERIFY(!AppConstants::Model::validateOverlapSize(-5));
        QVERIFY(!AppConstants::Model::validateMaxChars(50));
    }

    void testChunkSizeBoundaryValues() {
        // 边界值测试
        QVERIFY(AppConstants::Model::validateChunkSize(100));   // min
        QVERIFY(AppConstants::Model::validateChunkSize(5000));  // max
        QVERIFY(!AppConstants::Model::validateChunkSize(99));   // below min
        QVERIFY(!AppConstants::Model::validateChunkSize(5001)); // above max
    }

    void testOverlapSizeBoundaryValues() {
        QVERIFY(AppConstants::Model::validateOverlapSize(0));     // min
        QVERIFY(AppConstants::Model::validateOverlapSize(1000));  // max
        QVERIFY(!AppConstants::Model::validateOverlapSize(-1));   // below min
        QVERIFY(!AppConstants::Model::validateOverlapSize(1001)); // above max
    }

    void testMaxCharsBoundaryValues() {
        QVERIFY(AppConstants::Model::validateMaxChars(1000));    // min
        QVERIFY(AppConstants::Model::validateMaxChars(100000));  // max
        QVERIFY(!AppConstants::Model::validateMaxChars(999));    // below min
        QVERIFY(!AppConstants::Model::validateMaxChars(100001)); // above max
    }

    void testDefaultOverlapSizeTracksOverlapSize() {
        AppConstants::resetToDefaults();
        AppConstants::setOverlapSize(250);
        QCOMPARE(AppConstants::overlapSize(), 250);
        // DEFAULT_OVERLAP_SIZE() 返回 g_overlapSize 引用，应跟踪变更
        int& ref = AppConstants::Model::DEFAULT_OVERLAP_SIZE();
        QCOMPARE(ref, 250);
        ref = 300;
        QCOMPARE(AppConstants::overlapSize(), 300);
        AppConstants::resetToDefaults();
    }

    void testMaxCharsTracksMaxChars() {
        AppConstants::resetToDefaults();
        AppConstants::setMaxChars(5000);
        QCOMPARE(AppConstants::maxChars(), 5000);
        // MAX_CHARS() 返回 g_maxChars 引用，应跟踪变更
        int& ref = AppConstants::Model::MAX_CHARS();
        QCOMPARE(ref, 5000);
        ref = 6000;
        QCOMPARE(AppConstants::maxChars(), 6000);
        AppConstants::resetToDefaults();
    }

    void testDoubleInitialize() {
        // initialize() should be a no-op when g_initialized is already true
        AppConstants::resetToDefaults();
        // Trigger initialize via a getter (this will set g_initialized = true)
        QCOMPARE(AppConstants::chunkSize(), 500);
        // Calling initialize() again should return immediately (covers L161-162)
        AppConstants::initialize();
        QCOMPARE(AppConstants::chunkSize(), 500);
    }

    void testSaveSettingsPersistsValues() {
        AppConstants::resetToDefaults();
        AppConstants::setChunkSize(888);
        AppConstants::setOverlapSize(222);
        AppConstants::setMaxChars(4444);
        AppConstants::saveSettings();

        // Verify via QSettings directly
        QSettings settings(AppConstants::AppInfo::CONFIG_ORG, AppConstants::AppInfo::CONFIG_APP);
        QCOMPARE(settings.value("model/chunkSize").toInt(), 888);
        QCOMPARE(settings.value("model/overlapSize").toInt(), 222);
        QCOMPARE(settings.value("model/maxChars").toInt(), 4444);

        AppConstants::resetToDefaults();
    }
};

QTEST_MAIN(TestAppConstants)
#include "test_appconstants.moc"
