#include "historypanel.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include "../data/databasemanager.h"

namespace optikg {

HistoryPanel::HistoryPanel(QWidget *parent)
    : QWidget(parent)
    , searchEdit_(nullptr)
    , entityTypeFilter_(nullptr)
    , startDateFilter_(nullptr)
    , endDateFilter_(nullptr)
    , searchBtn_(nullptr)
    , clearFilterBtn_(nullptr)
    , tableWidget_(nullptr)
    , exportBtn_(nullptr)
    , batchDeleteBtn_(nullptr)
    , refreshBtn_(nullptr) {
    setupUi();
}

void HistoryPanel::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 标题
    QLabel* titleLabel = new QLabel(tr("历史记录"));
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    mainLayout->addWidget(titleLabel);

    // 筛选区域
    QGroupBox* filterGroup = new QGroupBox(tr("筛选条件"));
    QGridLayout* filterLayout = new QGridLayout(filterGroup);

    filterLayout->addWidget(new QLabel(tr("关键词:")), 0, 0);
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText(tr("输入关键词搜索..."));
    filterLayout->addWidget(searchEdit_, 0, 1);

    filterLayout->addWidget(new QLabel(tr("实体类型:")), 0, 2);
    entityTypeFilter_ = new QComboBox();
    entityTypeFilter_->addItem(tr("全部"), -1);
    entityTypeFilter_->addItem(tr("部件"), 0);
    entityTypeFilter_->addItem(tr("故障"), 1);
    entityTypeFilter_->addItem(tr("工具"), 2);
    entityTypeFilter_->addItem(tr("组成"), 3);
    filterLayout->addWidget(entityTypeFilter_, 0, 3);

    filterLayout->addWidget(new QLabel(tr("开始日期:")), 1, 0);
    startDateFilter_ = new QDateEdit();
    startDateFilter_->setCalendarPopup(true);
    startDateFilter_->setDate(QDate::currentDate().addDays(-30));
    filterLayout->addWidget(startDateFilter_, 1, 1);

    filterLayout->addWidget(new QLabel(tr("结束日期:")), 1, 2);
    endDateFilter_ = new QDateEdit();
    endDateFilter_->setCalendarPopup(true);
    endDateFilter_->setDate(QDate::currentDate());
    filterLayout->addWidget(endDateFilter_, 1, 3);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    searchBtn_ = new QPushButton(tr("搜索"));
    searchBtn_->setIcon(QIcon::fromTheme("edit-find"));
    connect(searchBtn_, &QPushButton::clicked, this, &HistoryPanel::onSearch);

    clearFilterBtn_ = new QPushButton(tr("清空筛选"));
    clearFilterBtn_->setIcon(QIcon::fromTheme("edit-clear"));
    connect(clearFilterBtn_, &QPushButton::clicked, this, &HistoryPanel::onClearFilter);

    buttonLayout->addStretch();
    buttonLayout->addWidget(searchBtn_);
    buttonLayout->addWidget(clearFilterBtn_);
    filterLayout->addLayout(buttonLayout, 2, 0, 1, 4);

    mainLayout->addWidget(filterGroup);

    // 表格
    setupTable();
    mainLayout->addWidget(tableWidget_);

    // 按钮区域
    QHBoxLayout* actionLayout = new QHBoxLayout();
    exportBtn_ = new QPushButton(tr("导出选中"));
    exportBtn_->setIcon(QIcon::fromTheme("document-export"));
    connect(exportBtn_, &QPushButton::clicked, this, &HistoryPanel::onExport);

    batchDeleteBtn_ = new QPushButton(tr("批量删除"));
    batchDeleteBtn_->setIcon(QIcon::fromTheme("edit-delete"));
    batchDeleteBtn_->setStyleSheet("background-color: #f44336; color: white;");
    connect(batchDeleteBtn_, &QPushButton::clicked, this, &HistoryPanel::onBatchDelete);

    refreshBtn_ = new QPushButton(tr("刷新"));
    refreshBtn_->setIcon(QIcon::fromTheme("view-refresh"));
    connect(refreshBtn_, &QPushButton::clicked, this, [this]() { loadHistory(); });

    actionLayout->addWidget(exportBtn_);
    actionLayout->addWidget(batchDeleteBtn_);
    actionLayout->addStretch();
    actionLayout->addWidget(refreshBtn_);

    mainLayout->addLayout(actionLayout);
}

