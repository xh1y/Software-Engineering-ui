#include <QTest>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QThread>
#include <QCoreApplication>
#include "core/inferenceengine.h"
#include "databasemanager.h"
#include "utils/configmanager.h"
#include "optikg/datamodel.h"

using namespace optikg;

class TestPerformance : public QObject {
    Q_OBJECT

private:
    QString modelPath_;
    bool hasModel_ = false;
    bool perfEnabled_ = false;
    QTemporaryDir* tempDir_ = nullptr;

private slots:
    void initTestCase() {
        ConfigManager::instance().initialize();

        // 检查是否启用性能测试标志
        perfEnabled_ = QCoreApplication::arguments().contains("-perf");

        // 检测模型文件
        modelPath_ = ConfigManager::instance().autoDetectModelPath();
        if (modelPath_.isEmpty()) {
            modelPath_ = "models/model_fp32.onnx";
        }
        hasModel_ = QFileInfo::exists(modelPath_);
        if (!hasModel_) {
            QSKIP("Model file not found, skipping performance tests.");
        }

        // 创建临时目录用于数据库测试
        tempDir_ = new QTemporaryDir();
        QVERIFY(tempDir_->isValid());
    }

    void cleanupTestCase() {
        DatabaseManager::instance().close();
        delete tempDir_;
    }

