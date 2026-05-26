#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include "ui/settingsdialog.h"
#include "utils/configmanager.h"

using namespace optikg;

class TestSettingsDialog : public QObject {
    Q_OBJECT

private:
    QTemporaryDir* tempDir_ = nullptr;

private slots:
    void initTestCase() {
        tempDir_ = new QTemporaryDir();
        QVERIFY(tempDir_->isValid());
        qputenv("XDG_CONFIG_HOME", tempDir_->path().toUtf8());
        ConfigManager::instance().initialize();
    }

    void cleanupTestCase() {
        delete tempDir_;
    }

    void init() {
        ConfigManager::instance().initialize();
        ConfigManager::instance().setJsonContentField("text");
        ConfigManager::instance().setCsvContentColumn("content");
        ConfigManager::instance().setCsvEncoding("UTF-8");
        ConfigManager::instance().setModelPath("");
        ConfigManager::instance().setThreshold(-10.0f);
        ConfigManager::instance().setBatchOutputDir("");
        ConfigManager::instance().setAutoExport(false);
        ConfigManager::instance().setDefaultExportFormat("json");
        ConfigManager::instance().saveSettings();
    }

    void testDefaultConstruction() {
        SettingsDialog dialog;
        QCOMPARE(dialog.windowTitle(), QString("设置"));
        QVERIFY(dialog.minimumWidth() >= 500);
        QVERIFY(dialog.minimumHeight() >= 400);
    }

    void testUiComponentsExist() {
        SettingsDialog dialog;

        auto* tabWidget = dialog.findChild<QTabWidget*>();
        QVERIFY(tabWidget != nullptr);
        QCOMPARE(tabWidget->count(), 3);

        auto* jsonFieldEdit = dialog.findChild<QLineEdit*>();
        QVERIFY(jsonFieldEdit != nullptr);

        auto* thresholdSpin = dialog.findChild<QDoubleSpinBox*>();
        QVERIFY(thresholdSpin != nullptr);
        QCOMPARE(thresholdSpin->minimum(), -100.0);
        QCOMPARE(thresholdSpin->maximum(), 0.0);

        auto* autoExportCheck = dialog.findChild<QCheckBox*>();
        QVERIFY(autoExportCheck != nullptr);

        auto buttons = dialog.findChildren<QPushButton*>();
        QVERIFY(buttons.size() >= 4); // 应用、确定、取消、恢复默认
    }

    void testLoadSettings() {
        // 预设 ConfigManager 的值
        ConfigManager::instance().setJsonContentField("body");
        ConfigManager::instance().setCsvContentColumn("description");
        ConfigManager::instance().setCsvEncoding("GBK");
        ConfigManager::instance().setThreshold(-5.5f);
        ConfigManager::instance().setAutoExport(true);
        ConfigManager::instance().setDefaultExportFormat("csv");

        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        auto lineEdits = dialog.findChildren<QLineEdit*>();
        bool foundJsonField = false;
        for (auto* edit : lineEdits) {
            if (edit->text() == "body") {
                foundJsonField = true;
                break;
            }
        }
        QVERIFY(foundJsonField);

        auto* thresholdSpin = dialog.findChild<QDoubleSpinBox*>();
        QVERIFY(thresholdSpin != nullptr);
        QCOMPARE(thresholdSpin->value(), -5.5);

        auto* autoExportCheck = dialog.findChild<QCheckBox*>();
        QVERIFY(autoExportCheck != nullptr);
        QCOMPARE(autoExportCheck->isChecked(), true);
    }

    void testOkButton() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // 通过 QMetaObject 调用 onOkClicked，应 accept 对话框
        QMetaObject::invokeMethod(&dialog, "onOkClicked");
        QCOMPARE(dialog.result(), static_cast<int>(QDialog::Accepted));
    }

    void testCancelButtonWithoutChanges() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        QMetaObject::invokeMethod(&dialog, "onCancelClicked");
        QCOMPARE(dialog.result(), static_cast<int>(QDialog::Rejected));
    }

    void testApplyButton() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // 修改阈值
        auto* thresholdSpin = dialog.findChild<QDoubleSpinBox*>();
        QVERIFY(thresholdSpin != nullptr);
        thresholdSpin->setValue(-8.0);

        QMetaObject::invokeMethod(&dialog, "onApplyClicked");
        QTest::qWait(50);

        // 应用后 ConfigManager 应被更新
        QCOMPARE(ConfigManager::instance().getThreshold(), -8.0f);
    }

    void testRestoreDefaults() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // 先修改值
        auto* thresholdSpin = dialog.findChild<QDoubleSpinBox*>();
        QVERIFY(thresholdSpin != nullptr);
        thresholdSpin->setValue(-50.0);

        // 模拟点击恢复默认（需要处理 QMessageBox 确认对话框）
        QTimer::singleShot(100, [this]() {
            // Iterate all top-level widgets to find the QMessageBox
            // (activeModalWidget() may return nullptr on Wayland)
            for (auto* w : qApp->topLevelWidgets()) {
                if (auto* dialog = qobject_cast<QMessageBox*>(w)) {
                    auto* yesBtn = dialog->button(QMessageBox::Yes);
                    if (yesBtn && yesBtn->isVisible()) {
                        QTest::mouseClick(yesBtn, Qt::LeftButton);
                        return;
                    }
                }
            }
        });

        QMetaObject::invokeMethod(&dialog, "onRestoreDefaultsClicked");
        QTest::qWait(200);

        // 验证阈值已恢复默认值
        QCOMPARE(static_cast<float>(thresholdSpin->value()), AppConstants::Model::DEFAULT_THRESHOLD);
    }
};

QTEST_MAIN(TestSettingsDialog)
#include "test_settingsdialog.moc"
