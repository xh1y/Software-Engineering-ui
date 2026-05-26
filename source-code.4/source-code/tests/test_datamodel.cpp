#include <QTest>
#include <QColor>
#include "optikg/datamodel.h"

using namespace optikg;

class TestDataModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

    // Entity 测试
    void testEntityDefaultConstruction() {
        Entity e;
        QCOMPARE(e.name, QString());
        QCOMPARE(static_cast<int>(e.type), static_cast<int>(EntityType::Component));
        QCOMPARE(e.confidence, 0.0f);
    }

    void testEntityParameterizedConstruction() {
        Entity e("轴承", EntityType::Component, 0.95f);
        QCOMPARE(e.name, QString("轴承"));
        QCOMPARE(static_cast<int>(e.type), static_cast<int>(EntityType::Component));
        QCOMPARE(e.confidence, 0.95f);
    }

    void testEntityTypeToString() {
        QCOMPARE(Entity::typeToString(EntityType::Component), QString("部件"));
        QCOMPARE(Entity::typeToString(EntityType::Fault), QString("故障"));
        QCOMPARE(Entity::typeToString(EntityType::Tool), QString("工具"));
        QCOMPARE(Entity::typeToString(EntityType::Composition), QString("组成"));
    }

    void testEntityStringToType() {
        QCOMPARE(Entity::stringToType("部件"), EntityType::Component);
        QCOMPARE(Entity::stringToType("故障"), EntityType::Fault);
        QCOMPARE(Entity::stringToType("工具"), EntityType::Tool);
        QCOMPARE(Entity::stringToType("组成"), EntityType::Composition);
        QCOMPARE(Entity::stringToType("未知"), EntityType::Component); // 默认回退
    }

    void testEntityTypeToColor() {
        QCOMPARE(Entity("test", EntityType::Component, 1.0f).typeToColor(), QColor(255, 107, 107));
        QCOMPARE(Entity("test", EntityType::Fault, 1.0f).typeToColor(), QColor(78, 205, 196));
        QCOMPARE(Entity("test", EntityType::Tool, 1.0f).typeToColor(), QColor(255, 193, 7));
        QCOMPARE(Entity("test", EntityType::Composition, 1.0f).typeToColor(), QColor(156, 39, 176));
    }

    void testEntityQHash() {
        QHash<EntityType, QString> hash;
        hash[EntityType::Component] = "C";
        hash[EntityType::Fault] = "F";
        QCOMPARE(hash.size(), 2);
        QVERIFY(hash.contains(EntityType::Component));
    }

    // Triple 测试
    void testTripleDefaultConstruction() {
        Triple t;
        QCOMPARE(t.relation, QString());
        QCOMPARE(t.confidence, 0.0f);
    }

    void testTripleParameterizedConstruction() {
        Entity s("轴承", EntityType::Component, 0.9f);
        Entity o("磨损", EntityType::Fault, 0.85f);
        Triple t(s, o, "相关", 0.88f);
        QCOMPARE(t.subject.name, QString("轴承"));
        QCOMPARE(t.object.name, QString("磨损"));
        QCOMPARE(t.relation, QString("相关"));
        QCOMPARE(t.confidence, 0.88f);
    }

    void testTripleRelationTypeToString() {
        QCOMPARE(Triple::relationTypeToString(RelationType::ComponentFault), QString("部件故障"));
        QCOMPARE(Triple::relationTypeToString(RelationType::PerformanceFault), QString("性能故障"));
        QCOMPARE(Triple::relationTypeToString(RelationType::DetectionTool), QString("检测工具"));
        QCOMPARE(Triple::relationTypeToString(RelationType::CompositionRel), QString("组成"));
    }

    void testTripleStringToRelationType() {
        QCOMPARE(Triple::stringToRelationType("部件故障"), RelationType::ComponentFault);
        QCOMPARE(Triple::stringToRelationType("性能故障"), RelationType::PerformanceFault);
        QCOMPARE(Triple::stringToRelationType("检测工具"), RelationType::DetectionTool);
        QCOMPARE(Triple::stringToRelationType("组成"), RelationType::CompositionRel);
        QCOMPARE(Triple::stringToRelationType("未知"), RelationType::ComponentFault); // 默认回退
    }

    // ExtractionRecord 测试
    void testExtractionRecordEmptyResults() {
        ExtractionRecord rec("测试文本", QList<Triple>(), 120);
        QCOMPARE(rec.content, QString("测试文本"));
        QCOMPARE(rec.processTimeMs, 120);
        QCOMPARE(rec.avgConfidence, 0.0f);
        QCOMPARE(rec.entityCount, 0);
        QCOMPARE(rec.relationCount, 0);
        QVERIFY(rec.createdAt.isValid());
    }

    void testExtractionRecordWithResults() {
        QList<Triple> triples;
        Entity s1("轴承", EntityType::Component, 0.8f);
        Entity o1("磨损", EntityType::Fault, 0.9f);
        triples.append(Triple(s1, o1, "相关", 0.85f));

        Entity s2("电机", EntityType::Component, 0.7f);
        Entity o2("异响", EntityType::Fault, 0.6f);
        triples.append(Triple(s2, o2, "相关", 0.65f));

        ExtractionRecord rec("电机轴承磨损异响", triples, 200);
        QCOMPARE(rec.entityCount, 4);
        QCOMPARE(rec.relationCount, 2);
        QCOMPARE(rec.avgConfidence, 0.75f); // (0.85 + 0.65) / 2
    }

    // GraphNode / GraphEdge 测试
    void testGraphNodeFromEntity() {
        Entity e("轴承", EntityType::Component, 0.9f);
        GraphNode node(e, 100.0, 200.0);
        QCOMPARE(node.id, QString("轴承"));
        QCOMPARE(node.name, QString("轴承"));
        QCOMPARE(node.type, EntityType::Component);
        QCOMPARE(node.color, e.typeToColor());
        QCOMPARE(node.x, 100.0);
        QCOMPARE(node.y, 200.0);
    }

    void testGraphEdgeConstruction() {
        GraphEdge edge("A", "B", "相关", 0.8f);
        QCOMPARE(edge.source, QString("A"));
        QCOMPARE(edge.target, QString("B"));
        QCOMPARE(edge.relation, QString("相关"));
        QCOMPARE(edge.confidence, 0.8f);
    }
};

QTEST_MAIN(TestDataModel)
#include "test_datamodel.moc"