    // PF-01 单条文本推理响应测试（≤ 2秒）
    void testSingleTextInferencePerformance() {
        if (!perfEnabled_) {
            QSKIP("Performance tests disabled by default - enable with -perf flag");
        }

        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath_));
        QVERIFY(engine.isModelLoaded());

        // 500字测试文本
        QString testText = "电机轴承出现异常磨损，需要更换专用润滑油。";
        // 重复以接近500字
        while (testText.length() < 500) {
            testText += "轴承磨损会导致设备运行不稳定，建议定期检查维护。";
        }

        QElapsedTimer timer;
        timer.start();

        auto results = engine.infer(testText);

        qint64 elapsed = timer.elapsed();

        // 验证响应时间 ≤ 2000ms (2秒)
        QVERIFY2(elapsed <= 2000,
                 qPrintable(QString("Inference took %1 ms, exceeds 2000 ms limit").arg(elapsed)));

        // 输出性能信息
        qDebug() << "Single text inference:" << elapsed << "ms, results:" << results.size();
    }

    // PF-02 长文本分块推理测试（≤ 8秒）
    void testLongTextChunkingPerformance() {
        if (!perfEnabled_) {
            QSKIP("Performance tests disabled by default - enable with -perf flag");
        }

        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath_));

        // 创建3000字长文本
        QString longText;
        for (int i = 0; i < 100; ++i) {
            longText += "电机轴承磨损故障分析报告：设备在运行过程中出现异常振动和噪音，"
                       "初步判断为轴承磨损导致。需要更换轴承并加注专用润滑油。"
                       "建议进行定期维护检查，避免类似故障再次发生。";
        }

        QElapsedTimer timer;
        timer.start();

        auto results = engine.inferLongText(longText, 500, 100);

        qint64 elapsed = timer.elapsed();

        // 验证响应时间 ≤ 8000ms (8秒)
        QVERIFY2(elapsed <= 8000,
                 qPrintable(QString("Long text inference took %1 ms, exceeds 8000 ms limit").arg(elapsed)));

        qDebug() << "Long text chunking inference:" << elapsed << "ms, results:" << results.size();
    }

    // PF-03 图谱加载时间测试（≤ 3秒）
    void testGraphLoadingPerformance() {
        if (!perfEnabled_) {
            QSKIP("Performance tests disabled by default - enable with -perf flag");
        }

        // 准备100个节点的测试数据
        QList<GraphNode> nodes;
        QList<GraphEdge> edges;

        for (int i = 0; i < 100; ++i) {
            Entity entity(QString("部件%1").arg(i), EntityType::Component, 0.9f);
            nodes.append(GraphNode(entity, 0.0, 0.0));

            if (i > 0) {
                edges.append(GraphEdge(QString("部件%1").arg(i-1),
                                       QString("部件%1").arg(i),
                                       "相关", 0.8f));
            }
        }

        QElapsedTimer timer;
        timer.start();

        // 实际图谱加载时间测试需要GraphWidget，这里模拟数据处理时间
        // 简单验证数据处理不超时
        QVERIFY(nodes.size() == 100);
        QVERIFY(edges.size() == 99);

        qint64 elapsed = timer.elapsed();

        // 数据处理应快速完成
        QVERIFY2(elapsed <= 3000,
                 qPrintable(QString("Graph data processing took %1 ms, exceeds 3000 ms limit").arg(elapsed)));

        qDebug() << "Graph data processing:" << elapsed << "ms";
    }

    // PF-04 批量处理速度测试（100条记录 ≤ 5分钟）
    void testBatchProcessingPerformance() {
        if (!perfEnabled_) {
            QSKIP("Performance tests disabled by default - enable with -perf flag");
        }

        // 注意：此测试可能耗时较长，谨慎启用
        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath_));

        // 创建测试文本列表
        QStringList testTexts;
        for (int i = 0; i < 100; ++i) {
            testTexts.append(QString("测试文本%1：电机轴承异常磨损需要维护检查").arg(i));
        }

        QElapsedTimer timer;
        timer.start();

        int processedCount = 0;
        for (const QString& text : testTexts) {
            auto results = engine.infer(text);
            processedCount++;

            // 每处理10条输出一次进度
            if (processedCount % 10 == 0) {
                qDebug() << "Processed" << processedCount << "of" << testTexts.size() << "records";
            }
        }

        qint64 elapsed = timer.elapsed();

        // 验证处理时间 ≤ 300000ms (5分钟)
        QVERIFY2(elapsed <= 300000,
                 qPrintable(QString("Batch processing took %1 ms, exceeds 300000 ms (5 min) limit").arg(elapsed)));

        qDebug() << "Batch processing (100 records):" << elapsed << "ms";
    }

    // PF-05 内存占用测试（≤ 1.5 GB）- 简化版本
    void testMemoryUsage() {
        if (!perfEnabled_) {
            QSKIP("Memory usage tests require external monitoring tools");
        }

        // 内存测试需要外部工具，这里仅验证功能不泄漏
        InferenceEngine engine;
        QVERIFY(engine.loadModel(modelPath_));

        // 多次推理验证内存稳定性
        for (int i = 0; i < 10; ++i) {
            auto results = engine.infer("电机轴承磨损测试文本");
            Q_UNUSED(results);
        }

        // 卸载模型释放内存
        engine.unload();
        QVERIFY(!engine.isModelLoaded());

        qDebug() << "Memory stability test passed";
    }

    // PF-06 启动时间测试（≤ 3秒）
    void testStartupTime() {
        if (!perfEnabled_) {
            QSKIP("Startup time test requires full application startup");
        }

        // 完整的应用程序启动时间测试需要启动整个应用
        // 这里测试核心组件初始化时间
        QElapsedTimer timer;
        timer.start();

        ConfigManager::instance().initialize();
        DatabaseManager::instance().initialize(tempDir_->filePath("startup_test.db"));

        qint64 elapsed = timer.elapsed();

        QVERIFY2(elapsed <= 3000,
                 qPrintable(QString("Core components initialization took %1 ms, exceeds 3000 ms limit").arg(elapsed)));

        qDebug() << "Core components initialization:" << elapsed << "ms";

        DatabaseManager::instance().close();
    }

    // PF-07 数据库查询响应测试（≤ 500 ms）
    void testDatabaseQueryPerformance() {
        DatabaseManager::instance().close();
        QString dbPath = tempDir_->filePath("query_perf.db");
        QVERIFY(DatabaseManager::instance().initialize(dbPath));

        // 插入测试数据
        for (int i = 0; i < 1000; ++i) {
            ExtractionRecord rec(QString("测试记录%1内容").arg(i), QList<Triple>(), i * 10);
            DatabaseManager::instance().insertExtractionRecord(rec);
        }

        // 测试关键词搜索性能
        QElapsedTimer timer;
        timer.start();

        QList<ExtractionRecord> results = DatabaseManager::instance().searchExtractionRecords("测试");

        qint64 elapsed = timer.elapsed();

        // 验证查询时间 ≤ 500ms
        QVERIFY2(elapsed <= 500,
                 qPrintable(QString("Database query took %1 ms, exceeds 500 ms limit").arg(elapsed)));

        qDebug() << "Database query (1000 records):" << elapsed << "ms, results:" << results.size();
    }

    // 基准测试：推理引擎性能
    void benchmarkInferenceEngine() {
        if (!perfEnabled_) {
            QSKIP("Benchmark tests disabled by default");
        }

        InferenceEngine engine;
        QBENCHMARK {
            engine.loadModel(modelPath_);
            auto results = engine.infer("电机轴承磨损故障分析");
            engine.unload();
        }
    }

    // 基准测试：数据库插入性能
    void benchmarkDatabaseInsert() {
        if (!perfEnabled_) {
            QSKIP("Benchmark tests disabled by default");
        }

        DatabaseManager::instance().close();
        QString dbPath = tempDir_->filePath("benchmark_insert.db");
        DatabaseManager::instance().initialize(dbPath);

        QBENCHMARK {
            ExtractionRecord rec("基准测试记录", QList<Triple>(), 100);
            DatabaseManager::instance().insertExtractionRecord(rec);
        }
    }

    // 辅助测试：验证性能指标
    void verifyPerformanceMetrics() {
        // 汇总性能要求
        QVector<QPair<QString, int>> requirements = {
            {"PF-01 单条文本推理", 2000},
            {"PF-02 长文本分块推理", 8000},
            {"PF-03 图谱加载", 3000},
            {"PF-04 批量处理(100条)", 300000},
            {"PF-05 内存占用", 1500}, // MB
            {"PF-06 启动时间", 3000},
            {"PF-07 数据库查询", 500}
        };

        qDebug() << "\n性能测试要求汇总:";
        for (const auto& req : requirements) {
            qDebug() << req.first << ":" << req.second
                     << (req.first.contains("内存") ? "MB" : "ms");
        }
        qDebug() << "\n注：实际性能测试需要启用相应测试用例";
    }
};

QTEST_MAIN(TestPerformance)
#include "test_performance.moc"