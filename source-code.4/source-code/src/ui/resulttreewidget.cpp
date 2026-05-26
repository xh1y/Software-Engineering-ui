#include "resulttreewidget.h"

#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QColor>
#include <QBrush>

namespace optikg {

ResultTreeWidget::ResultTreeWidget(QWidget *parent)
    : QTreeWidget(parent) {
    setupUi();
}

void ResultTreeWidget::setupUi() {
    setColumnCount(3);
    setHeaderLabels({tr("名称"), tr("类型"), tr("置信度")});

    setAlternatingRowColors(false); // 现代设计一般不交替背景色
    setFrameShape(QFrame::NoFrame); // 无边框
    setIndentation(15);
    setAnimated(true);
    setSortingEnabled(true);
    sortByColumn(2, Qt::DescendingOrder); // 按置信度降序排序

    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(false);
    header()->resizeSection(0, 200);
    header()->resizeSection(1, 100);
    header()->resizeSection(2, 80);

    // 通过样式表增加行高
    setStyleSheet("QTreeView::item { height: 35px; }");

    connect(this, &QTreeWidget::itemClicked, [this](QTreeWidgetItem* item, int column) {
        Q_UNUSED(column);
        QVariant data = item->data(0, Qt::UserRole);
        if (data.isValid()) {
            if (data.canConvert<Entity>()) {
                Entity entity = data.value<Entity>();
                emit entityClicked(entity.name, entity.typeToString());
            } else if (data.canConvert<Triple>()) {
                Triple triple = data.value<Triple>();
                emit tripleClicked(triple);
            }
        }
    });
}

void ResultTreeWidget::setResults(const QList<Triple>& triples) {
    clearResults();

    // 按实体类型分组
    QHash<EntityType, QList<Entity>> entitiesByType;
    QHash<QString, QList<Triple>> triplesByRelation;

    for (const auto& triple : triples) {
        entitiesByType[triple.subject.type].append(triple.subject);
        entitiesByType[triple.object.type].append(triple.object);
        triplesByRelation[triple.relation].append(triple);
    }

    // 添加实体分组
    for (auto it = entitiesByType.begin(); it != entitiesByType.end(); ++it) {
        EntityType type = it.key();
        const QList<Entity>& entities = it.value();

        QTreeWidgetItem* typeItem = new QTreeWidgetItem(this);
        typeItem->setText(0, Entity::typeToString(type) + tr(" (%1)").arg(entities.size()));
        typeItem->setIcon(0, QIcon());
        typeItem->setExpanded(true);

        // 去重
        QHash<QString, Entity> uniqueEntities;
        for (const auto& entity : entities) {
            if (!uniqueEntities.contains(entity.name)) {
                uniqueEntities[entity.name] = entity;
            }
        }

        for (const auto& entity : uniqueEntities) {
            QTreeWidgetItem* entityItem = createEntityItem(entity);
            typeItem->addChild(entityItem);
        }
    }

    // 添加关系分组
    for (auto it = triplesByRelation.begin(); it != triplesByRelation.end(); ++it) {
        const QString& relation = it.key();
        const QList<Triple>& relationTriples = it.value();

        QTreeWidgetItem* relationItem = new QTreeWidgetItem(this);
        relationItem->setText(0, relation + tr(" (%1)").arg(relationTriples.size()));
        relationItem->setIcon(0, QIcon());
        relationItem->setExpanded(true);

        for (const auto& triple : relationTriples) {
            QTreeWidgetItem* tripleItem = createTripleItem(triple);
            relationItem->addChild(tripleItem);
        }
    }

    // 如果没有结果，显示提示
    if (triples.isEmpty()) {
        QTreeWidgetItem* emptyItem = new QTreeWidgetItem(this);
        emptyItem->setText(0, tr("暂无抽取结果"));
        emptyItem->setTextAlignment(0, Qt::AlignCenter);
        emptyItem->setForeground(0, QBrush(Qt::gray));
        emptyItem->setFlags(Qt::NoItemFlags);
    }
}

void ResultTreeWidget::clearResults() {
    clear();
}

QTreeWidgetItem* ResultTreeWidget::createEntityItem(const Entity& entity) {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, entity.name);
    item->setText(1, entity.typeToString());
    item->setText(2, confidenceToString(entity.confidence));

    // 颜色根据置信度
    item->setForeground(2, QBrush(confidenceToColor(entity.confidence)));

    // 存储实体数据
    item->setData(0, Qt::UserRole, QVariant::fromValue(entity));

    return item;
}

QTreeWidgetItem* ResultTreeWidget::createTripleItem(const Triple& triple) {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    QString text = QString("%1 → %2").arg(triple.subject.name, triple.object.name);
    item->setText(0, text);
    item->setText(1, triple.relation);
    item->setText(2, confidenceToString(triple.confidence));

    // 颜色根据置信度
    item->setForeground(2, QBrush(confidenceToColor(triple.confidence)));

    // 存储三元组数据
    item->setData(0, Qt::UserRole, QVariant::fromValue(triple));

    return item;
}

QString ResultTreeWidget::confidenceToString(float confidence) {
    return QString::number(confidence, 'f', 2);
}

QColor ResultTreeWidget::confidenceToColor(float confidence) {
    if (confidence >= 0.8) return QColor(0, 150, 0); // 高置信度：绿色
    if (confidence >= 0.6) return QColor(255, 140, 0); // 中置信度：橙色
    return QColor(220, 0, 0); // 低置信度：红色
}

} // namespace optikg