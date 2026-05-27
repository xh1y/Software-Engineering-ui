#pragma once

#include <QGraphicsView>
#include <QGraphicsPathItem>
#include <QGraphicsEllipseItem>
#include <QHash>
#include <QPair>
#include "optikg/datamodel.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsItem;
QT_END_NAMESPACE

namespace optikg {

class GraphNode;
class GraphEdge;
class GraphWidget;  // 前置声明

class GraphNodeItem : public QGraphicsEllipseItem {
public:
    explicit GraphNodeItem(const QString& nodeId, GraphWidget* graphWidget,
                          qreal x, qreal y, qreal width, qreal height,
                          QGraphicsItem* parent = nullptr);

    QString nodeId() const { return nodeId_; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QString nodeId_;
    GraphWidget* graphWidget_;  // 指向父widget的指针
};

class GraphWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);

    void setGraphData(const QList<GraphNode>& nodes, const QList<GraphEdge>& edges);
    void clearGraph();
    void highlightNode(const QString& nodeId);
    void highlightPath(const QString& sourceId, const QString& targetId);

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToView();

    // 供GraphNodeItem调用的方法
    void handleNodePositionChanged(const QString& nodeId, const QPointF& newPos);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupScene();
    void createNode(const GraphNode& data);
    void createEdge(const GraphEdge& data);
    void computeForceDirectedLayout(qreal repulsionK = 100.0, qreal attractionK = 0.05,
                                     qreal damping = 0.85, int iterations = 100);
    void updateLegend();
    void updateConnectedEdges(const QString& nodeId);
    void showEmptyState();
    void hideEmptyState();
    qreal calculateOptimalRadius(int nodeCount);
    void clearGraphData();  ///< 清理节点和边数据对象

private:
    QGraphicsScene* scene_;
    QHash<QString, GraphNode*> nodes_;
    QList<GraphEdge*> edges_;
    QHash<QString, GraphNodeItem*> nodeItems_;           // 节点ID -> 图形项
    QHash<QString, QList<GraphEdge*>> nodeEdges_;        // 节点ID -> 连接的边列表
    QHash<QPair<QString, QString>, QGraphicsPathItem*> edgeItems_; // (源,目标) -> 连线项
    QHash<QPair<QString, QString>, QGraphicsSimpleTextItem*> edgeTextItems_; // (源,目标) -> 关系文本项

    bool dragging_;
    QPoint lastMousePos_;
    qreal scaleFactor_;

    // 空状态相关
    QGraphicsItemGroup* emptyStateGroup_ = nullptr;
    bool hasData_ = false;

    // 颜色配置
    static const QHash<QString, QColor> TYPE_COLORS;
};

} // namespace optikg
