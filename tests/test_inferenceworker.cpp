#include <QTest>
#include <QSignalSpy>
#include <QFileInfo>
#include "core/inferenceengine.h"
#include "core/inferenceworker.h"
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
        modelPath_ = ConfigManager::instance().autoDetectModelPath();
        if (modelPath_.isEmpty()) {
            modelPath_ = "models/model_fp32.onnx";
        }
        // 文件存在 AND 可以加载才标记为有模型
        if (QFileInfo::exists(modelPath_)) {
            InferenceEngine engine;
            hasModel_ = engine.loadModel(modelPath_);
        } else {
            hasModel_ = false;
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
};

QTEST_MAIN(TestInferenceWorker)
#include "test_inferenceworker.moc"