void HistoryPanel::setupTable() {
    tableWidget_ = new QTableWidget();
    tableWidget_->setColumnCount(6);
    tableWidget_->setHorizontalHeaderLabels({
        tr("ID"), tr("时间"), tr("处理时间(ms)"), tr("平均置信度"), tr("实体数"), tr("关系数")
    });

    tableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget_->setAlternatingRowColors(false); // 关闭斑马纹，配合 QSS 更好看
    tableWidget_->setShowGrid(false);            // 隐藏 Excel 式网格线
    tableWidget_->setFrameShape(QFrame::NoFrame);// 去除最外层边框
    tableWidget_->verticalHeader()->setVisible(false);
    tableWidget_->verticalHeader()->setDefaultSectionSize(40); // 增加行高，显得大气

    QHeaderView* header = tableWidget_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);

    connect(tableWidget_, &QTableWidget::cellClicked,
            this, &HistoryPanel::onItemClicked);
    connect(tableWidget_, &QTableWidget::cellDoubleClicked,
            this, &HistoryPanel::onItemDoubleClicked);
}

void HistoryPanel::loadHistory() {
    QList<ExtractionRecord> records = DatabaseManager::instance().getAllExtractionRecords();
    refreshTable(records);
}

void HistoryPanel::addRecord(const ExtractionRecord& record) {
    // 刷新整个表格以确保数据一致
    loadHistory();
}

void HistoryPanel::removeRecord(qint64 id) {
    // 从数据库中删除记录，然后刷新表格
    if (DatabaseManager::instance().deleteExtractionRecord(id)) {
        loadHistory();
    }
}

void HistoryPanel::clearHistory() {
    tableWidget_->setRowCount(0);
}

QList<qint64> HistoryPanel::selectedRecordIds() const {
    QList<qint64> ids;
    QList<QTableWidgetSelectionRange> ranges = tableWidget_->selectedRanges();
    for (const auto& range : ranges) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            QTableWidgetItem* item = tableWidget_->item(row, 0);
            if (item) {
                ids.append(item->text().toLongLong());
            }
        }
    }
    return ids;
}

void HistoryPanel::refreshTable(const QList<ExtractionRecord>& records) {
    tableWidget_->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const auto& record = records[i];
        tableWidget_->setItem(i, 0, new QTableWidgetItem(QString::number(record.id)));
        tableWidget_->setItem(i, 1, new QTableWidgetItem(record.createdAt.toString("yyyy-MM-dd hh:mm:ss")));
        tableWidget_->setItem(i, 2, new QTableWidgetItem(QString::number(record.processTimeMs)));
        tableWidget_->setItem(i, 3, new QTableWidgetItem(QString::number(record.avgConfidence, 'f', 2)));
        tableWidget_->setItem(i, 4, new QTableWidgetItem(QString::number(record.entityCount)));
        tableWidget_->setItem(i, 5, new QTableWidgetItem(QString::number(record.relationCount)));
    }
}

void HistoryPanel::onSearch() {
    QString keyword = searchEdit_->text();
    QDate startDate = startDateFilter_->date();
    QDate endDate = endDateFilter_->date();

    QDateTime startTime, endTime;
    if (startDate.isValid()) {
        startTime = QDateTime(startDate, QTime(0, 0, 0));
    }
    if (endDate.isValid()) {
        endTime = QDateTime(endDate, QTime(23, 59, 59));
    }

    QList<ExtractionRecord> records = DatabaseManager::instance().searchExtractionRecords(
        keyword, startTime, endTime);
    refreshTable(records);
}

void HistoryPanel::onFilterChanged() {
    // 自动搜索？暂不实现
}

void HistoryPanel::onItemClicked(int row, int column) {
    Q_UNUSED(column);
    QTableWidgetItem* item = tableWidget_->item(row, 0);
    if (item) {
        emit recordClicked(item->text().toLongLong());
    }
}

void HistoryPanel::onItemDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    QTableWidgetItem* item = tableWidget_->item(row, 0);
    if (item) {
        emit recordDoubleClicked(item->text().toLongLong());
    }
}

void HistoryPanel::onExport() {
    QList<qint64> ids = selectedRecordIds();
    if (ids.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择要导出的记录"));
        return;
    }
    emit exportClicked(ids);
}

void HistoryPanel::onBatchDelete() {
    QList<qint64> ids = selectedRecordIds();
    if (ids.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择要删除的记录"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("确认删除"),
        tr("确定要删除选中的 %1 条记录吗？").arg(ids.size()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        emit batchDeleteClicked(ids);
    }
}

void HistoryPanel::onClearFilter() {
    searchEdit_->clear();
    entityTypeFilter_->setCurrentIndex(0);
    startDateFilter_->setDate(QDate::currentDate().addDays(-30));
    endDateFilter_->setDate(QDate::currentDate());
    loadHistory();
}

} // namespace optikg