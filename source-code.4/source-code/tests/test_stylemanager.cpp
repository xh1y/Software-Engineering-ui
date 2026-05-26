#include <QTest>
#include <QApplication>
#include <QTemporaryFile>
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
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        QString qss = "QMainWindow { background: #333; }";
        tempFile.write(qss.toUtf8());
        tempFile.close();

        StyleManager& sm = StyleManager::instance();
        QVERIFY(sm.loadStyleSheet(tempFile.fileName()));
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
        // reloadCurrentTheme 会重新设置相同的 stylesheet
        QCOMPARE(qApp->styleSheet(), qss);
    }

    void testLastErrorClearedOnSuccess() {
        StyleManager& sm = StyleManager::instance();
        // 先产生一个错误
        sm.loadStyleSheet("/fake/file.qss");
        QVERIFY(!sm.lastError().isEmpty());

        // 成功后应清除错误
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write("QWidget {}");
        tempFile.close();

        QVERIFY(sm.loadStyleSheet(tempFile.fileName()));
        QVERIFY(sm.lastError().isEmpty());
    }
};

QTEST_MAIN(TestStyleManager)
#include "test_stylemanager.moc"
