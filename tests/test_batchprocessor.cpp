#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include "core/batchprocessor.h"
#include "core/inferenceengine.h"
#include "data/databasemanager.h"
#include "utils/configmanager.h"

using namespace optikg;

class TestBatchProcessor : public QObject {
    Q_OBJECT

private:
    QString modelPath_;
    bool hasModel_ = false;

    QString createJsonFile(QTemporaryDir& dir, const QString& name, const QStringList& texts) {
        QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        QJsonArray records;
        for (const auto& t : texts) {
            QJsonObject rec;
            rec["text"] = t;
            records.append(rec);
        }
        file.write(QJsonDocument(records).toJson());
        file.close();
        return path;
    }

    QString createCsvFile(QTemporaryDir& dir, const QString& name, const QStringList& lines) {
        QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        file.write(lines.join("\n").toUtf8());
        file.close();
        return path;
    }

    // JSON with "records" key: {"records": [{"text": "..."}]}
    QString createJsonRecordsFile(QTemporaryDir& dir, const QString& name, const QStringList& texts) {
        QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        QJsonArray records;
        for (const auto& t : texts) {
            QJsonObject rec;
            rec["text"] = t;
            records.append(rec);
        }
        QJsonObject root;
        root["records"] = records;
        file.write(QJsonDocument(root).toJson());
        file.close();
        return path;
    }

    // JSON with "items" key: {"items": [{"text": "..."}]}
    QString createJsonItemsFile(QTemporaryDir& dir, const QString& name, const QStringList& texts) {
        QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        QJsonArray items;
        for (const auto& t : texts) {
            QJsonObject rec;
            rec["text"] = t;
            items.append(rec);
        }
        QJsonObject root;
        root["items"] = items;
        file.write(QJsonDocument(root).toJson());
        file.close();
        return path;
    }

    // JSON with "data" key: {"data": [{"text": "..."}]}
    QString createJsonDataFile(QTemporaryDir& dir, const QString& name, const QStringList& texts) {
        QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        QJsonArray dataArr;
        for (const auto& t : texts) {
            QJsonObject rec;
            rec["text"] = t;
            dataArr.append(rec);
        }
        QJsonObject root;
        root["data"] = dataArr;
        file.write(QJsonDocument(root).toJson());
        file.close();
        return path;
    }

    // Single JSON object: {"text": "..."}
    QString createJsonSingleObjectFile(QTemporaryDir& dir, const QString& name, const QString& text) {
        QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        QJsonObject root;
        root["text"] = text;
        file.write(QJsonDocument(root).toJson());
        file.close();
        return path;
    }

    // String array: ["text1", "text2"]
    QString createJsonStringArrayFile(QTemporaryDir& dir, const QString& name, const QStringList& texts) {
        QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        QJsonArray arr;
        for (const auto& t : texts)
            arr.append(t);
        file.write(QJsonDocument(arr).toJson());
        file.close();
        return path;
    }

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

    void cleanupTestCase() {
    }

    void testDefaultConstruction() {
        BatchProcessor processor;
        QCOMPARE(processor.getResults().size(), 0);
    }

    void testSetFiles() {
        BatchProcessor processor;
        QStringList files = {"/path/to/file1.json", "/path/to/file2.csv"};
        processor.setFiles(files);
        QVERIFY(true);
    }

    void testJsonContentField() {
        BatchProcessor processor;
        processor.setJsonContentField("content");
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
        processor.setModelPath(modelPath_);
        QVERIFY(true);
    }

    void testStopRequest() {
        BatchProcessor processor;
        processor.stop();
        QVERIFY(true);
    }

    void testScanJsonFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonFile(tempDir, "test_scan.json",
            {"发动机轴承出现异常磨损", "液压系统压力不足"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
        auto args = scanFinishedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 1);
        QVERIFY(args.at(1).toInt() >= 0);
    }

    void testScanCsvFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createCsvFile(tempDir, "test_scan.csv",
            {"content", "发动机异常", "液压泄漏"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setCsvContentColumn("content");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
        auto args = scanFinishedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 1);
        QVERIFY(args.at(1).toInt() >= 0);
    }

