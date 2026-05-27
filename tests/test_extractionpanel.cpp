#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include "ui/extractionpanel.h"

using namespace optikg;

class TestExtractionPanel : public QObject {
    Q_OBJECT

private:
    ExtractionPanel *panel = nullptr;

private slots:
    void init() {
        panel = new ExtractionPanel();
    }

    void cleanup() {
        delete panel;
        panel = nullptr;
    }

    void testDefaultConstruction() {
        QVERIFY(panel != nullptr);
        QVERIFY(panel->text().isEmpty());
    }

    void testSetAndGetText() {
        panel->setText("发动机异响故障");
        QCOMPARE(panel->text(), QString("发动机异响故障"));
    }

    void testSetEmptyText() {
        panel->setText("");
        QCOMPARE(panel->text(), QString(""));
    }

    void testTextTrimmed() {
        panel->setText("  前后有空格  ");
        QCOMPARE(panel->text(), QString("前后有空格"));
    }

    void testClear() {
        panel->setText("测试文本");
        QCOMPARE(panel->text(), QString("测试文本"));
        panel->clear();
        QCOMPARE(panel->text(), QString(""));
    }

    void testSetEnabled() {
        panel->setEnabled(false);
        // 验证控件被禁用 —— 通过 setText/getText 间接验证
        // QTextEdit 禁用后 setPlainText 仍可用，但用户无法交互
        // 这里验证不崩溃
        panel->setText("enabled test");
        QCOMPARE(panel->text(), QString("enabled test"));

        panel->setEnabled(true);
        QCOMPARE(panel->text(), QString("enabled test"));
    }

    void testExtractClickedSignal() {
        QSignalSpy spy(panel, &ExtractionPanel::extractClicked);
        QVERIFY(spy.isValid());

        // 通过 findChild 找到 extractBtn 并点击
        QPushButton *btn = panel->findChild<QPushButton *>("extractBtn");
        QVERIFY(btn != nullptr);
        btn->click();
        QCOMPARE(spy.count(), 1);
    }

    void testClearClickedSignal() {
        QSignalSpy spy(panel, &ExtractionPanel::clearClicked);
        QVERIFY(spy.isValid());

        QPushButton *btn = panel->findChild<QPushButton *>("clearBtn");
        QVERIFY(btn != nullptr);
        btn->click();
        QCOMPARE(spy.count(), 1);
    }

    void testTextChangedSignal() {
        QSignalSpy spy(panel, &ExtractionPanel::textChanged);
        QVERIFY(spy.isValid());

        panel->setText("新文本");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("新文本"));
    }

    void testTextChangedSignalForEmpty() {
        QSignalSpy spy(panel, &ExtractionPanel::textChanged);

        panel->setText("先有内容");
        QCOMPARE(spy.count(), 1);
        panel->clear();
        QCOMPARE(spy.count(), 2);
    }

    void testCharCountNormal() {
        panel->setText("短文本");
        // 字符数小于 MAX_CHARS * 0.9 = 4500，state 应为 "normal"
        QLabel *label = panel->findChild<QLabel *>("charCountLabel");
        QVERIFY(label != nullptr);
        QVERIFY(label->text().contains("5000"));
    }

    void testCharCountWarning() {
        // 填充到超过 90% 但不满 5000
        QString longText(4600, 'A');
        panel->setText(longText);
        QLabel *label = panel->findChild<QLabel *>("charCountLabel");
        QVERIFY(label != nullptr);
        QCOMPARE(label->property("state").toString(), QString("warning"));
    }

    void testCharCountError() {
        // 填充到满 5000
        QString maxText(5100, 'B');  // 超过 5000，会被截断
        panel->setText(maxText);
        QCOMPARE(panel->text().length(), 5000);

        QLabel *label = panel->findChild<QLabel *>("charCountLabel");
        QVERIFY(label != nullptr);
        QCOMPARE(label->property("state").toString(), QString("error"));
    }

    void testTruncation() {
        // 超过 MAX_CHARS (5000) 的文本应被截断
        QString veryLong(6000, 'X');
        panel->setText(veryLong);
        QCOMPARE(panel->text().length(), 5000);
    }

    void testTruncationExactlyAtLimit() {
        QString exactLimit(5000, 'Y');
        panel->setText(exactLimit);
        QCOMPARE(panel->text().length(), 5000);
    }

    void testMultipleTextChanges() {
        // 多次变更文本
        panel->setText("first");
        panel->setText("second");
        panel->setText("third");
        QCOMPARE(panel->text(), QString("third"));
    }
};

QTEST_MAIN(TestExtractionPanel)
#include "test_extractionpanel.moc"
