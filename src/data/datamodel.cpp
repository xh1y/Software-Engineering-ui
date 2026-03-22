#include "optikg/datamodel.h"

#include <QCoreApplication>

namespace optikg {

QString Entity::typeToString() const {
    return typeToString(type);
}

QString Entity::typeToString(EntityType type) {
    switch (type) {
        case EntityType::Component: return QCoreApplication::translate("Entity", "部件");
        case EntityType::Fault: return QCoreApplication::translate("Entity", "故障");
        case EntityType::Tool: return QCoreApplication::translate("Entity", "工具");
        case EntityType::Composition: return QCoreApplication::translate("Entity", "组成");
        default: return QCoreApplication::translate("Entity", "未知");
    }
}

EntityType Entity::stringToType(const QString& str) {
    if (str == "部件" || str == QCoreApplication::translate("Entity", "部件")) return EntityType::Component;
    if (str == "故障" || str == QCoreApplication::translate("Entity", "故障")) return EntityType::Fault;
    if (str == "工具" || str == QCoreApplication::translate("Entity", "工具")) return EntityType::Tool;
    if (str == "组成" || str == QCoreApplication::translate("Entity", "组成")) return EntityType::Composition;
    return EntityType::Component; // 默认
}

QColor Entity::typeToColor() const {
    switch (type) {
        case EntityType::Component: return QColor(255, 107, 107); // #FF6B6B
        case EntityType::Fault: return QColor(78, 205, 196);      // #4ECDC4
        case EntityType::Tool: return QColor(255, 193, 7);        // #FFC107
        case EntityType::Composition: return QColor(156, 39, 176); // #9C27B0
        default: return Qt::gray;
    }
}

QString Triple::relationTypeToString() const {
    return relationTypeToString(relationType);
}

QString Triple::relationTypeToString(RelationType type) {
    switch (type) {
        case RelationType::ComponentFault: return QCoreApplication::translate("Relation", "部件故障");
        case RelationType::PerformanceFault: return QCoreApplication::translate("Relation", "性能故障");
        case RelationType::DetectionTool: return QCoreApplication::translate("Relation", "检测工具");
        case RelationType::CompositionRel: return QCoreApplication::translate("Relation", "组成");
        default: return QCoreApplication::translate("Relation", "未知");
    }
}

RelationType Triple::stringToRelationType(const QString& str) {
    if (str == "部件故障" || str == QCoreApplication::translate("Relation", "部件故障")) return RelationType::ComponentFault;
    if (str == "性能故障" || str == QCoreApplication::translate("Relation", "性能故障")) return RelationType::PerformanceFault;
    if (str == "检测工具" || str == QCoreApplication::translate("Relation", "检测工具")) return RelationType::DetectionTool;
    if (str == "组成" || str == QCoreApplication::translate("Relation", "组成")) return RelationType::CompositionRel;
    return RelationType::ComponentFault; // 默认
}

} // namespace optikg