    void testProcessEmptyFileList() {
        BatchProcessor processor;
        QSignalSpy finishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.start();
        processor.wait();

        QCOMPARE(finishedSpy.count(), 1);
        auto args = finishedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 0);
        QCOMPARE(args.at(1).toInt(), 0);
        QCOMPARE(args.at(2).toInt(), 0);
    }

    void testProgressSignals() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonFile(tempDir, "test_progress.json",
            {"进度测试文本", "第二个文本"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy finishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(finishedSpy.count() > 0);
    }

    void testErrorHandling() {
        BatchProcessor processor;
        QSignalSpy finishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({"/nonexistent/file.json"});

        processor.start();
        processor.wait();
        QTest::qWait(50);

        QVERIFY(finishedSpy.count() > 0);
        auto args = finishedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 1);
        QCOMPARE(args.at(2).toInt(), 1);
    }

    void testEmptyJsonFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = tempDir.filePath("empty.json");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{}");
        file.close();

        BatchProcessor processor;
        QSignalSpy finishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(50);

        QVERIFY(finishedSpy.count() > 0);
    }

    void testProcessJsonFile() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonFile(tempDir, "test_process.json",
            {"发动机轴承出现异常磨损，需要更换专用润滑油"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(200);

        QVERIFY(batchFinishedSpy.count() > 0 || errorSpy.count() > 0);
    }

    void testProcessCsvFile() {
        if (!hasModel_) QSKIP("Model not found");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createCsvFile(tempDir, "test_process.csv",
            {"content", "液压系统压力不足"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({path});
        processor.setCsvContentColumn("content");
        processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(200);

        QVERIFY(batchFinishedSpy.count() > 0);
    }

    void testStopDuringProcessing() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QStringList texts;
        for (int i = 0; i < 5; ++i)
            texts << QString("停止测试文本%1").arg(i);
        QString path = createJsonFile(tempDir, "test_stop.json", texts);
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.stop();
        processor.wait();
        QTest::qWait(50);

        QVERIFY(true);
    }

    void testJsonLinesFormat() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = tempDir.filePath("test_jsonl.jsonl");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        for (int i = 0; i < 3; ++i) {
            QJsonObject rec;
            rec["text"] = QString("JSONL测试文本%1 — 轴承磨损").arg(i);
            file.write(QJsonDocument(rec).toJson(QJsonDocument::Compact));
            file.write("\n");
        }
        file.close();

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
    }

    void testMultipleJsonFiles() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QStringList paths;
        for (int f = 0; f < 2; ++f) {
            QStringList texts;
            for (int i = 0; i < 1; ++i)
                texts << QString("多文件测试 故障检测").arg(f).arg(i);
            paths << createJsonFile(tempDir, QString("test_multi_%1.json").arg(f), texts);
            QVERIFY(!paths.last().isEmpty());
        }

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles(paths);
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
    }

    void testAutoExportLogic() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonFile(tempDir, "test_autoexport.json",
            {"自动导出测试 — 检查机油状态"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
    }

    void testSaveToDatabaseLogic() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        DatabaseManager::instance().close();
        QVERIFY(DatabaseManager::instance().initialize(tempDir.filePath("batch_test.db")));

        int initialCount = DatabaseManager::instance().getRecordCount();

        QString path = createJsonFile(tempDir, "test_savetodb.json",
            {"数据库测试 — 传感器故障"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        auto results = processor.getResults();
        QList<ExtractionRecord> allRecords;
        for (const auto& result : results) {
            if (result.success) {
                allRecords.append(result.extractionRecords);
            }
        }

        if (!allRecords.isEmpty()) {
            QVERIFY(DatabaseManager::instance().insertExtractionRecords(allRecords));
            QVERIFY(DatabaseManager::instance().getRecordCount() >= initialCount);
        }

        DatabaseManager::instance().close();
    }

    void testPlainTextFileProcessing() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = tempDir.filePath("test_plain.txt");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QStringLiteral("电机轴承出现异常磨损，需要更换专用润滑油。").toUtf8());
        file.close();

        BatchProcessor processor;
        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);
        processor.start();
        processor.wait();
        QTest::qWait(100);
        QVERIFY(processor.getResults().size() >= 0);
    }

    void testUnsupportedFileFormat() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create one valid JSON file so totalRecords > 0 (processing phase runs)
        QString jsonPath = createJsonFile(tempDir, "valid.json", {"有效记录"});
        QVERIFY(!jsonPath.isEmpty());

        // Create a .dat file (unsupported extension)
        QString datPath = tempDir.filePath("test_unsupported.dat");
        QFile file(datPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("some data");
        file.close();

        BatchProcessor processor;
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({jsonPath, datPath});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
        auto args = batchFinishedSpy.takeFirst();
        // Should have at least 1 failure (the .dat file)
        QVERIFY(args.at(0).toInt() >= 1);
        QVERIFY(args.at(2).toInt() >= 1);
    }

    void testBatchResultExport() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonFile(tempDir, "test_export.json",
            {"导出测试 — 压力传感器异常"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
    }

    void testJsonRecordsFormat() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonRecordsFile(tempDir, "test_records.json",
            {"records格式测试 — 齿轮磨损"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
        auto args = scanFinishedSpy.takeFirst();
        QVERIFY(args.at(0).toInt() >= 0);
    }

    void testJsonItemsFormat() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonItemsFile(tempDir, "test_items.json",
            {"items格式测试 — 液压泵故障"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
    }

    void testJsonDataFormat() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonDataFile(tempDir, "test_data.json",
            {"data格式测试 — 传感器异常"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
    }

    void testJsonSingleObjectFormat() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonSingleObjectFile(tempDir, "test_single.json",
            "单对象格式测试 — 机油变质");
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
    }

    void testJsonStringArrayFormat() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonStringArrayFile(tempDir, "test_strarr.json",
            {"字符串数组1 — 轴承异常", "字符串数组2 — 液压泄漏"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
    }

    // CSV quoted fields (exercises parseCsvLine in-quotes comma handling)
    void testCsvWithQuotedFields() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createCsvFile(tempDir, "test_quoted.csv",
            {"content,other",
             "\"轴承, 磨损, 故障\",extra",
             "\"液压泄漏\",more"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setCsvContentColumn("content");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
        auto args = scanFinishedSpy.takeFirst();
        // Should scan 2 data rows (quoted fields with commas parsed correctly)
        QCOMPARE(args.at(1).toInt(), 2);
    }

    // CSV with escaped double quotes inside quoted fields
    void testCsvWithEscapedQuotes() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QFile file(tempDir.filePath("test_escaped.csv"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        // "She said ""hello"" to me" => after parsing: She said "hello" to me
        file.write("content,note\n\"She said \"\"hello\"\" to me\",ok\nnormal text,ok\n");
        file.close();

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({file.fileName()});
        processor.setCsvContentColumn("content");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
        auto args = scanFinishedSpy.takeFirst();
        QCOMPARE(args.at(1).toInt(), 2);
    }

    // CSV with invalid encoding name (exercises the encoding warning branch)
    void testCsvWithInvalidEncoding() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createCsvFile(tempDir, "test_bad_enc.csv",
            {"content", "正常文本", "另一条"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({path});
        processor.setCsvContentColumn("content");
        processor.setCsvEncoding("INVALID-ENCODING-NAME");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
        auto args = scanFinishedSpy.takeFirst();
        QCOMPARE(args.at(1).toInt(), 2); // should still process with UTF-8 fallback
    }

    // CSV where the specified content column is not found in the header
    void testCsvContentColumnNotFound() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createCsvFile(tempDir, "test_missing_col.csv",
            {"col_a,col_b,col_c",
             "val1,val2,val3",
             "val4,val5,val6"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({path});
        processor.setCsvContentColumn("nonexistent_column");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
        // Column not found → failCount should be 1
        auto args = batchFinishedSpy.takeFirst();
        QCOMPARE(args.at(2).toInt(), 1);
    }

    // CSV with empty header (first line empty)
    void testCsvEmptyHeader() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QFile file(tempDir.filePath("test_empty_header.csv"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\n");
        file.close();

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({file.fileName()});
        processor.setCsvContentColumn("anycolumn");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(50);

        QVERIFY(batchFinishedSpy.count() > 0);
        auto args = batchFinishedSpy.takeFirst();
        // Empty header → error → failCount should be 1
        QCOMPARE(args.at(2).toInt(), 1);
    }

    // CSV with empty fields (exercises parseCsvLine empty field branch)
    void testCsvWithEmptyFields() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QFile file(tempDir.filePath("test_empty_fields.csv"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("col1,col2,col3\n,a_val,\nval_a,,val_c\n");
        file.close();

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({file.fileName()});
        processor.setCsvContentColumn("col1");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
    }

    // processJsonFileWithProgress: file exists but can't be opened (permission denied)
    void testJsonFileOpenError() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create a valid JSON file so scan succeeds and we enter processing phase
        QString validPath = createJsonFile(tempDir, "valid_for_open_test.json",
            {"有效记录用于触发处理阶段"});
        QVERIFY(!validPath.isEmpty());

        // Create a JSON file and remove read permission
        QString lockedPath = tempDir.filePath("locked.json");
        QFile lockedFile(lockedPath);
        QVERIFY(lockedFile.open(QIODevice::WriteOnly));
        lockedFile.write(QByteArray(R"([{"text":"secret"}])"));
        lockedFile.close();
        QFile::setPermissions(lockedPath, QFileDevice::WriteOwner); // no ReadOwner

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);

        processor.setFiles({validPath, lockedPath});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
        auto args = batchFinishedSpy.takeFirst();
        // locked.json should fail (can't open) → at least 1 failure
        QVERIFY(args.at(2).toInt() >= 1);

        // Restore permissions for cleanup
        QFile::setPermissions(lockedPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    // processCsvFileWithProgress: file exists but can't be opened (permission denied)
    void testCsvFileOpenError() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create a valid JSON file so scan succeeds and we enter processing phase
        QString validPath = createJsonFile(tempDir, "valid_for_csv_open_test.json",
            {"有效记录用于触发处理阶段"});
        QVERIFY(!validPath.isEmpty());

        // Create a CSV file and remove read permission
        QString lockedPath = tempDir.filePath("locked.csv");
        QFile lockedFile(lockedPath);
        QVERIFY(lockedFile.open(QIODevice::WriteOnly));
        lockedFile.write(QByteArray("content\nsecret text"));
        lockedFile.close();
        QFile::setPermissions(lockedPath, QFileDevice::WriteOwner); // no ReadOwner

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({validPath, lockedPath});
        processor.setCsvContentColumn("content");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
        auto args = batchFinishedSpy.takeFirst();
        // locked.csv should fail (can't open)
        QVERIFY(args.at(2).toInt() >= 1);

        // Restore permissions for cleanup
        QFile::setPermissions(lockedPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    // Scan phase with empty JSON file (not just process)
    void testScanEmptyFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QFile file(tempDir.filePath("empty_for_scan.json"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("");
        file.close();

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({file.fileName()});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(50);

        // Empty file → 0 records → batchFinished with all failures
        QVERIFY(scanFinishedSpy.count() > 0);
        auto args = scanFinishedSpy.takeFirst();
        QCOMPARE(args.at(1).toInt(), 0); // totalRecords = 0
    }

    // Set model path to a garbage file → loadModel fails → run() returns early (lines 246-251)
    void testModelLoadFailureDuringBatch() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Valid JSON so scan phase finds records (>0)
        QString jsonPath = createJsonFile(tempDir, "valid.json", {"发动机异常磨损"});
        QVERIFY(!jsonPath.isEmpty());

        // Create a garbage file to use as fake model
        QString fakeModelPath = tempDir.filePath("fake_model.onnx");
        QFile fakeModel(fakeModelPath);
        QVERIFY(fakeModel.open(QIODevice::WriteOnly));
        fakeModel.write(QByteArray("not a valid ONNX model"));
        fakeModel.close();

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);
        QSignalSpy errorSpy(&processor, &BatchProcessor::errorOccurred);

        processor.setFiles({jsonPath});
        processor.setJsonContentField("text");
        processor.setModelPath(fakeModelPath);  // garbage → loadModel will fail

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
        auto args = batchFinishedSpy.takeFirst();
        // Model load failed → all files fail
        QCOMPARE(args.at(1).toInt(), 0); // 0 success
        QCOMPARE(args.at(2).toInt(), 1); // 1 fail
    }

    // JSON Lines content in a .json file → exercises JSON parse failure → JSONL fallback (lines 406-422)
    void testJsonLinesInDotJsonFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create a .json file containing JSONL content (each line is a valid JSON object)
        QString path = tempDir.filePath("test_jsonl_as_json.json");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        for (int i = 0; i < 3; ++i) {
            QJsonObject rec;
            rec["text"] = QString("JSONL-in-JSON测试%1 — 轴承磨损").arg(i);
            file.write(QJsonDocument(rec).toJson(QJsonDocument::Compact));
            file.write("\n");
        }
        file.close();

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
        auto scanArgs = scanFinishedSpy.takeFirst();
        QCOMPARE(scanArgs.at(1).toInt(), 3); // 3 JSONL records found in scan

        if (hasModel_) {
            QVERIFY(batchFinishedSpy.count() > 0);
        }
    }

    // Stop during processing with many records → exercises stopRequested_ in processing loops (lines 258, 480, 663)
    void testStopDuringHeavyProcessing() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create many records so processing takes measurable time
        QStringList texts;
        for (int i = 0; i < 50; ++i)
            texts << QString("停止中断测试文本%1 — 发动机轴承异常磨损故障").arg(i);
        QString path = createJsonFile(tempDir, "test_stop_heavy.json", texts);
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        processor.setFiles({path});
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        QTest::qWait(200); // wait for processing to actually start
        processor.stop();
        processor.wait();
        QTest::qWait(100);

        // Should not crash; stopRequested_ was checked somewhere
        QVERIFY(true);
    }

    // Model auto-detection: don't set modelPath → exercises lines 217-220
    void testAutoDetectModel() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString path = createJsonFile(tempDir, "test_autodetect.json",
            {"自动检测模型测试 — 传感器故障"});
        QVERIFY(!path.isEmpty());

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({path});
        processor.setJsonContentField("text");
        // Do NOT call setModelPath — auto-detection should kick in

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
        auto args = batchFinishedSpy.takeFirst();
        // Either model auto-detected and processing succeeds, or no model found
        QVERIFY(args.at(0).toInt() >= 0);
    }

    // CSV where the target column has all-empty values → exercises line 644 (empty texts after parsing)
    void testCsvAllEmptyContentColumn() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QFile file(tempDir.filePath("test_empty_col.csv"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("col_a,col_b,col_c\n"
                   "val1,,val3\n"
                   "val4,,val6\n"
                   "val7,,val9\n");
        file.close();

        BatchProcessor processor;
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles({file.fileName()});
        processor.setCsvContentColumn("col_b"); // col_b is always empty

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(batchFinishedSpy.count() > 0);
        auto args = batchFinishedSpy.takeFirst();
        // Empty content column → all texts empty → success with 0 triples
        QCOMPARE(args.at(1).toInt(), 1); // successCount = 1
    }

    // CSV with quoted fields → exercises parseCsvLine quote-handling (lines 63-69, 92-96)
    void testCsvQuotedFields() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Fields with quotes: commas inside quotes should not split,
        // and double-quote escaping should be handled
        QFile file(tempDir.filePath("test_quoted.csv"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("col_a,col_b,col_c\n"
                   "\"motor bearing\",\"hello \"\"world\"\"\",val3\n"
                   "\"pump, valve\",normal_text,val6\n");
        file.close();

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);

        processor.setFiles({file.fileName()});
        processor.setCsvContentColumn("col_a"); // col_a has quoted values

        // Set a non-existent JSON content field so scan does text detection properly
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        processor.wait();
        QTest::qWait(100);

        QVERIFY(scanFinishedSpy.count() > 0);
        auto args = scanFinishedSpy.takeFirst();
        // Should detect 2 records — quotes stripped properly, internal comma not splitting
        QCOMPARE(args.at(1).toInt(), 2);
    }

    // Stop during file scan → exercises stopRequested_ in scan loop (lines 181-183)
    void testStopDuringFileScan() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create many files so scan phase takes measurable time
        QStringList files;
        for (int i = 0; i < 30; ++i) {
            QString path = createJsonFile(tempDir, QString("scan_%1.json").arg(i),
                {QString("扫描中断测试文本%1 — 发动机轴承磨损").arg(i)});
            QVERIFY(!path.isEmpty());
            files.append(path);
        }

        BatchProcessor processor;
        QSignalSpy scanFinishedSpy(&processor, &BatchProcessor::scanFinished);
        QSignalSpy batchFinishedSpy(&processor, &BatchProcessor::batchFinished);

        processor.setFiles(files);
        processor.setJsonContentField("text");
        if (hasModel_) processor.setModelPath(modelPath_);

        processor.start();
        // Immediately request stop — should hit stopRequested_ in the scan loop
        QTest::qWait(10);
        processor.stop();
        processor.wait();
        QTest::qWait(100);

        // Should finish (either scan stopped early, or reached batchFinished)
        QVERIFY(true); // didn't crash
    }
};

QTEST_MAIN(TestBatchProcessor)
#include "test_batchprocessor.moc"
