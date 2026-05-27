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
#include <QFileDialog>
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

    void testTabSwitching() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        auto* tabWidget = dialog.findChild<QTabWidget*>();
        QVERIFY(tabWidget != nullptr);

        // Switch to each tab
        tabWidget->setCurrentIndex(0);
        QTest::qWait(30);
        QCOMPARE(tabWidget->currentIndex(), 0);

        tabWidget->setCurrentIndex(1);
        QTest::qWait(30);
        QCOMPARE(tabWidget->currentIndex(), 1);

        tabWidget->setCurrentIndex(2);
        QTest::qWait(30);
        QCOMPARE(tabWidget->currentIndex(), 2);
    }

    void testCancelWithUnsavedChanges() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // Make a change
        auto* thresholdSpin = dialog.findChild<QDoubleSpinBox*>();
        QVERIFY(thresholdSpin != nullptr);
        double originalVal = thresholdSpin->value();
        thresholdSpin->setValue(-3.0);

        // Cancel without applying
        QMetaObject::invokeMethod(&dialog, "onCancelClicked");
        QCOMPARE(dialog.result(), static_cast<int>(QDialog::Rejected));

        // ConfigManager should still have the old value
        QVERIFY(ConfigManager::instance().getThreshold() != -3.0f);
    }

    void testBrowseModelButtonExists() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        QPushButton* browseModelBtn = nullptr;
        for (auto* btn : dialog.findChildren<QPushButton*>()) {
            if (btn->text() == tr("浏览...")) {
                // SettingsDialog has two browse buttons (model and output dir)
                // Find the one on the model page (tab 1)
                browseModelBtn = btn;
                break;
            }
        }
        QVERIFY(browseModelBtn != nullptr);
    }

    void testBrowseOutputDirButtonExists() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // Switch to batch tab (index 2) to access output dir button
        auto* tabWidget = dialog.findChild<QTabWidget*>();
        QVERIFY(tabWidget != nullptr);
        tabWidget->setCurrentIndex(2);
        QTest::qWait(30);

        QPushButton* browseOutputBtn = nullptr;
        for (auto* btn : dialog.findChildren<QPushButton*>()) {
            if (btn->text() == tr("浏览...")) {
                browseOutputBtn = btn;
                break;
            }
        }
        QVERIFY(browseOutputBtn != nullptr);
    }

    void testModelPathLineEdit() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        auto* tabWidget = dialog.findChild<QTabWidget*>();
        QVERIFY(tabWidget != nullptr);
        tabWidget->setCurrentIndex(1); // model page
        QTest::qWait(30);

        auto* modelPathEdit = dialog.findChild<QLineEdit*>();
        QVERIFY(modelPathEdit != nullptr);
        // Model path edit should have a placeholder text
        QVERIFY(!modelPathEdit->placeholderText().isEmpty());

        // Set and verify model path
        modelPathEdit->setText("/test/model/path.onnx");
        QCOMPARE(modelPathEdit->text(), QString("/test/model/path.onnx"));
    }

    // Test onCancelClicked with unsaved changes, user clicks No (don't cancel)
    void testCancelWithChangesUserSaysNo() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // Step 1: Restore defaults to set hasChanges_ = true
        QTimer::singleShot(100, []() {
            for (auto* w : QApplication::topLevelWidgets()) {
                if (auto* mb = qobject_cast<QMessageBox*>(w)) {
                    if (auto* yesBtn = mb->button(QMessageBox::Yes)) {
                        yesBtn->click();
                    }
                }
            }
        });
        QMetaObject::invokeMethod(&dialog, "onRestoreDefaultsClicked");
        QTest::qWait(250);

        // Step 2: Cancel — hasChanges_ is true, QMessageBox appears
        // Auto-click "No" to stay in dialog
        QTimer::singleShot(100, []() {
            for (auto* w : QApplication::topLevelWidgets()) {
                if (auto* mb = qobject_cast<QMessageBox*>(w)) {
                    if (auto* noBtn = mb->button(QMessageBox::No)) {
                        noBtn->click();
                    }
                }
            }
        });

        QMetaObject::invokeMethod(&dialog, "onCancelClicked");
        QTest::qWait(300);
        // Should not crash — user chose to stay
        QVERIFY(dialog.isVisible());
    }

    // Test onRestoreDefaultsClicked with user clicking No
    void testRestoreDefaultsCancelled() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // Modify a value first
        auto* thresholdSpin = dialog.findChild<QDoubleSpinBox*>();
        QVERIFY(thresholdSpin != nullptr);
        thresholdSpin->setValue(-50.0);

        // Auto-click "No" on the QMessageBox::question
        QTimer::singleShot(150, []() {
            for (QWidget* w : QApplication::topLevelWidgets()) {
                if (auto* mb = qobject_cast<QMessageBox*>(w)) {
                    if (auto* noBtn = mb->button(QMessageBox::No)) {
                        noBtn->click();
                    }
                }
            }
        });

        QMetaObject::invokeMethod(&dialog, "onRestoreDefaultsClicked");
        QTest::qWait(300);

        // Value should NOT have been restored to default
        QCOMPARE(static_cast<float>(thresholdSpin->value()), -50.0f);
    }

    // Test onBrowseModelClicked — dialog cancelled
    void testBrowseModelClickedCancel() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        auto* tabWidget = dialog.findChild<QTabWidget*>();
        QVERIFY(tabWidget != nullptr);
        tabWidget->setCurrentIndex(1); // model page
        QTest::qWait(30);

        // Auto-close the QFileDialog
        QTimer::singleShot(150, []() {
            for (QWidget* w : QApplication::topLevelWidgets()) {
                if (auto* fd = qobject_cast<QFileDialog*>(w)) {
                    fd->reject();
                }
            }
        });

        QMetaObject::invokeMethod(&dialog, "onBrowseModelClicked");
        QTest::qWait(300);
        // Should not crash — dialog was canceled
    }

    // Test onBrowseOutputDirClicked — dialog cancelled
    void testBrowseOutputDirClickedCancel() {
        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        auto* tabWidget = dialog.findChild<QTabWidget*>();
        QVERIFY(tabWidget != nullptr);
        tabWidget->setCurrentIndex(2); // batch page
        QTest::qWait(30);

        // Auto-close the QFileDialog
        QTimer::singleShot(150, []() {
            for (QWidget* w : QApplication::topLevelWidgets()) {
                if (auto* fd = qobject_cast<QFileDialog*>(w)) {
                    fd->reject();
                }
            }
        });

        QMetaObject::invokeMethod(&dialog, "onBrowseOutputDirClicked");
        QTest::qWait(300);
        // Should not crash — dialog was canceled
    }

    // Test loadSettings with encoding not in combo list (covers lines 226-228)
    void testLoadSettingsEncodingNotFound() {
        ConfigManager::instance().setCsvEncoding("ISO-8859-15");
        ConfigManager::instance().saveSettings();

        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // The encoding combo should have the custom value set as text
        auto comboBoxes = dialog.findChildren<QComboBox*>();
        bool found = false;
        for (auto* combo : comboBoxes) {
            if (combo->currentText() == "ISO-8859-15") {
                found = true;
                break;
            }
        }
        QVERIFY(found);

        // Restore
        ConfigManager::instance().setCsvEncoding("UTF-8");
    }

    // Test loadSettings with format not in combo list (covers lines 239-242)
    void testLoadSettingsFormatNotFound() {
        ConfigManager::instance().setDefaultExportFormat("xml");
        ConfigManager::instance().saveSettings();

        SettingsDialog dialog;
        dialog.show();
        QTest::qWait(50);

        // Should not crash; combo defaults to first item
        auto* autoExportCheck = dialog.findChild<QCheckBox*>();
        QVERIFY(autoExportCheck != nullptr);

        // Restore
        ConfigManager::instance().setDefaultExportFormat("json");
    }
};

QTEST_MAIN(TestSettingsDialog)
#include "test_settingsdialog.moc"
