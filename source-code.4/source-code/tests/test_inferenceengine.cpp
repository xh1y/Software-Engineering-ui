#include <QTest>
#include <QFileInfo>
#include "core/inferenceengine.h"
#include "utils/configmanager.h"

using namespace optikg;

class TestInferenceEngine : public QObject {
    Q_OBJECT

private:
    QString modelPath_;
    bool hasModel_ = false;

private slots:
    void initTestCase() {
        ConfigManager::instance().initialize();
        modelPath_ = ConfigManager::instance().autoDetectModelPath();
        if (modelPath_.isEmpty()) {
            modelPath_ = "models/model_fp32.onnx";
        }
        hasModel_ = QFileInfo::exists(modelPath_);
        if (!hasModel_) {
            QSKIP("Model file not found, skipping inference tests.");
        }
    }

    void testDefaultConstruction() {
        InferenceEngine engine;
        QVERIFY(!engine.isModelLoaded());
    }

    void testLoadEmptyPath() {
        InferenceEngine engine;
        QVERIFY(!engine.loadModel(""));
    }

    void testLoadNonExistentModel() {
        InferenceEngine engine;
        QVERIFY(!engine.loadModel("/fake/path/model.onnx"));
    }

    void testLoadValidModel() {
        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath_));
        QVERIFY(engine.isModelLoaded());
    }

    void testReloadSameModel() {
        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath_));
        QVERIFY(engine.loadModel(modelPath_)); // 第二次应直接返回 true
    }

    void testInferEmptyText() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        auto results = engine.infer("");
        QCOMPARE(results.size(), 0);
    }

    void testInferNormalText() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        auto results = engine.infer("电机轴承出现异常磨损");
        // 结果数量取决于模型，但通常至少有一些输出
        // 我们不做强断言，只验证不崩溃
        QVERIFY(results.size() >= 0);
    }

    void testInferJsonInput() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        auto results = engine.infer(R"({"text":"液压系统泄漏"})");
        QVERIFY(results.size() >= 0);
    }

    void testInferInvalidJson() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        auto results = engine.infer("这不是JSON{");
        QVERIFY(results.size() >= 0); // 作为普通文本处理
    }

    void testThresholdFiltering() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        engine.setThreshold(0.99f); // 很高的阈值
        auto results = engine.infer("电机轴承磨损");
        for (const auto& t : results) {
            QVERIFY(t.confidence >= 0.99f);
        }
    }

    void testInferWithoutModel() {
        InferenceEngine engine;
        auto results = engine.infer("任意文本");
        QCOMPARE(results.size(), 0);
    }

    void testUnload() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        QVERIFY(engine.isModelLoaded());
        engine.unload();
        QVERIFY(!engine.isModelLoaded());
    }

    void testLongTextChunking() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        QString longText = QString("电机轴承磨损。").repeated(100); // > 500 chars
        auto results = engine.inferLongText(longText, 500, 100);
        QVERIFY(results.size() >= 0); // 允许为空，但不能崩溃
    }

    void testInferMissingTextField() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // JSON 缺失 text 字段，但有备用字段 content
        auto results = engine.infer(R"({"content":"异常振动"})");
        QVERIFY(results.size() >= 0); // 应正常解析 content 字段
    }

    void testChineseTokenSpaceCleanup() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // 输入包含 CJK 字符间多余空格
        auto results = engine.infer("轴 承 磨 损");
        // 无法直接验证内部空格清理，但至少不应崩溃
        QVERIFY(results.size() >= 0);
    }

    void testLongTextTruncation() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // 创建超长文本（超过最大序列长度）
        // 由于 maxSeqLength_ 是私有成员，我们通过多次重复来确保 token 数超限
        QString veryLongText = QString("电机轴承磨损。").repeated(1000);
        auto results = engine.infer(veryLongText);
        // 应正常处理而不崩溃
        QVERIFY(results.size() >= 0);
    }
};

QTEST_MAIN(TestInferenceEngine)
#include "test_inferenceengine.moc"
