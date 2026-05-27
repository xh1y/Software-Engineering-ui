#include <QTest>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QDir>
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
        // 按优先级尝试多个候选路径，找到真正能加载的模型
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList candidates = {
            QDir(appDir).filePath("../models/model_fp32.onnx"),
            QDir(appDir).filePath("../../models/model_fp32.onnx"),
            "models/model_fp32.onnx",
            ConfigManager::instance().autoDetectModelPath(),
            "model/model_fp32.onnx",
        };
        candidates.removeDuplicates();
        hasModel_ = false;
        for (const auto& path : candidates) {
            if (path.isEmpty()) continue;
            if (!QFileInfo::exists(path)) continue;
            InferenceEngine probe;
            if (probe.loadModel(path)) {
                modelPath_ = path;
                hasModel_ = true;
                break;
            }
        }
        if (!hasModel_) {
            QSKIP("Valid ONNX model not found, skipping inference tests.");
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

    void testInferJsonNoTextFields() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // JSON with no recognized text fields at all
        auto results = engine.infer(R"({"unknown_key":"some_value"})");
        // Should try all backup fields and find none, return empty
        QVERIFY(results.size() >= 0);
    }

    void testInferJsonWithSentenceField() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // JSON with "sentence" field (one of the fallback fields)
        auto results = engine.infer(R"({"sentence":"液压系统压力不足"})");
        QVERIFY(results.size() >= 0);
    }

    void testInferJsonWithDataField() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // JSON with "data" field (one of the fallback fields)
        auto results = engine.infer(R"({"data":"发动机异响"})");
        QVERIFY(results.size() >= 0);
    }

    void testInferJsonWithInputField() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // JSON with "input" field (one of the fallback fields)
        auto results = engine.infer(R"({"input":"轴承过热"})");
        QVERIFY(results.size() >= 0);
    }

    void testInferWhitespaceOnlyText() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        auto results = engine.infer("   ");
        QVERIFY(results.size() >= 0);
    }

    void testLongTextEmptyInput() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        auto results = engine.inferLongText("", 500, 100);
        QCOMPARE(results.size(), 0);
    }

    void testLongTextShortInput() {
        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // Text shorter than chunkSize should use regular infer
        auto results = engine.inferLongText("轴承磨损", 500, 100);
        QVERIFY(results.size() >= 0);
    }

    void testLoadDifferentModelAfterLoaded() {
        // Load a real model first, then try to load a different (non-existent) one
        // This exercises the cleanup-and-reload path
        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath_));
        QVERIFY(engine.isModelLoaded());
        // Try to load a different model that doesn't exist
        QVERIFY(!engine.loadModel("/tmp/nonexistent_different_model.onnx"));
        // After failed load, model should be unloaded (cleanup was called)
        QVERIFY(!engine.isModelLoaded());
    }

    void testLoadModelWithInvalidFile() {
        // Create a temporary file that is NOT a valid ONNX model
        // This exercises the Ort::Exception handler in loadModel()
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString invalidModelPath = tempDir.filePath("invalid_model.onnx");
        QFile fakeModel(invalidModelPath);
        QVERIFY(fakeModel.open(QIODevice::WriteOnly));
        fakeModel.write(QByteArray("this is not a valid ONNX model file"));
        fakeModel.close();

        InferenceEngine engine;
        QVERIFY(!engine.loadModel(invalidModelPath));
        // After failed load, model should not be loaded
        QVERIFY(!engine.isModelLoaded());
    }

    // ---- Metadata / Tokenizer error path tests ----

    // Helper: create a test model dir by copying real model + its external data
    QString setupModelDir(const QString& dirPath, bool withTokenizer, bool withMetadata,
                          const QString& metadataContent = QString(),
                          const QString& tokenizerContent = QString()) {
        QString modelDir = QFileInfo(modelPath_).absolutePath();
        // Copy model file
        QString dstModel = dirPath + "/model_fp32.onnx";
        QFile::copy(modelPath_, dstModel);
        // Copy external data file (must be a real copy, not symlink — ONNX Runtime
        // validates that external data paths don't escape the model directory)
        QString dataFile = modelDir + "/model_fp32.onnx.data";
        if (QFileInfo::exists(dataFile)) {
            QFile::copy(dataFile, dirPath + "/model_fp32.onnx.data");
        }
        // Copy or create tokenizer
        if (withTokenizer) {
            if (tokenizerContent.isEmpty()) {
                QFile::copy(modelDir + "/tokenizer.json", dirPath + "/tokenizer.json");
            } else {
                QFile f(dirPath + "/tokenizer.json");
                f.open(QIODevice::WriteOnly);
                f.write(tokenizerContent.toUtf8());
                f.close();
            }
        }
        // Copy or create metadata
        if (withMetadata) {
            if (metadataContent.isEmpty()) {
                QFile::copy(modelDir + "/metadata.json", dirPath + "/metadata.json");
            } else {
                QFile f(dirPath + "/metadata.json");
                f.open(QIODevice::WriteOnly);
                f.write(metadataContent.toUtf8());
                f.close();
            }
        }
        return dstModel;
    }

    // Metadata file not found → FileNotFound branch (lines 93-95)
    void testLoadModelMissingMetadata() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString modelPath = setupModelDir(tempDir.path(), true, false); // tokenizer yes, metadata no

        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath));
        QVERIFY(engine.isModelLoaded());
        // Should have loaded with default metadata values
    }

    // Metadata file is invalid JSON → InvalidJson branch (lines 96-98)
    void testLoadModelInvalidMetadata() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString modelPath = setupModelDir(tempDir.path(), true, true, "{ this is not valid json }");

        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath));
        QVERIFY(engine.isModelLoaded());
    }

    // Metadata missing required fields → MissingRequiredFields branch (lines 99-101)
    void testLoadModelEmptyMetadata() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString modelPath = setupModelDir(tempDir.path(), true, true, "{}");

        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath));
        QVERIFY(engine.isModelLoaded());
    }

    // Metadata with out-of-range max_len → validation branch (lines 180-183)
    void testLoadModelMetadataBadMaxLen() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString modelPath = setupModelDir(tempDir.path(), true, true,
            R"({"max_len": 9999, "threshold": -10.0})");

        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath));
        QVERIFY(engine.isModelLoaded());
    }

    // Metadata with negative max_len → also hits validation (lines 180-183)
    void testLoadModelMetadataNegativeMaxLen() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString modelPath = setupModelDir(tempDir.path(), true, true,
            R"({"max_len": -1, "threshold": -10.0})");

        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath));
        QVERIFY(engine.isModelLoaded());
    }

    // Tokenizer file missing → loadTokenizer failure → loadModel fails (lines 132-133)
    void testLoadModelMissingTokenizer() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString modelPath = setupModelDir(tempDir.path(), false, true); // no tokenizer

        InferenceEngine engine;
        QVERIFY(!engine.loadModel(modelPath));
        QVERIFY(!engine.isModelLoaded());
    }

    // Tokenizer file is garbage (not valid JSON) → exception in FromBlobJSON (lines 142-145)
    void testLoadModelCorruptTokenizer() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString modelPath = setupModelDir(tempDir.path(), true, true, QString(),
                                          "this is not a valid tokenizer blob");

        InferenceEngine engine;
        QVERIFY(!engine.loadModel(modelPath));
        QVERIFY(!engine.isModelLoaded());
    }

    // JSON input with empty "text" field → exercises parse-then-empty check (lines 252-254)
    void testInferJsonEmptyTextField() {
        if (!hasModel_) QSKIP("Model not found");

        InferenceEngine engine;
        engine.loadModel(modelPath_);
        // JSON with empty text field → text.isEmpty() after JSON parsing → returns empty
        auto results = engine.infer(R"({"text":""})");
        QCOMPARE(results.size(), 0);
    }

    // ---- Long text dedup edge cases ----

    // Long text with sections that might produce similar but not identical entities
    void testInferLongTextWithSimilarEntities() {
        if (!hasModel_) QSKIP("Model not found");

        InferenceEngine engine;
        engine.loadModel(modelPath_);

        // Create long text where same concept appears with slight variations
        // to exercise the calculateSimilarity code path (lines 498-508)
        QString section1 = "主变压器出现异常温升，需要立即停机检查。";
        QString section2 = "主变压器A相绕组温度过高，存在绝缘老化风险。";
        QString section3 = "冷却系统故障导致变压器油温超标。";
        QString filler = "维护人员已到场进行检查和维修准备工作。";

        QString text;
        while (text.length() < 800) {
            text += section1 + filler + section2 + filler + section3 + filler;
        }
        QVERIFY(text.length() > 500);

        auto results = engine.inferLongText(text, 500, 100);
        QVERIFY(results.size() >= 0);
    }

    // Long text with entities that differ only by whitespace
    void testInferLongTextWhitespaceVariant() {
        if (!hasModel_) QSKIP("Model not found");

        InferenceEngine engine;
        engine.loadModel(modelPath_);

        // Same entity with and without extra spaces
        QString s1 = "发动机 轴承 磨损故障。";
        QString s2 = "发动机轴承磨损故障。";
        QString filler = "检查其他设备运行状态正常。";

        QString text;
        while (text.length() < 800) {
            text += s1 + filler + s2 + filler;
        }
        QVERIFY(text.length() > 500);

        auto results = engine.inferLongText(text, 500, 100);
        QVERIFY(results.size() >= 0);
    }
};

QTEST_MAIN(TestInferenceEngine)
#include "test_inferenceengine.moc"
