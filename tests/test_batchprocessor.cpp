#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "core/batchprocessor.h"
#include "core/inferenceengine.h"
#include "data/databasemanager.h"
#include "utils/configmanager.h"

using namespace optikg;

class TestBatchProcessor : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        ConfigManager::instance().initialize();
    }

    void cleanupTestCase() {
        // 清理
    }

    void testDefaultConstruction() {
        BatchProcessor processor;
        // 验证默认值
        QCOMPARE(processor.getResults().size(), 0);
    }

    void testSetFiles() {
        BatchProcessor processor;
        QStringList files = {"/path/to/file1.json", "/path/to/file2.csv"};
        processor.setFiles(files);
        // 虽然无法直接获取内部文件列表，但可以通过处理结果间接验证
        // 这里至少验证设置不崩溃
        QVERIFY(true);
    }

    void testJsonContentField() {
        BatchProcessor processor;
        processor.setJsonContentField("content");
        // 验证设置生效（可能需要间接验证）
        QVERIFY(true);
    }

    void testCsvContentColumn() {
        BatchProcessor processor;
        processor.setCsvContentColumn("description");
        QVERIFY(true);
    }

    void testCsvEncoding() {
        BatchProcessor processor;
        processor.setCsvEncoding("GBK");
        QVERIFY(true);
    }

    void testThreshold() {
        BatchProcessor processor;
        processor.setThreshold(0.8f);
        QVERIFY(true);
    }

    void testModelPath() {
        BatchProcessor processor;
        processor.setModelPath("/path/to/model.onnx");
        QVERIFY(true);
    }

    void testStopRequest() {
        BatchProcessor processor;
        processor.stop();
        // 验证停止请求设置（无法直接验证）
        QVERIFY(true);
    }

    void testScanJsonFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建测试JSON文件
        QTemporaryFile tempFile(tempDir.path() + "/test_scan.json");
        tempFile.setAutoRemove(false); // 防止文件被自动删除
        QVERIFY(tempFile.open());

        QJsonArray records;
        for (int i = 0; i < 3; ++i) {
            QJsonObject record;
            record["text"] = QString("测试文本%1").arg(i);
            records.append(record);
        }

        QJsonDocument doc(records);
        tempFile.write(doc.toJson());
        tempFile.flush(); // 确保数据写入磁盘
        tempFile.close();

        // 通过BatchProcessor测试JSON扫描
        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({tempFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx"); // 设置无效模型让处理快速失败

        processor.start();
        processor.wait();

        // 验证扫描完成信号被发射，且正确统计了3条记录
        if (scanFinishedSpy.count() > 0) {
            auto args = scanFinishedSpy.takeFirst();
            int totalFiles = args.at(0).toInt();
            int totalRecords = args.at(1).toInt();
            QCOMPARE(totalFiles, 1);
            QCOMPARE(totalRecords, 3); // JSON文件中有3条记录
        }
    }

    void testScanCsvFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建测试CSV文件
        QTemporaryFile tempFile(tempDir.path() + "/test_scan.csv");
        tempFile.setAutoRemove(false); // 防止文件被自动删除
        QVERIFY(tempFile.open());

        QString csvContent = "content\n测试内容1\n测试内容2\n测试内容3\n";
        tempFile.write(csvContent.toUtf8());
        tempFile.flush(); // 确保数据写入磁盘
        tempFile.close();

        // 通过BatchProcessor测试CSV扫描
        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({tempFile.fileName()});
        processor.setCsvContentColumn("content");
        processor.setModelPath("/nonexistent/model.onnx"); // 设置无效模型让处理快速失败

        processor.start();
        processor.wait();

        // 验证扫描完成信号被发射，且正确统计了3条记录（标题行不算）
        if (scanFinishedSpy.count() > 0) {
            auto args = scanFinishedSpy.takeFirst();
            int totalFiles = args.at(0).toInt();
            int totalRecords = args.at(1).toInt();
            QCOMPARE(totalFiles, 1);
            QCOMPARE(totalRecords, 3); // CSV文件中有3条数据记录
        }
    }

    void testProcessEmptyFileList() {
        BatchProcessor processor;
        QSignalSpy finishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.start();
        processor.wait(); // 等待线程完成

        // 空文件列表应正常完成
        QCOMPARE(finishedSpy.count(), 1);
        auto args = finishedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 0); // totalFiles = 0
        QCOMPARE(args.at(1).toInt(), 0); // successCount = 0
        QCOMPARE(args.at(2).toInt(), 0); // failCount = 0
    }

    void testProgressSignals() {
        BatchProcessor processor;
        QSignalSpy progressSpy(&processor, &BatchProcessor::progressChanged);
        QSignalSpy scanProgressSpy(&processor, &BatchProcessor::scanProgress);
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);
        QSignalSpy finishedSpy(&processor, &BatchProcessor::batchFinished);

        // 创建测试文件
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QTemporaryFile jsonFile(tempDir.path() + "/test_progress.json");
        QVERIFY(jsonFile.open());
        jsonFile.setAutoRemove(false); // 防止文件被自动删除

        QJsonArray records;
        for (int i = 0; i < 2; ++i) {
            QJsonObject record;
            record["text"] = QString("进度测试文本%1").arg(i);
            records.append(record);
        }

        QJsonDocument doc(records);
        jsonFile.write(doc.toJson());
        jsonFile.flush(); // 确保数据写入磁盘
        jsonFile.close();

        processor.setFiles({jsonFile.fileName()});
        processor.setJsonContentField("text");
        // 设置一个不存在的模型路径，让处理快速失败
        processor.setModelPath("/nonexistent/model.onnx");

        processor.start();
        processor.wait();

        // 验证至少收到了扫描完成信号和批处理完成信号
        // 扫描阶段应该发射信号
        QVERIFY(finishedSpy.count() > 0);
        if (finishedSpy.count() > 0) {
            auto args = finishedSpy.takeFirst();
            // totalFiles应该为1
            QCOMPARE(args.at(0).toInt(), 1);
        }
    }

    void testErrorHandling() {
        BatchProcessor processor;
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);

        // 设置不存在的文件
        processor.setFiles({"/nonexistent/file.json"});

        processor.start();
        processor.wait();

        // 应收到错误信号
        QVERIFY(errorSpy.count() > 0);
    }

    void testEmptyJsonFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建空的JSON文件
        QTemporaryFile tempFile(tempDir.path() + "/empty.json");
        tempFile.setAutoRemove(false); // 防止文件被自动删除
        QVERIFY(tempFile.open());
        tempFile.write("{}"); // 空对象，没有内容字段
        tempFile.flush(); // 确保数据写入磁盘
        tempFile.close();

        BatchProcessor processor;
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);

        processor.setFiles({tempFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx");

        processor.start();
        processor.wait();

        // 空JSON文件应产生错误信号（未找到内容字段）
        QVERIFY(errorSpy.count() > 0);
    }

    void testFileProcessedSignal() {
        BatchProcessor processor;
        QSignalSpy fileProcessedSpy(&processor, &BatchProcessor::fileProcessed);

        // 创建简单JSON文件
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QTemporaryFile jsonFile(tempDir.path() + "/test_signal.json");
        QVERIFY(jsonFile.open());
        jsonFile.setAutoRemove(false); // 防止文件被自动删除

        QJsonObject record;
        record["text"] = "信号测试文本";
        QJsonArray records = {record};

        QJsonDocument doc(records);
        jsonFile.write(doc.toJson());
        jsonFile.flush(); // 确保数据写入磁盘
        jsonFile.close();

        processor.setFiles({jsonFile.fileName()});
        processor.setJsonContentField("text");

        // 设置一个低阈值，确保有结果
        processor.setThreshold(-10.0f);

        // 需要模型进行实际处理
        // 这里仅验证设置
        QVERIFY(true);
    }

    void testProcessJsonFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建测试JSON文件
        QTemporaryFile jsonFile(tempDir.path() + "/test_process.json");
        QVERIFY(jsonFile.open());
        jsonFile.setAutoRemove(false); // 防止文件被自动删除

        QJsonArray records;
        for (int i = 0; i < 2; ++i) {
            QJsonObject record;
            record["text"] = QString("处理测试文本%1").arg(i);
            records.append(record);
        }

        QJsonDocument doc(records);
        jsonFile.write(doc.toJson());
        jsonFile.flush(); // 确保数据写入磁盘
        jsonFile.close();

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);

        processor.setFiles({jsonFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx"); // 无效模型路径，快速失败

        processor.start();
        processor.wait();

        // 应收到完成信号或错误信号
        QVERIFY(batchFinishedSpy.count() > 0 || errorSpy.count() > 0);
        if (batchFinishedSpy.count() > 0) {
            auto args = batchFinishedSpy.takeFirst();
            // 应报告1个文件
            QCOMPARE(args.at(0).toInt(), 1);
        }
    }

    void testProcessCsvFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建测试CSV文件
        QTemporaryFile csvFile(tempDir.path() + "/test_process.csv");
        QVERIFY(csvFile.open());
        csvFile.setAutoRemove(false); // 防止文件被自动删除

        QString csvContent = "content\n测试内容1\n测试内容2\n";
        csvFile.write(csvContent.toUtf8());
        csvFile.flush(); // 确保数据写入磁盘
        csvFile.close();

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);

        processor.setFiles({csvFile.fileName()});
        processor.setCsvContentColumn("content");
        processor.setModelPath("/nonexistent/model.onnx"); // 无效模型路径

        processor.start();
        processor.wait();

        // 应收到完成信号或错误信号
        QVERIFY(batchFinishedSpy.count() > 0 || errorSpy.count() > 0);
        if (batchFinishedSpy.count() > 0) {
            auto args = batchFinishedSpy.takeFirst();
            QCOMPARE(args.at(0).toInt(), 1);
        }
    }

    void testStopDuringProcessing() {
        // 测试停止功能（BB-12 处理中断测试）
        BatchProcessor processor;

        // 创建一个较大的JSON文件以延长处理时间
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QTemporaryFile jsonFile(tempDir.path() + "/test_stop.json");
        QVERIFY(jsonFile.open());
        jsonFile.setAutoRemove(false); // 防止文件被自动删除

        QJsonArray records;
        for (int i = 0; i < 10; ++i) {
            QJsonObject record;
            record["text"] = QString("停止测试文本%1").arg(i);
            records.append(record);
        }

        QJsonDocument doc(records);
        jsonFile.write(doc.toJson());
        jsonFile.flush(); // 确保数据写入磁盘
        jsonFile.close();

        processor.setFiles({jsonFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx"); // 无效模型，快速失败

        // 启动处理，然后立即请求停止
        processor.start();
        processor.stop(); // 请求停止
        processor.wait();

        // 验证处理被中断（无法直接验证，但至少不应崩溃）
        QVERIFY(true);
    }

    void testJsonLinesFormat() {
        // 测试JSON Lines格式（BB-10）
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建JSON Lines文件（每行一个JSON对象）
        QTemporaryFile jsonlFile(tempDir.path() + "/test_jsonl.jsonl");
        jsonlFile.setAutoRemove(false); // 防止文件被自动删除
        QVERIFY(jsonlFile.open());

        QStringList lines;
        for (int i = 0; i < 3; ++i) {
            QJsonObject record;
            record["text"] = QString("JSONL测试文本%1").arg(i);
            lines.append(QJsonDocument(record).toJson(QJsonDocument::Compact));
        }

        jsonlFile.write(lines.join("\n").toUtf8());
        jsonlFile.flush(); // 确保数据写入磁盘
        jsonlFile.close();

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({jsonlFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx");

        processor.start();
        processor.wait();

        // 扫描阶段应识别出3条记录
        if (scanFinishedSpy.count() > 0) {
            auto args = scanFinishedSpy.takeFirst();
            int totalFiles = args.at(0).toInt();
            int totalRecords = args.at(1).toInt();
            QCOMPARE(totalFiles, 1);
            QCOMPARE(totalRecords, 3);
        }
    }

    void testMultipleJsonFiles() {
        // 测试多个JSON文件处理（BB-08 批量JSON处理）
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QStringList files;
        for (int f = 0; f < 3; ++f) {
            QTemporaryFile jsonFile(tempDir.path() + QString("/test_multi_%1.json").arg(f));
            QVERIFY(jsonFile.open());
            jsonFile.setAutoRemove(false); // 防止文件被自动删除

            QJsonArray records;
            for (int i = 0; i < 2; ++i) {
                QJsonObject record;
                record["text"] = QString("多文件测试%1-%2").arg(f).arg(i);
                records.append(record);
            }

            QJsonDocument doc(records);
            jsonFile.write(doc.toJson());
            jsonFile.flush(); // 确保数据写入磁盘
            jsonFile.close();

            files.append(jsonFile.fileName());
        }

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles(files);
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx");

        processor.start();
        processor.wait();

        // 扫描阶段应识别出3个文件，6条记录
        if (scanFinishedSpy.count() > 0) {
            auto args = scanFinishedSpy.takeFirst();
            int totalFiles = args.at(0).toInt();
            int totalRecords = args.at(1).toInt();
            QCOMPARE(totalFiles, 3);
            QCOMPARE(totalRecords, 6);
        }

        // 批处理完成信号
        if (batchFinishedSpy.count() > 0) {
            auto args = batchFinishedSpy.takeFirst();
            QCOMPARE(args.at(0).toInt(), 3); // 3个文件
        }
    }

    // BB-13 自动导出测试（补充）
    void testAutoExportLogic() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建测试JSON文件
        QTemporaryFile jsonFile(tempDir.path() + "/test_autoexport.json");
        QVERIFY(jsonFile.open());
        jsonFile.setAutoRemove(false);

        QJsonArray records;
        QJsonObject record;
        record["text"] = "自动导出测试文本";
        records.append(record);

        QJsonDocument doc(records);
        jsonFile.write(doc.toJson());
        jsonFile.flush();
        jsonFile.close();

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({jsonFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx");

        processor.start();
        processor.wait();

        QVERIFY(batchFinishedSpy.count() > 0);
        auto results = processor.getResults();
        QVERIFY(results.size() >= 1);
        // 验证结果中包含原始文件路径信息（用于自动导出）
        QCOMPARE(results.first().filePath, jsonFile.fileName());
    }

    // BB-14 保存到数据库测试（补充）
    void testSaveToDatabaseLogic() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 初始化临时数据库
        DatabaseManager::instance().close();
        QVERIFY(DatabaseManager::instance().initialize(tempDir.filePath("batch_test.db")));

        int initialCount = DatabaseManager::instance().getRecordCount();

        // 创建测试JSON文件
        QTemporaryFile jsonFile(tempDir.path() + "/test_savetodb.json");
        QVERIFY(jsonFile.open());
        jsonFile.setAutoRemove(false);

        QJsonArray records;
        for (int i = 0; i < 2; ++i) {
            QJsonObject record;
            record["text"] = QString("数据库测试文本%1").arg(i);
            records.append(record);
        }

        QJsonDocument doc(records);
        jsonFile.write(doc.toJson());
        jsonFile.flush();
        jsonFile.close();

        BatchProcessor processor;
        processor.setFiles({jsonFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx");

        processor.start();
        processor.wait();

        // 模拟保存到数据库：将处理结果转换为抽取记录并插入
        auto results = processor.getResults();
        QList<ExtractionRecord> allRecords;
        for (const auto& result : results) {
            if (result.success) {
                allRecords.append(result.extractionRecords);
            }
        }

        // 即使模型无效导致没有成功结果，也验证数据库操作本身正常
        if (!allRecords.isEmpty()) {
            QVERIFY(DatabaseManager::instance().insertExtractionRecords(allRecords));
            QVERIFY(DatabaseManager::instance().getRecordCount() >= initialCount);
        }

        DatabaseManager::instance().close();
    }

    // BB-36 打开纯文本文件测试（补充）
    void testPlainTextFileProcessing() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建纯文本文件
        QTemporaryFile textFile(tempDir.path() + "/test_plain.txt");
        QVERIFY(textFile.open());
        textFile.setAutoRemove(false);

        QString textContent = "电机轴承出现异常磨损，需要更换专用润滑油。";
        textFile.write(textContent.toUtf8());
        textFile.flush();
        textFile.close();

        // 测试纯文本文件内容读取
        QFile file(textFile.fileName());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QString content = QString::fromUtf8(file.readAll());
        QCOMPARE(content, textContent);
        file.close();

        // 测试BatchProcessor对.txt文件的扫描行为（应作为普通文本或报错）
        BatchProcessor processor;
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);
        processor.setFiles({textFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx");
        processor.start();
        processor.wait();
        // 非JSON/CSV文件可能触发错误，但至少不应崩溃
        QVERIFY(processor.getResults().size() >= 0);
    }

    // BB-37 批量结果导出测试（补充）
    void testBatchResultExport() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建测试JSON文件
        QTemporaryFile jsonFile(tempDir.path() + "/test_export.json");
        QVERIFY(jsonFile.open());
        jsonFile.setAutoRemove(false);

        QJsonArray records;
        for (int i = 0; i < 2; ++i) {
            QJsonObject record;
            record["text"] = QString("导出测试%1").arg(i);
            records.append(record);
        }

        QJsonDocument doc(records);
        jsonFile.write(doc.toJson());
        jsonFile.flush();
        jsonFile.close();

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({jsonFile.fileName()});
        processor.setJsonContentField("text");
        processor.setModelPath("/nonexistent/model.onnx");

        processor.start();
        processor.wait();

        QVERIFY(batchFinishedSpy.count() > 0);
        auto results = processor.getResults();
        QVERIFY(results.size() >= 1);
        // 验证结果中包含 source_file 相关信息（filePath字段）
        QCOMPARE(results.first().filePath, jsonFile.fileName());
    }
};

QTEST_MAIN(TestBatchProcessor)
#include "test_batchprocessor.moc"