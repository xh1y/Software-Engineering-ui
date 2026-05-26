#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QElapsedTimer>
#include <memory>
#include "optikg/datamodel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QTextEdit;
class QPushButton;
class QStatusBar;
class QProgressBar;
class QLabel;
QT_END_NAMESPACE



namespace optikg {

class ExtractionPanel;
class GraphWidget;
class HistoryPanel;
class ResultTreeWidget;
class InferenceWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onExtractClicked();
    void onClearClicked();
    void onSaveClicked();
    void onSearchClicked();
    void onBatchProcessClicked();
    void onExportJsonClicked();
    void onExportCsvClicked();
    void onSettingsClicked();
    void onOpenClicked();
    void onOpenTextFileClicked();
    void onAboutClicked();
    void onHelpClicked();

    void onExtractionStarted();
    void onExtractionFinished(const QList<optikg::Triple>& results);
    void onExtractionError(const QString& error);
    void onProgressChanged(int value);
    void onBatchDeleteRecords(const QList<qint64>& ids);

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupShortcuts();
    void connectSignals();

    void loadSettings();
    void saveSettings();
    void showToast(const QString& message, bool isError = false);
    void loadRecordDetails(qint64 recordId);

    // UI 组件
    Ui::MainWindow *ui;
    ExtractionPanel* extractionPanel_;
    GraphWidget* graphWidget_;
    HistoryPanel* historyPanel_;
    ResultTreeWidget* resultTree_;

    // 状态栏组件
    QStatusBar* statusBar_;
    QLabel* statusLabel_;
    QLabel* modeLabel_;
    QLabel* thresholdLabel_;
    QLabel* countLabel_;
    QProgressBar* progressBar_;

    // 计时器
    QElapsedTimer extractionTimer_;

    // 当前抽取结果
    QList<optikg::Triple> currentResults_;

    // 工作线程
    InferenceWorker* currentWorker_;
};

} // namespace optikg