#pragma once

#include <QTreeWidget>
#include "optikg/datamodel.h"

namespace optikg {

class ResultTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit ResultTreeWidget(QWidget *parent = nullptr);

    void setResults(const QList<Triple>& triples);
    void clearResults();

signals:
    void entityClicked(const QString& entityName, const QString& type);
    void tripleClicked(const Triple& triple);

private:
    void setupUi();
    QTreeWidgetItem* createEntityItem(const Entity& entity);
    QTreeWidgetItem* createTripleItem(const Triple& triple);
    QString confidenceToString(float confidence);
    QColor confidenceToColor(float confidence);
};

} // namespace optikg