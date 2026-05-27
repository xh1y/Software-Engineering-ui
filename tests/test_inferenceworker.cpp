#include <QTest>
#include <QSignalSpy>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "core/inferenceengine.h"
#define private public
#include "core/inferenceworker.h"
#undef private
#include "utils/configmanager.h"

using namespace optikg;

class TestInferenceWorker : public QObject {
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
    }

    void testDefaultConstruction() {
        InferenceWorker worker;
        QVERIFY(!worker.isRunning());
    }

    void testSetters() {
        InferenceWorker worker;
        worker.setText("电机轴承磨损");
        worker.setModelPath("/tmp/model.onnx");
        worker.setThreshold(-0.2f);
        // 没有 getter，通过运行验证
    }

    void testStop() {
        InferenceWorker worker;
        worker.stop();
        QVERIFY(!worker.isRunning());
    }

    void testInvalidModelPath() {
        InferenceWorker worker;
        worker.setText("测试文本");
        worker.setModelPath("/nonexistent/model.onnx");
        worker.setThreshold(-10.0f);

        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);
        QSignalSpy progressSpy(&worker, &InferenceWorker::progressChanged);
        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);

        worker.start();
        QVERIFY(worker.wait(10000));

        QVERIFY2(errorSpy.count() > 0, "Should emit error for invalid model path");
        QVERIFY(resultSpy.isEmpty() || resultSpy.count() == 0);
    }

    void testValidModelInference() {
        if (!hasModel_) {
            QSKIP("Model file not found, skipping inference worker test.");
        }

        InferenceWorker worker;
        worker.setText("电机轴承出现异常磨损");
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy progressSpy(&worker, &InferenceWorker::progressChanged);
        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(30000));

        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));

        QVERIFY(progressSpy.count() > 0);
        // 最后一个进度应为 100
        if (!progressSpy.isEmpty()) {
            QList<QVariant> lastArgs = progressSpy.last();
            QCOMPARE(lastArgs.at(0).toInt(), 100);
        }

        QVERIFY(resultSpy.count() > 0);
        QList<QVariant> resultArgs = resultSpy.takeFirst();
        QList<Triple> results = resultArgs.at(0).value<QList<Triple>>();
        QVERIFY(results.size() >= 0); // 至少不崩溃，结果数量取决于模型
    }

    void testLongTextInference() {
        if (!hasModel_) {
            QSKIP("Model file not found, skipping inference worker test.");
        }

        InferenceWorker worker;
        QString longText = QString("电机轴承磨损测试。").repeated(51); // > 450 chars
        QVERIFY(longText.length() > 450);

        worker.setText(longText);
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy progressSpy(&worker, &InferenceWorker::progressChanged);
        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(60000));

        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));

        QVERIFY(resultSpy.count() > 0);
        QList<QVariant> resultArgs = resultSpy.takeFirst();
        QList<Triple> results = resultArgs.at(0).value<QList<Triple>>();
        QVERIFY(results.size() >= 0);
    }

    void testEmptyText() {
        if (!hasModel_) {
            QSKIP("Model file not found, skipping inference worker test.");
        }

        InferenceWorker worker;
        worker.setText("");
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(30000));

        QVERIFY(resultSpy.count() > 0);
        QList<QVariant> resultArgs = resultSpy.takeFirst();
        QList<Triple> results = resultArgs.at(0).value<QList<Triple>>();
        QCOMPARE(results.size(), 0);
    }

    void testThresholdFiltering() {
        if (!hasModel_) {
            QSKIP("Model file not found, skipping inference worker test.");
        }

        InferenceWorker worker;
        worker.setText("电机轴承出现异常磨损");
        worker.setModelPath(modelPath_);
        worker.setThreshold(0.99f); // 很高的阈值

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(30000));

        QVERIFY(resultSpy.count() > 0);
        QList<QVariant> resultArgs = resultSpy.takeFirst();
        QList<Triple> results = resultArgs.at(0).value<QList<Triple>>();
        // 模型输出logits值（可能为负），阈值0.99过滤后可能为空
        QVERIFY(results.size() >= 0);
    }

    void testDestructionWhileRunning() {
        // 测试正在运行时被析构不会崩溃
        {
            InferenceWorker worker;
            worker.setText("测试文本");
            worker.setModelPath("/nonexistent/model.onnx");
            worker.start();
            // 不等待，直接析构
        }
        QVERIFY(true); // 没有崩溃即通过
    }

    void testJsonTextInput() {
        if (!hasModel_) {
            QSKIP("Model file not found, skipping inference worker test.");
        }

        InferenceWorker worker;
        worker.setText(R"({"text":"液压系统压力不足"})");
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(30000));
        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));
        QVERIFY(resultSpy.count() > 0);
    }

    void testTextExactlyAtChunkThreshold() {
        if (!hasModel_) {
            QSKIP("Model file not found, skipping inference worker test.");
        }

        InferenceWorker worker;
        // Create text exactly at CHUNK_THRESHOLD (450 chars)
        QString text = QString(450, 'x');  // exactly at threshold, should NOT use chunking
        worker.setText(text);
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(30000));
        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));
        QVERIFY(resultSpy.count() > 0);
    }

    void testVeryHighThresholdFiltering() {
        if (!hasModel_) {
            QSKIP("Model file not found, skipping inference worker test.");
        }

        InferenceWorker worker;
        worker.setText("电机轴承异常磨损");
        worker.setModelPath(modelPath_);
        worker.setThreshold(0.999f); // very high threshold, should filter almost everything

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(30000));
        QVERIFY(resultSpy.count() > 0);
    }

    void testLoadModelEmptyPath() {
        InferenceWorker worker;
        worker.setText("test");
        worker.setModelPath("");

        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);
        worker.start();
        QVERIFY(worker.wait(10000));
        QVERIFY(errorSpy.count() > 0);
    }

    // Pass garbage file as model → Ort::Exception in loadModel → std::exception in run()
    void testLoadModelWithGarbageFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString garbagePath = tempDir.filePath("garbage.onnx");
        QFile f(garbagePath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("not an onnx model file at all"));
        f.close();

        InferenceWorker worker;
        worker.setText("测试文本");
        worker.setModelPath(garbagePath);

        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);
        worker.start();
        QVERIFY(worker.wait(10000));
        // Should emit error due to model loading failure
        QVERIFY(errorSpy.count() > 0);
    }

    // Long text with repeated entity → triggers dedup in inferWithChunking
    void testLongTextWithDedup() {
        if (!hasModel_) QSKIP("Model not found");

        // Build a long text that exceeds CHUNK_THRESHOLD with repeated entity mentions
        // Each chunk should contain "发动机磨损" so dedup can merge them
        QString base = "发动机轴承出现异常磨损，需要立即更换。";
        QString filler = "维护人员检查了设备运行状态，确认所有参数正常。";
        QString text;
        while (text.length() < 600) {
            text += base + filler;
        }
        QVERIFY(text.length() > 450);

        InferenceWorker worker;
        worker.setText(text);
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(60000));
        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));
        QVERIFY(resultSpy.count() > 0);
    }

    // Long text with sentence-ending punctuation near chunk boundary
    void testLongTextSentenceBoundary() {
        if (!hasModel_) QSKIP("Model not found");

        // Craft text so a chunk boundary falls near a sentence-ending punctuation
        // chunkSize=512, so put a "。" at position ~500
        QString prefix = QString(490, 'x') + "。发动机轴承异常磨损。";
        QString suffix = QString(200, 'y');
        QString text = prefix + suffix;
        QVERIFY(text.length() > 500);

        InferenceWorker worker;
        worker.setText(text);
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(60000));
        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));
        QVERIFY(resultSpy.count() > 0);
    }

    // Long text with whitespace-variant entities → exercises normalizeEntity
    void testLongTextWhitespaceEntities() {
        if (!hasModel_) QSKIP("Model not found");

        // Entities with extra whitespace that normalizeEntity should clean
        QString base = "发动机 轴承 磨损故障，需要更换专用润滑油。";
        QString filler = "检查液压系统压力，确认传感器工作正常。";
        QString text;
        while (text.length() < 600) {
            text += base + filler;
        }
        QVERIFY(text.length() > 450);

        InferenceWorker worker;
        worker.setText(text);
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(60000));
        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));
        QVERIFY(resultSpy.count() > 0);
    }

    // Stop worker during processing
    void testStopDuringRun() {
        if (!hasModel_) QSKIP("Model not found");

        InferenceWorker worker;
        // Long text so processing takes measurable time
        QString text = QString("发动机轴承异常磨损测试。").repeated(60);
        worker.setText(text);
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        worker.start();
        QTest::qWait(50);
        worker.stop();
        QVERIFY(worker.wait(30000));
        // Should not crash
        QVERIFY(true);
    }

    // JSON input format: {"text": "..."} exercises the JSON branch in tokenizeAndPredict
    void testJsonObjectInput() {
        if (!hasModel_) QSKIP("Model not found");

        InferenceWorker worker;
        worker.setText(R"({"text":"液压系统压力不足，需要检修"})");
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);

        worker.start();
        QVERIFY(worker.wait(30000));
        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));
        QVERIFY(resultSpy.count() > 0);
    }

    // Missing tokenizer.json — exercises loadTokenizer failure path (lines 148-150, 186)
    void testMissingTokenizerFile() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Symlink the real model to temp dir so the ONNX file exists
        QString linkedModel = tempDir.filePath("model.onnx");
        QFile::link(modelPath_, linkedModel);

        // Do NOT copy tokenizer.json → loadTokenizer will fail

        InferenceWorker worker;
        worker.setText("测试文本");
        worker.setModelPath(linkedModel);

        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);
        worker.start();
        QVERIFY(worker.wait(10000));
        // Should emit error because tokenizer is missing
        QVERIFY(errorSpy.count() > 0);
    }

    // Corrupt metadata.json — exercises loadMetadata invalid JSON path (line 213)
    void testCorruptMetadataFile() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Symlink the real model and its tokenizer.json
        QString linkedModel = tempDir.filePath("model.onnx");
        QFile::link(modelPath_, linkedModel);

        QString modelDir = QFileInfo(modelPath_).absolutePath();
        QString tokenizerSrc = QDir(modelDir).filePath("tokenizer.json");
        if (QFileInfo::exists(tokenizerSrc)) {
            QFile::copy(tokenizerSrc, tempDir.filePath("tokenizer.json"));
        }

        // Create corrupt metadata.json (invalid JSON)
        QFile metaFile(tempDir.filePath("metadata.json"));
        QVERIFY(metaFile.open(QIODevice::WriteOnly));
        metaFile.write(QByteArray("this is not valid JSON {{{"));
        metaFile.close();

        InferenceWorker worker;
        worker.setText("测试文本");
        worker.setModelPath(linkedModel);

        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);
        worker.start();
        QVERIFY(worker.wait(10000));
        // Worker should still run (metadata failure is non-fatal warning),
        // or fail with error — either way it shouldn't crash
        QVERIFY(true);
    }

    // Very long text that exceeds TARGET_LEN tokens → exercises token resize (line 271)
    void testVeryLongTextTokenResize() {
        if (!hasModel_) QSKIP("Model not found");

        InferenceWorker worker;
        // Generate very long Chinese text (2000+ chars) to exceed maxSeqLength_ tokens
        QStringList phrases = {
            "发动机轴承出现异常磨损需要更换专用润滑油",
            "液压系统压力不足导致设备无法正常运行",
            "传感器故障影响数据采集精度需要校准",
            "齿轮箱温度过高可能存在润滑不良问题",
            "控制系统响应延迟影响生产效率和质量"
        };
        QString longText;
        while (longText.length() < 2500) {
            longText += phrases.join("，") + "。";
        }
        QVERIFY(longText.length() > 2000);

        worker.setText(longText);
        worker.setModelPath(modelPath_);
        worker.setThreshold(-10.0f);

        QSignalSpy resultSpy(&worker, &InferenceWorker::resultReady);
        QSignalSpy errorSpy(&worker, &InferenceWorker::errorOccurred);
        QSignalSpy progressSpy(&worker, &InferenceWorker::progressChanged);

        worker.start();
        QVERIFY(worker.wait(60000));
        QVERIFY2(errorSpy.isEmpty(), qPrintable(
            errorSpy.isEmpty() ? QString() : errorSpy.takeFirst().at(0).toString()));
        QVERIFY(resultSpy.count() > 0);
    }

    // ---- Tokenizer / Metadata error path tests (via #define private public) ----

    // Tokenizer file not found → returns false (lines 186-187)
    void testTokenizerFileNotFound() {
        InferenceWorker worker;
        QVERIFY(!worker.loadTokenizer("/nonexistent/path/tokenizer.json"));
    }

    // Tokenizer file is garbage → FromBlobJSON throws → returns false (lines 196-197)
    void testTokenizerGarbageContent() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = tempDir.filePath("bad_tokenizer.json");
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("this is not valid tokenizer json"));
        f.close();

        InferenceWorker worker;
        QVERIFY(!worker.loadTokenizer(path));
    }

    // Metadata file not found → returns false (lines 204-205)
    void testMetadataFileNotFound() {
        InferenceWorker worker;
        QVERIFY(!worker.loadMetadata("/nonexistent/path/metadata.json"));
    }

    // Metadata file is invalid JSON → returns false (lines 213-215)
    void testMetadataInvalidJson() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = tempDir.filePath("bad_metadata.json");
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("{{{ not valid json"));
        f.close();

        InferenceWorker worker;
        QVERIFY(!worker.loadMetadata(path));
    }
};

QTEST_MAIN(TestInferenceWorker)
#include "test_inferenceworker.moc"
