#pragma once

#include <QWidget>
#include <QDateTime>
#include "optikg/datamodel.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
class QLineEdit;
class QPushButton;
class QComboBox;
class QDateEdit;
QT_END_NAMESPACE

namespace optikg {

class HistoryPanel : public QWidget {
    Q_OBJECT

public:
    explicit HistoryPanel(QWidget *parent = nullptr);

    void loadHistory();

    QList<qint64> selectedRecordIds() const;

signals:
    void recordClicked(qint64 id);
    void recordDoubleClicked(qint64 id);
    void exportClicked(const QList<qint64>& ids);
    void batchDeleteClicked(const QList<qint64>& ids);

private slots:
    void onSearch();
    void onItemClicked(int row, int column);
    void onItemDoubleClicked(int row, int column);
    void onExport();
    void onBatchDelete();
    void onClearFilter();

private:
    void setupUi();
    void setupTable();
    void refreshTable(const QList<ExtractionRecord>& records);

    // 筛选组件
    QLineEdit* searchEdit_;
    QComboBox* entityTypeFilter_;
    QDateEdit* startDateFilter_;
    QDateEdit* endDateFilter_;
    QPushButton* searchBtn_;
    QPushButton* clearFilterBtn_;

    // 表格
    QTableWidget* tableWidget_;

    // 按钮
    QPushButton* exportBtn_;
    QPushButton* batchDeleteBtn_;
    QPushButton* refreshBtn_;
};

} // namespace optikg