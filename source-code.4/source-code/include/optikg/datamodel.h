#pragma once

#include <QString>
#include <QColor>
#include <QDateTime>
#include <QList>

namespace optikg {
    // 实体类型枚举 (EntityType)
    enum class EntityType {
        Component = 0, // 部件
        Fault = 1, // 故障
        Tool = 2, // 工具
        Composition = 3 // 组成
    };

    inline size_t qHash(EntityType key, size_t seed = 0) noexcept {
        return ::qHash(static_cast<uint>(key), seed);
    }

    // 关系类型枚举 (RelationType)
    enum class RelationType {
        ComponentFault = 0, // 部件故障
        PerformanceFault = 1, // 性能故障
        DetectionTool = 2, // 检测工具
        CompositionRel = 3 // 组成
    };

    inline size_t qHash(RelationType key, size_t seed = 0) noexcept {
        return ::qHash(static_cast<uint>(key), seed);
    }

    // 实体结构
    struct Entity {
        int id = -1;
        QString name;
        EntityType type = EntityType::Component;
        int startPos = -1;
        int endPos = -1;
        float confidence = 0.0f;

        Entity() = default;

        Entity(const QString &name, EntityType type, float confidence)
            : name(name), type(type), confidence(confidence) {
        }

        QString typeToString() const;

        static QString typeToString(EntityType type);

        static EntityType stringToType(const QString &str);

        QColor typeToColor() const;
    };

    // 三元组结构
    struct Triple {
        Entity subject;
        Entity object;
        QString relation;
        RelationType relationType = RelationType::ComponentFault;
        float confidence = 0.0f;

        Triple() = default;

        Triple(const Entity &subj, const Entity &obj, const QString &rel, float conf)
            : subject(subj), object(obj), relation(rel), confidence(conf) {
        }

        QString relationTypeToString() const;

        static QString relationTypeToString(RelationType type);

        static RelationType stringToRelationType(const QString &str);
    };

    // 抽取记录
    struct ExtractionRecord {
        qint64 id = -1;
        QString content;
        QDateTime createdAt;
        int processTimeMs = 0;
        float avgConfidence = 0.0f;
        int entityCount = 0;
        int relationCount = 0;
        QList<Triple> triples;

        ExtractionRecord() = default;

        ExtractionRecord(const QString &text, const QList<Triple> &results, int processTime)
            : content(text), processTimeMs(processTime), triples(results) {
            createdAt = QDateTime::currentDateTime();
            if (!results.isEmpty()) {
                float total = 0.0f;
                for (const auto &triple: results) {
                    total += triple.confidence;
                }
                avgConfidence = total / results.size();
            }
            entityCount = results.size() * 2; // 每个三元组有两个实体
            relationCount = results.size();
        }
    };

    // 图节点数据
    struct GraphNode {
        QString id;
        QString name;
        EntityType type;
        QColor color;
        qreal x = 0.0;
        qreal y = 0.0;

        GraphNode() = default;

        GraphNode(const Entity &entity, qreal x = 0.0, qreal y = 0.0)
            : id(entity.name), name(entity.name), type(entity.type),
              color(entity.typeToColor()), x(x), y(y) {
        }
    };

    // 图边数据
    struct GraphEdge {
        QString source;
        QString target;
        QString relation;
        float confidence = 0.0f;

        GraphEdge() = default;

        GraphEdge(const QString &src, const QString &tgt, const QString &rel, float conf)
            : source(src), target(tgt), relation(rel), confidence(conf) {
        }
    };
} // namespace optikg
