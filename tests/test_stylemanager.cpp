#include <QTest>
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include "utils/stylemanager.h"

using namespace optikg;

class TestStyleManager : public QObject {
    Q_OBJECT

private slots:
    void testSingleton() {
        StyleManager& instance1 = StyleManager::instance();
        StyleManager& instance2 = StyleManager::instance();
        QCOMPARE(&instance1, &instance2);
    }

    void testLoadNonExistentFile() {
        StyleManager& sm = StyleManager::instance();
        QVERIFY(!sm.loadStyleSheet("/nonexistent/path/style.qss"));
        QVERIFY(!sm.lastError().isEmpty());
        QVERIFY(sm.lastError().contains("does not exist"));
    }

    void testLoadNonExistentResource() {
        StyleManager& sm = StyleManager::instance();
        QVERIFY(!sm.loadStyleSheetFromResource(":/styles/nonexistent.qss"));
        QVERIFY(!sm.lastError().isEmpty());
        QVERIFY(sm.lastError().contains("does not exist"));
    }

    void testLoadValidStyleSheet() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString filePath = tempDir.filePath("test.qss");
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QString qss = "QMainWindow { background: #333; }";
        file.write(qss.toUtf8());
        file.close();

        StyleManager& sm = StyleManager::instance();
        QVERIFY2(sm.loadStyleSheet(filePath),
                  qPrintable(QString("loadStyleSheet failed: %1, path: %2, exists: %3")
                             .arg(sm.lastError(), filePath)
                             .arg(QFile::exists(filePath) ? "yes" : "no")));
        QCOMPARE(sm.currentStyleSheet(), qss);
        QVERIFY(sm.lastError().isEmpty());
    }

    void testApplyStyleSheet() {
        StyleManager& sm = StyleManager::instance();
        QString qss = "QLabel { color: red; }";
        sm.applyStyleSheet(qss);
        QCOMPARE(sm.currentStyleSheet(), qss);
        QCOMPARE(qApp->styleSheet(), qss);
    }

    void testReloadCurrentTheme() {
        StyleManager& sm = StyleManager::instance();
        QString qss = "QPushButton { background: blue; }";
        sm.applyStyleSheet(qss);
        QVERIFY(sm.reloadCurrentTheme());
        QCOMPARE(qApp->styleSheet(), qss);
    }

    void testLastErrorClearedOnSuccess() {
        StyleManager& sm = StyleManager::instance();
        // 先产生一个错误
        sm.loadStyleSheet("/fake/file.qss");
        QVERIFY(!sm.lastError().isEmpty());

        // 成功后应清除错误
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString filePath = tempDir.filePath("test2.qss");
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("QWidget {}");
        file.close();

        QVERIFY2(sm.loadStyleSheet(filePath),
                  qPrintable(QString("loadStyleSheet failed: %1").arg(sm.lastError())));
        QVERIFY(sm.lastError().isEmpty());
    }
};

QTEST_MAIN(TestStyleManager)
#include "test_stylemanager.moc"
