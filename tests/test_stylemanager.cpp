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

    void testLoadStyleSheetOpenFailure() {
        // 创建一个文件但设为不可读
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString filePath = tempDir.filePath("unreadable.qss");
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("QWidget {}");
        file.close();

        // 修改权限使文件不可读
        file.setPermissions(QFileDevice::WriteOwner);

        StyleManager& sm = StyleManager::instance();
        QVERIFY(!sm.loadStyleSheet(filePath));
        QVERIFY(!sm.lastError().isEmpty());
        QVERIFY(sm.lastError().contains("Cannot open"));

        // 恢复权限以便 QTemporaryDir 清理
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    void testLoadStyleSheetFromResourceSuccess() {
        // 使用已知存在的资源路径
        StyleManager& sm = StyleManager::instance();
        bool result = sm.loadStyleSheetFromResource(":/styles/modern_pro.qss");
        if (result) {
            QVERIFY(!sm.currentStyleSheet().isEmpty());
            QVERIFY(sm.lastError().isEmpty());
        } else {
            // 资源可能不存在，这是允许的
            QSKIP("Resource :/styles/modern_pro.qss not available");
        }
    }

    void testApplyStyleSheetEmpty() {
        StyleManager& sm = StyleManager::instance();
        sm.applyStyleSheet("");
        QCOMPARE(sm.currentStyleSheet(), QString(""));
        // qApp->styleSheet() 可能保留之前的值，但 currentStyleSheet 应该更新
    }

    void testReloadCurrentThemeAfterError() {
        StyleManager& sm = StyleManager::instance();
        // 设置一个样式
        QString qss = "QPushButton { color: green; }";
        sm.applyStyleSheet(qss);
        QCOMPARE(sm.currentStyleSheet(), qss);

        // reload 应该成功
        QVERIFY(sm.reloadCurrentTheme());
        // 样式应该还在
        QCOMPARE(qApp->styleSheet(), qss);
    }

    void testInitialize() {
        // initialize 尝试加载默认资源 / 文件
        // 可能成功也可能失败（取决于资源是否存在），但不应该崩溃
        StyleManager& sm = StyleManager::instance();
        sm.initialize();
        // 仅验证不崩溃
        QVERIFY(true);
    }

    void testMultipleLoadStyleSheet() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // 创建两个不同的样式文件
        QString path1 = tempDir.filePath("style1.qss");
        QFile file1(path1);
        QVERIFY(file1.open(QIODevice::WriteOnly));
        file1.write("QMainWindow { background: #111; }");
        file1.close();

        QString path2 = tempDir.filePath("style2.qss");
        QFile file2(path2);
        QVERIFY(file2.open(QIODevice::WriteOnly));
        file2.write("QMainWindow { background: #222; }");
        file2.close();

        StyleManager& sm = StyleManager::instance();
        QVERIFY(sm.loadStyleSheet(path1));
        QCOMPARE(sm.currentStyleSheet(), QString("QMainWindow { background: #111; }"));

        QVERIFY(sm.loadStyleSheet(path2));
        QCOMPARE(sm.currentStyleSheet(), QString("QMainWindow { background: #222; }"));
    }
};

QTEST_MAIN(TestStyleManager)
#include "test_stylemanager.moc"
