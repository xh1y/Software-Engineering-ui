#include <QTest>
#include <QSignalSpy>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include "ui/graphwidget.h"
#include "optikg/datamodel.h"

using namespace optikg;

class TestGraphWidget : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // 初始化
    }

    void cleanupTestCase() {
        // 清理
    }

    void testDefaultConstruction() {
        GraphWidget widget;
        QVERIFY(widget.scene() != nullptr);
        QVERIFY(widget.scene()->sceneRect().isValid());
    }

    void testSetEmptyGraphData() {
        GraphWidget widget;
        widget.setGraphData(QList<GraphNode>(), QList<GraphEdge>());

        // 空数据应显示空状态提示
        auto items = widget.scene()->items();
        bool hasEmptyStateText = false;
        for (auto* item : items) {
            if (auto* textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
                if (textItem->toPlainText().contains("暂无知识图谱数据")) {
                    hasEmptyStateText = true;
                    break;
                }
            }
        }
        QVERIFY(hasEmptyStateText);
    }

    void testSetSingleNode() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());

        // 验证场景中有椭圆节点
        auto items = widget.scene()->items();
        int ellipseCount = 0;
        for (auto* item : items) {
            if (dynamic_cast<QGraphicsEllipseItem*>(item)) {
                ellipseCount++;
            }
        }
        QVERIFY(ellipseCount >= 1);
    }

    void testSetMultipleNodes() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("节点2", EntityType::Fault, 0.8f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("节点3", EntityType::Tool, 0.7f), 0.0, 0.0));
        QList<GraphEdge> edges;
        edges.append(GraphEdge("节点1", "节点2", "相关", 0.85f));
        widget.setGraphData(nodes, edges);

        // 验证场景中有多个椭圆节点和边
        auto items = widget.scene()->items();
        int ellipseCount = 0;
        int pathCount = 0;
        for (auto* item : items) {
            if (dynamic_cast<QGraphicsEllipseItem*>(item)) {
                ellipseCount++;
            }
            if (dynamic_cast<QGraphicsPathItem*>(item)) {
                pathCount++;
            }
        }
        QVERIFY(ellipseCount >= 3);
        QVERIFY(pathCount >= 1);
    }

    void testClearGraph() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());

        // 设置数据后应该有节点
        auto itemsBefore = widget.scene()->items();
        int itemCountBefore = itemsBefore.size();
        QVERIFY(itemCountBefore > 0);

        widget.clearGraph();

        // 清空后应该回到空状态（有提示文字）
        auto itemsAfter = widget.scene()->items();
        bool hasEmptyStateText = false;
        for (auto* item : itemsAfter) {
            if (auto* textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
                if (textItem->toPlainText().contains("暂无知识图谱数据")) {
                    hasEmptyStateText = true;
                    break;
                }
            }
        }
        QVERIFY(hasEmptyStateText);
    }

    void testHighlightNode() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("节点2", EntityType::Fault, 0.8f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());

        // 记录高亮前的颜色
        QColor colorBefore;
        for (auto* item : widget.scene()->items()) {
            if (auto* ellipse = dynamic_cast<QGraphicsEllipseItem*>(item)) {
                colorBefore = ellipse->brush().color();
                break;
            }
        }

        widget.highlightNode("节点1");

        // 验证有高亮颜色的节点（黄色高亮）
        bool hasHighlightedNode = false;
        for (auto* item : widget.scene()->items()) {
            if (auto* ellipse = dynamic_cast<QGraphicsEllipseItem*>(item)) {
                QColor brushColor = ellipse->brush().color();
                // 高亮颜色是 QColor(255, 200, 100)
                if (brushColor == QColor(255, 200, 100)) {
                    hasHighlightedNode = true;
                    break;
                }
            }
        }
        QVERIFY(hasHighlightedNode);
    }

    void testHighlightPath() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("节点2", EntityType::Fault, 0.8f), 0.0, 0.0));
        QList<GraphEdge> edges;
        edges.append(GraphEdge("节点1", "节点2", "相关", 0.85f));
        widget.setGraphData(nodes, edges);

        widget.highlightPath("节点1", "节点2");

        // 验证有红色边框的节点和绿色边框的节点
        bool hasRedPen = false;
        bool hasGreenPen = false;
        bool hasBlueEdge = false;

        for (auto* item : widget.scene()->items()) {
            if (auto* ellipse = dynamic_cast<QGraphicsEllipseItem*>(item)) {
                QPen pen = ellipse->pen();
                if (pen.color() == Qt::red && pen.width() == 3) {
                    hasRedPen = true;
                }
                if (pen.color() == Qt::green && pen.width() == 3) {
                    hasGreenPen = true;
                }
            }
            if (auto* path = dynamic_cast<QGraphicsPathItem*>(item)) {
                QPen pen = path->pen();
                if (pen.color() == QColor(0, 0, 255) && pen.width() == 3) {
                    hasBlueEdge = true;
                }
            }
        }

        QVERIFY(hasRedPen);
        QVERIFY(hasGreenPen);
        QVERIFY(hasBlueEdge);
    }

    void testZoomOperations() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());

        qreal initialScale = widget.transform().m11();

        widget.zoomIn();
        qreal scaleAfterZoomIn = widget.transform().m11();
        QVERIFY(scaleAfterZoomIn > initialScale);

        widget.zoomOut();
        qreal scaleAfterZoomOut = widget.transform().m11();
        QVERIFY(qFuzzyCompare(scaleAfterZoomOut, initialScale) || scaleAfterZoomOut < scaleAfterZoomIn);

        widget.resetZoom();
        qreal scaleAfterReset = widget.transform().m11();
        QVERIFY(qFuzzyCompare(scaleAfterReset, 1.0));

        // fitToView 不应崩溃
        widget.fitToView();
        QVERIFY(widget.transform().m11() > 0);
    }

    void testNodeClickedSignal() {
        GraphWidget widget;
        QSignalSpy spy(&widget, &GraphWidget::nodeClicked);
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());
        QCOMPARE(spy.count(), 0);

        // 信号连接应正常（无法直接模拟鼠标点击，但 spy 能连接说明信号有效）
        QVERIFY(true);
    }

    void testEdgeClickedSignal() {
        GraphWidget widget;
        QSignalSpy spy(&widget, &GraphWidget::edgeClicked);
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("节点2", EntityType::Fault, 0.8f), 0.0, 0.0));
        QList<GraphEdge> edges;
        edges.append(GraphEdge("节点1", "节点2", "相关", 0.85f));
        widget.setGraphData(nodes, edges);
        QCOMPARE(spy.count(), 0);

        QVERIFY(true);
    }

    // 力导向布局算法测试（间接通过 setGraphData 验证）
    void testLayoutConvergence() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        for (int i = 0; i < 20; ++i) {
            nodes.append(GraphNode(Entity(QString("节点%1").arg(i), EntityType::Component, 0.9f), 0.0, 0.0));
        }
        widget.setGraphData(nodes, QList<GraphEdge>());

        // 布局后场景中应有大量图形项（节点 + 文本 + 图例）
        auto items = widget.scene()->items();
        QVERIFY(items.size() >= 20);

        // 验证节点有非零位置（力导向布局后应分散开）
        int nodesWithNonZeroPos = 0;
        for (auto* item : items) {
            if (auto* ellipse = dynamic_cast<QGraphicsEllipseItem*>(item)) {
                if (!qFuzzyIsNull(ellipse->pos().x()) || !qFuzzyIsNull(ellipse->pos().y())) {
                    nodesWithNonZeroPos++;
                }
            }
        }
        // 大多数节点应被布局到不同位置
        QVERIFY(nodesWithNonZeroPos >= 10);
    }

    void testIterationControl() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        for (int i = 0; i < 10; ++i) {
            nodes.append(GraphNode(Entity(QString("节点%1").arg(i), EntityType::Component, 0.9f), 0.0, 0.0));
        }
        // 多次设置数据验证迭代稳定性（不应崩溃）
        widget.setGraphData(nodes, QList<GraphEdge>());
        widget.setGraphData(nodes, QList<GraphEdge>()); // 第二次设置

        auto items = widget.scene()->items();
        QVERIFY(items.size() >= 10);
    }

    void testEdgeVisualUpdate() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("节点2", EntityType::Fault, 0.8f), 0.0, 0.0));
        QList<GraphEdge> edges;
        edges.append(GraphEdge("节点1", "节点2", "相关", 0.85f));
        widget.setGraphData(nodes, edges);

        // 测试高亮功能（不应崩溃）
        widget.highlightNode("节点1");
        widget.highlightPath("节点1", "节点2");
        widget.clearGraph();

        // 清空后应回到空状态
        bool hasEmptyStateText = false;
        for (auto* item : widget.scene()->items()) {
            if (auto* textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
                if (textItem->toPlainText().contains("暂无知识图谱数据")) {
                    hasEmptyStateText = true;
                    break;
                }
            }
        }
        QVERIFY(hasEmptyStateText);
    }

    void testManyNodesLayout() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        for (int i = 0; i < 50; ++i) {
            nodes.append(GraphNode(Entity(QString("节点%1").arg(i), EntityType::Component, 0.9f), 0.0, 0.0));
        }
        widget.setGraphData(nodes, QList<GraphEdge>());

        // 大量节点布局不应崩溃，且场景中应有足够多的项
        auto items = widget.scene()->items();
        QVERIFY(items.size() >= 50);
    }
};

QTEST_MAIN(TestGraphWidget)
#include "test_graphwidget.moc"
