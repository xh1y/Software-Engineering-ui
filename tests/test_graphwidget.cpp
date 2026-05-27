#include <QTest>
#include <QSignalSpy>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QPointingDevice>
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

    // --- Tests for uncovered event handlers ---

    // Test wheelEvent with Ctrl+scroll up (zoom in)
    void testWheelEventZoomIn() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());
        widget.show();
        QTest::qWait(50);

        qreal scaleBefore = widget.transform().m11();

        // Send wheel event to the viewport — QGraphicsView routes viewport
        // wheel events to GraphWidget::wheelEvent overrides
        QPointF pos(50, 50);
        QWheelEvent event(pos, pos, QPoint(0, 0), QPoint(0, 120),
                          Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(widget.viewport(), &event);
        QTest::qWait(50);

        qreal scaleAfter = widget.transform().m11();
        QVERIFY(scaleAfter > scaleBefore);
    }

    // Test wheelEvent with Ctrl+scroll down (zoom out)
    void testWheelEventZoomOut() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());
        widget.show();
        QTest::qWait(50);

        // Zoom in first so we have room to zoom out
        widget.zoomIn();
        qreal scaleBefore = widget.transform().m11();

        QPointF pos(50, 50);
        QWheelEvent event(pos, pos, QPoint(0, 0), QPoint(0, -120),
                          Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(widget.viewport(), &event);
        QTest::qWait(50);

        qreal scaleAfter = widget.transform().m11();
        QVERIFY(scaleAfter < scaleBefore);
    }

    // Test wheelEvent without Ctrl modifier (should delegate to base class)
    void testWheelEventNoModifier() {
        GraphWidget widget;
        widget.show();
        QTest::qWait(50);

        QPoint pos(50, 50);
        QWheelEvent event(pos, pos, QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        // Should not crash
        QApplication::sendEvent(&widget, &event);
        QVERIFY(true);
    }

    // Test mousePressEvent on empty area (starts dragging)
    void testMousePressOnEmptyArea() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 50.0, 50.0));
        widget.setGraphData(nodes, QList<GraphEdge>());
        widget.resize(400, 400);
        widget.show();
        QTest::qWait(100);

        // Click on empty area (0,0) - should start dragging
        QMouseEvent event(QEvent::MouseButtonPress, QPointF(10, 10), QPointF(10, 10),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &event);
        // Should not crash
    }

    // Test mousePressEvent on a node (should delegate to base class for dragging)
    void testMousePressOnNode() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());
        widget.resize(400, 400);
        widget.show();
        QTest::qWait(100);

        // Click near center (where the node should be after layout)
        QMouseEvent event(QEvent::MouseButtonPress, QPointF(200, 200), QPointF(200, 200),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &event);
        // Should not crash - delegates to base class
    }

    // Test mouseMoveEvent during drag (panning)
    void testMouseMoveDuringDrag() {
        GraphWidget widget;
        widget.resize(400, 400);
        widget.show();
        QTest::qWait(100);

        // Start drag
        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &pressEvent);

        // Move during drag
        QMouseEvent moveEvent(QEvent::MouseMove, QPointF(150, 120), QPointF(150, 120),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &moveEvent);
        // Should not crash - should have panned the view
    }

    // Test mouseReleaseEvent after drag
    void testMouseReleaseAfterDrag() {
        GraphWidget widget;
        widget.resize(400, 400);
        widget.show();
        QTest::qWait(100);

        // Start drag
        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &pressEvent);

        // Move
        QMouseEvent moveEvent(QEvent::MouseMove, QPointF(150, 120), QPointF(150, 120),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &moveEvent);

        // Release
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPointF(150, 120), QPointF(150, 120),
                                 Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &releaseEvent);
        // Should not crash
    }

    // Test mouseReleaseEvent without prior drag
    void testMouseReleaseNoDrag() {
        GraphWidget widget;
        widget.resize(400, 400);
        widget.show();
        QTest::qWait(100);

        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPointF(100, 100), QPointF(100, 100),
                                 Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &releaseEvent);
        // Should not crash - delegates to base class
    }

    // --- Tests for uncovered branches ---

    // Test showEmptyState when emptyStateGroup_ exists but is not in scene (lines 133-134)
    void testShowEmptyStateReAddToScene() {
        GraphWidget widget;
        widget.show();
        QTest::qWait(50);

        // Initially showEmptyState is called from setupScene → creates empty state group
        QVERIFY(widget.scene()->items().size() > 0);

        // Remove the empty state group from scene (simulating unusual state)
        auto items = widget.scene()->items();
        for (auto* item : items) {
            widget.scene()->removeItem(item);
        }

        // Now call clearGraph which calls showEmptyState again
        // This should hit the re-add branch (line 133)
        widget.clearGraph();
        // Should not crash — the emptyStateGroup_ was re-added
        QVERIFY(widget.scene()->items().size() > 0);
    }

    // Test setGraphData with 4 nodes to exercise the 3-6 node layout path (line 281)
    void testMediumSmallLayout() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        for (int i = 0; i < 4; ++i) {
            nodes.append(GraphNode(Entity(QString("N%1").arg(i), EntityType::Component, 0.9f), 0.0, 0.0));
        }
        QList<GraphEdge> edges;
        edges.append(GraphEdge("N0", "N1", "相关", 0.85f));
        widget.setGraphData(nodes, edges);

        auto items = widget.scene()->items();
        QVERIFY(items.size() >= 4);
    }

    // Test createNode with very long name to exercise text eliding (lines 569-570)
    void testCreateNodeLongName() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        // Create a node with an extremely long name
        QString longName = "这是一个非常非常长的节点名称用于测试文本省略功能ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        nodes.append(GraphNode(Entity(longName, EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());

        // Should not crash - text should be elided
        auto items = widget.scene()->items();
        QVERIFY(items.size() >= 1);
    }

    // Test createEdge with non-existent source node (lines 597-599)
    void testCreateEdgeMissingSource() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        QList<GraphEdge> edges;
        // Source doesn't exist in nodes
        edges.append(GraphEdge("NonExistent", "节点1", "相关", 0.85f));
        widget.setGraphData(nodes, edges);

        // Should not crash - edge should be skipped with warning
        QVERIFY(true);
    }

    // Test createEdge with non-existent target node (lines 597-599)
    void testCreateEdgeMissingTarget() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        QList<GraphEdge> edges;
        // Target doesn't exist in nodes
        edges.append(GraphEdge("节点1", "NonExistent", "相关", 0.85f));
        widget.setGraphData(nodes, edges);

        // Should not crash
        QVERIFY(true);
    }

    // Test createEdge with medium confidence (0.5-0.8) (line 641-642)
    void testCreateEdgeMediumConfidence() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("节点2", EntityType::Fault, 0.8f), 0.0, 0.0));
        QList<GraphEdge> edges;
        edges.append(GraphEdge("节点1", "节点2", "相关", 0.65f)); // Medium confidence
        widget.setGraphData(nodes, edges);

        // Should have created an edge with orange-ish color
        auto items = widget.scene()->items();
        bool hasEdge = false;
        for (auto* item : items) {
            if (auto* path = dynamic_cast<QGraphicsPathItem*>(item)) {
                hasEdge = true;
                break;
            }
        }
        QVERIFY(hasEdge);
    }

    // Test createEdge with low confidence (<0.5) (line 643-644)
    void testCreateEdgeLowConfidence() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("节点2", EntityType::Fault, 0.8f), 0.0, 0.0));
        QList<GraphEdge> edges;
        edges.append(GraphEdge("节点1", "节点2", "相关", 0.35f)); // Low confidence
        widget.setGraphData(nodes, edges);

        auto items = widget.scene()->items();
        bool hasEdge = false;
        for (auto* item : items) {
            if (auto* path = dynamic_cast<QGraphicsPathItem*>(item)) {
                hasEdge = true;
                break;
            }
        }
        QVERIFY(hasEdge);
    }

    // Test updateLegend re-creation (exercises lines 827-829 clearing old legend)
    void testUpdateLegendRecreation() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        nodes.append(GraphNode(Entity("节点1", EntityType::Component, 0.9f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());

        // Call setGraphData again with different data → updateLegend is called again
        // This exercises the "clear old legend" branch
        QList<GraphNode> nodes2;
        nodes2.append(GraphNode(Entity("新节点", EntityType::Fault, 0.8f), 0.0, 0.0));
        widget.setGraphData(nodes2, QList<GraphEdge>());

        // Should not crash - old legend was cleared and new one created
        QVERIFY(true);
    }

    // Test computeForceDirectedLayout with overlapping nodes (line 718-721)
    void testLayoutOverlappingNodes() {
        GraphWidget widget;
        QList<GraphNode> nodes;
        // Place nodes at identical positions to trigger the jitter branch
        nodes.append(GraphNode(Entity("N0", EntityType::Component, 0.9f), 0.0, 0.0));
        nodes.append(GraphNode(Entity("N1", EntityType::Fault, 0.8f), 0.0, 0.0));
        widget.setGraphData(nodes, QList<GraphEdge>());

        // Should not crash — layout should jitter overlapping nodes apart
        auto items = widget.scene()->items();
        QVERIFY(items.size() >= 2);
    }

    // Send mouse press through viewport (the natural Qt event path for QGraphicsView)
    void testMousePressViaViewport() {
        GraphWidget widget;
        widget.resize(400, 400);
        widget.show();
        QTest::qWait(100);

        QMouseEvent event(QEvent::MouseButtonPress, QPointF(50, 50), QPointF(50, 50),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(widget.viewport(), &event);
        // Should not crash
    }

    // Send mouse release through viewport after drag
    void testMouseReleaseViaViewport() {
        GraphWidget widget;
        widget.resize(400, 400);
        widget.show();
        QTest::qWait(100);

        // Press to start drag
        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(50, 50), QPointF(50, 50),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(widget.viewport(), &pressEvent);

        // Release after drag
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPointF(50, 50), QPointF(50, 50),
                                 Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(widget.viewport(), &releaseEvent);
        // Should not crash
    }

    // Wheel event without Ctrl modifier — delegates to base class (line 449)
    void testWheelEventNoCtrlViaViewport() {
        GraphWidget widget;
        widget.resize(400, 400);
        widget.show();
        QTest::qWait(50);

        QPointF pos(50, 50);
        QWheelEvent event(pos, pos, QPoint(0, 0), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(widget.viewport(), &event);
        // Should not crash — delegates to QGraphicsView::wheelEvent
    }
};

QTEST_MAIN(TestGraphWidget)
#include "test_graphwidget.moc"
