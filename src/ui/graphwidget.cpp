#include "graphwidget.h"

#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QGraphicsItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsItemGroup>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QTimer>
#include <QDebug>
#include "../utils/logger.h"
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <Qt>
#include <QFont>
#include <QFontMetrics>
#include <QRectF>
#include <QVariant>
#include <QString>
#include <cmath>

namespace optikg {

// GraphNodeItem 实现
GraphNodeItem::GraphNodeItem(const QString& nodeId, GraphWidget* graphWidget,
                             qreal x, qreal y, qreal width, qreal height, QGraphicsItem* parent)
    : QGraphicsEllipseItem(x, y, width, height, parent)
    , nodeId_(nodeId)
    , graphWidget_(graphWidget) {
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

QVariant GraphNodeItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemPositionHasChanged && graphWidget_) {
        graphWidget_->handleNodePositionChanged(nodeId_, this->pos());
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

// 颜色映射 - 马卡龙色系（柔和版）
const QHash<QString, QColor> GraphWidget::TYPE_COLORS = {
    {"部件", QColor(255, 138, 128)},    // 柔和红 #FF8A80
    {"故障", QColor(128, 216, 255)},    // 柔和青 #80D8FF
    {"工具", QColor(255, 229, 127)},    // 柔和黄 #FFE57F
    {"组成", QColor(179, 136, 255)}     // 柔和紫 #B388FF
};

GraphWidget::GraphWidget(QWidget *parent)
    : QGraphicsView(parent)
    , scene_(nullptr)
    , dragging_(false)
    , lastMousePos_(0, 0)
    , scaleFactor_(1.0) {
    setupScene();
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setBackgroundBrush(QBrush(QColor(245, 245, 245)));
}

void GraphWidget::setupScene() {
    scene_ = new QGraphicsScene(this);
    scene_->setSceneRect(-400, -300, 800, 600);
    setScene(scene_);

    // 显示空状态
    showEmptyState();
}

void GraphWidget::showEmptyState() {
    if (emptyStateGroup_) {
        emptyStateGroup_->setVisible(true);
        return;
    }

    emptyStateGroup_ = new QGraphicsItemGroup();
    emptyStateGroup_->setZValue(-100); // 置于最底层
    scene_->addItem(emptyStateGroup_);

    // 1. 简约的网络图标（使用组合图形，不用path避免线条显示问题）
    // 中心圆
    QGraphicsEllipseItem* centerCircle = new QGraphicsEllipseItem(-12, -12, 24, 24, emptyStateGroup_);
    centerCircle->setBrush(QBrush(QColor(200, 200, 200, 60)));
    centerCircle->setPen(Qt::NoPen);
    centerCircle->setPos(0, -30);

    // 周围小圆
    QGraphicsEllipseItem* smallCircle1 = new QGraphicsEllipseItem(-8, -8, 16, 16, emptyStateGroup_);
    smallCircle1->setBrush(QBrush(QColor(200, 200, 200, 40)));
    smallCircle1->setPen(Qt::NoPen);
    smallCircle1->setPos(-40, -60);

    QGraphicsEllipseItem* smallCircle2 = new QGraphicsEllipseItem(-8, -8, 16, 16, emptyStateGroup_);
    smallCircle2->setBrush(QBrush(QColor(200, 200, 200, 40)));
    smallCircle2->setPen(Qt::NoPen);
    smallCircle2->setPos(40, -60);

    QGraphicsEllipseItem* smallCircle3 = new QGraphicsEllipseItem(-8, -8, 16, 16, emptyStateGroup_);
    smallCircle3->setBrush(QBrush(QColor(200, 200, 200, 40)));
    smallCircle3->setPen(Qt::NoPen);
    smallCircle3->setPos(-40, 0);

    QGraphicsEllipseItem* smallCircle4 = new QGraphicsEllipseItem(-8, -8, 16, 16, emptyStateGroup_);
    smallCircle4->setBrush(QBrush(QColor(200, 200, 200, 40)));
    smallCircle4->setPen(Qt::NoPen);
    smallCircle4->setPos(40, 0);

    // 2. 主提示文字
    QGraphicsTextItem* mainText = new QGraphicsTextItem(tr("暂无知识图谱数据"), emptyStateGroup_);
    mainText->setDefaultTextColor(QColor(150, 150, 150));
    mainText->setFont(QFont("Microsoft YaHei", 14, QFont::Medium));
    QRectF mainRect = mainText->boundingRect();
    mainText->setPos(-mainRect.width() / 2, 30);

    // 3. 副提示文字
    QGraphicsTextItem* subText = new QGraphicsTextItem(tr("在左侧输入文本，点击「开始抽取」生成知识图谱"), emptyStateGroup_);
    subText->setDefaultTextColor(QColor(180, 180, 180));
    subText->setFont(QFont("Microsoft YaHei", 11));
    QRectF subRect = subText->boundingRect();
    subText->setPos(-subRect.width() / 2, 60);

    hasData_ = false;
}

void GraphWidget::hideEmptyState() {
    if (emptyStateGroup_) {
        emptyStateGroup_->setVisible(false);
    }
    hasData_ = true;
}

qreal GraphWidget::calculateOptimalRadius(int nodeCount) {
    // 根据节点数量计算最优半径，避免节点重叠但也不要太分散
    // 每个节点大约需要 120-150px 的空间（包括边距）
    if (nodeCount <= 1) return 0;
    if (nodeCount <= 2) return 150;
    if (nodeCount <= 3) return 180;
    if (nodeCount <= 4) return 200;
    if (nodeCount <= 6) return 250;
    if (nodeCount <= 10) return 300;
    if (nodeCount <= 15) return 380;
    if (nodeCount <= 20) return 450;
    return 550;
}

void GraphWidget::setGraphData(const QList<GraphNode>& nodes, const QList<GraphEdge>& edges) {
    // 如果没有数据，显示空状态
    if (nodes.isEmpty()) {
        clearGraph();
        return;
    }

    // 有数据，隐藏空状态
    hideEmptyState();

    // 清理旧数据但保留空状态组
    for (GraphEdge* edge : edges_) {
        delete edge;
    }
    edges_.clear();

    for (GraphNode* node : nodes_) {
        delete node;
    }
    nodes_.clear();

    // 清除场景中的图形项，但保留空状态组
    QList<QGraphicsItem*> items = scene_->items();
    for (QGraphicsItem* item : items) {
        if (item != emptyStateGroup_ && item->parentItem() != emptyStateGroup_) {
            scene_->removeItem(item);
            delete item;
        }
    }
    nodeItems_.clear();
    edgeItems_.clear();
    edgeTextItems_.clear();

    // 1. 创建节点
    for (const auto& node : nodes) {
        createNode(node);
    }

    // 2. 计算圆形布局并更新节点位置
    const int nodeCount = nodes_.size();
    const qreal radius = calculateOptimalRadius(nodeCount);
    int i = 0;
    QList<QString> nodeIds = nodes_.keys();
    for (const QString& nodeId : nodeIds) {
        GraphNode* nodeData = nodes_.value(nodeId);
        QGraphicsEllipseItem* ellipse = nodeItems_.value(nodeId);
        if (nodeData && ellipse) {
            qreal angle = 2 * M_PI * i / nodeCount;
            qreal x = radius * cos(angle);
            qreal y = radius * sin(angle);
            nodeData->x = x;
            nodeData->y = y;
            ellipse->setPos(x, y);
            i++;
        }
    }

    // 3. 创建边
    for (const auto& edge : edges) {
        createEdge(edge);
    }

    // 4. 对所有节点应用力导向布局，确保节点间有足够距离但不过度分散
    if (nodeCount <= 3) {
        // 少量节点：适当斥力，避免重叠即可
        computeForceDirectedLayout(150, 0.02, 0.85, 60);
    } else if (nodeCount <= 6) {
        // 小量节点：中等斥力
        computeForceDirectedLayout(120, 0.03, 0.88, 50);
    } else if (nodeCount <= 10) {
        // 中等数量：较小斥力
        computeForceDirectedLayout(100, 0.04, 0.9, 40);
    } else {
        // 大量节点：标准力导向布局
        computeForceDirectedLayout();
    }

    // 更新图例
    updateLegend();
    fitToView();
}

void GraphWidget::clearGraph() {
    // 删除所有边数据
    for (GraphEdge* edge : edges_) {
        delete edge;
    }
    edges_.clear();

    // 删除所有节点数据
    for (GraphNode* node : nodes_) {
        delete node;
    }
    nodes_.clear();

    // 清除场景中的图形项，但保留空状态组
    QList<QGraphicsItem*> items = scene_->items();
    for (QGraphicsItem* item : items) {
        if (item != emptyStateGroup_ && item->parentItem() != emptyStateGroup_) {
            scene_->removeItem(item);
            delete item;
        }
    }
    nodeItems_.clear();
    edgeItems_.clear();
    edgeTextItems_.clear();

    // 显示空状态
    showEmptyState();
    hasData_ = false;
    fitToView();
}

void GraphWidget::highlightNode(const QString& nodeId) {
    // 重置所有节点颜色
    for (auto it = nodeItems_.begin(); it != nodeItems_.end(); ++it) {
        QGraphicsEllipseItem* ellipse = it.value();
        GraphNode* nodeData = nodes_.value(it.key());
        if (nodeData) {
            ellipse->setBrush(QBrush(nodeData->color));
            ellipse->setPen(Qt::NoPen); // 无边框
        }
    }

    // 高亮指定节点
    QGraphicsEllipseItem* ellipse = nodeItems_.value(nodeId);
    if (ellipse) {
        ellipse->setBrush(QBrush(QColor(255, 200, 100))); // 柔和黄色高亮
        ellipse->setPen(QPen(QColor(255, 165, 0), 2)); // 橙色边框
    }
}

void GraphWidget::highlightPath(const QString& sourceId, const QString& targetId) {
    // 重置所有高亮
    for (auto it = nodeItems_.begin(); it != nodeItems_.end(); ++it) {
        QGraphicsEllipseItem* ellipse = it.value();
        GraphNode* nodeData = nodes_.value(it.key());
        if (nodeData) {
            ellipse->setBrush(QBrush(nodeData->color));
            ellipse->setPen(Qt::NoPen); // 无边框
        }
    }
    for (auto it = edgeItems_.begin(); it != edgeItems_.end(); ++it) {
        QGraphicsPathItem* pathItem = it.value();
        pathItem->setPen(QPen(QColor(200, 200, 200), 1, Qt::SolidLine));
    }

    // 高亮源节点和目标节点
    QGraphicsEllipseItem* sourceEllipse = nodeItems_.value(sourceId);
    QGraphicsEllipseItem* targetEllipse = nodeItems_.value(targetId);
    if (sourceEllipse) {
        sourceEllipse->setBrush(QBrush(QColor(255, 200, 200))); // 浅红色
        sourceEllipse->setPen(QPen(Qt::red, 3));
    }
    if (targetEllipse) {
        targetEllipse->setBrush(QBrush(QColor(200, 255, 200))); // 浅绿色
        targetEllipse->setPen(QPen(Qt::green, 3));
    }

    // 高亮边
    QPair<QString, QString> key = qMakePair(sourceId, targetId);
    QGraphicsPathItem* pathItem = edgeItems_.value(key);
    if (pathItem) {
        pathItem->setPen(QPen(QColor(0, 0, 255), 3, Qt::SolidLine)); // 蓝色粗线
    }
}

void GraphWidget::zoomIn() {
    scaleFactor_ *= 1.2;
    setTransform(QTransform::fromScale(scaleFactor_, scaleFactor_));
}

void GraphWidget::zoomOut() {
    scaleFactor_ /= 1.2;
    setTransform(QTransform::fromScale(scaleFactor_, scaleFactor_));
}

void GraphWidget::resetZoom() {
    scaleFactor_ = 1.0;
    setTransform(QTransform());
}

void GraphWidget::fitToView() {
    if (!scene_->items().isEmpty()) {
        fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
        scaleFactor_ = transform().m11();
    }
}

void GraphWidget::wheelEvent(QWheelEvent *event) {
    // 缩放
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void GraphWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 检查是否点击了图形项
        QGraphicsItem* item = itemAt(event->pos());
        if (!item) {
            // 点击空白区域，开始拖动视图
            dragging_ = true;
            lastMousePos_ = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        // 点击了图形项，让基类处理（节点拖动等）
    }
    QGraphicsView::mousePressEvent(event);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event) {
    if (dragging_) {
        QScrollBar* hBar = horizontalScrollBar();
        QScrollBar* vBar = verticalScrollBar();
        QPoint delta = event->pos() - lastMousePos_;
        lastMousePos_ = event->pos();
        hBar->setValue(hBar->value() - delta.x());
        vBar->setValue(vBar->value() - delta.y());
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && dragging_) {
        dragging_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void GraphWidget::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    // 保持场景居中
    centerOn(0, 0);
}

void GraphWidget::createNode(const GraphNode& data) {
    // 计算文本大小，动态调整节点大小
    QFont font("Microsoft YaHei", 10, QFont::Bold);
    QFontMetrics fontMetrics(font);
    QRect textBounding = fontMetrics.boundingRect(data.name);
    qreal textWidth = textBounding.width();
    qreal textHeight = textBounding.height();

    // 椭圆大小：文本大小加上内边距，限制在最小和最大范围内
    const qreal minWidth = 80.0;
    const qreal minHeight = 50.0;
    const qreal maxWidth = 200.0;
    const qreal maxHeight = 80.0;
    const qreal paddingX = 40.0;
    const qreal paddingY = 20.0;

    qreal ellipseWidth = qMax(minWidth, qMin(maxWidth, textWidth + paddingX));
    qreal ellipseHeight = qMax(minHeight, qMin(maxHeight, textHeight + paddingY));

    // 创建椭圆节点（中心在本地坐标原点）
    GraphNodeItem* nodeItem = new GraphNodeItem(data.id, this, -ellipseWidth/2, -ellipseHeight/2, ellipseWidth, ellipseHeight);
    
    // 1. 去掉黑黑的边框，换成纯色背景
    nodeItem->setBrush(QBrush(data.color));
    nodeItem->setPen(Qt::NoPen); // 取消黑色硬边框！这很重要！
    
    // 2. 添加高级投影效果 (提升立体感)
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 40)); // 半透明黑
    shadow->setOffset(0, 4);
    nodeItem->setGraphicsEffect(shadow);
    
    nodeItem->setData(0, data.id); // 存储节点ID（兼容性）
    nodeItem->setPos(data.x, data.y); // 设置初始位置

    // 3. 优化文字
    QFont nodeFont("-apple-system, Microsoft YaHei", 10, QFont::Bold);
    
    // 添加节点名称文本，如果太长则用省略号
    QString displayText = data.name;
    QFontMetrics nodeFontMetrics(nodeFont);
    // 检查文本宽度是否超过椭圆宽度减去内边距
    if (nodeFontMetrics.horizontalAdvance(data.name) > ellipseWidth - 20) {
        displayText = nodeFontMetrics.elidedText(data.name, Qt::ElideRight, ellipseWidth - 20);
    }
    QGraphicsTextItem* text = new QGraphicsTextItem(displayText, nodeItem);
    text->setDefaultTextColor(Qt::white);
    text->setFont(nodeFont);
    // 居中文本
    QRectF textRect = text->boundingRect();
    text->setPos(-textRect.width() / 2, -textRect.height() / 2);

    scene_->addItem(nodeItem);

    // 存储节点引用
    GraphNode* nodeData = new GraphNode(data);
    nodeData->x = data.x;
    nodeData->y = data.y;
    nodes_.insert(data.id, nodeData);
    nodeItems_.insert(data.id, nodeItem);
}

void GraphWidget::createEdge(const GraphEdge& data) {
    // 查找源节点和目标节点
    GraphNode* sourceNode = nodes_.value(data.source);
    GraphNode* targetNode = nodes_.value(data.target);

    if (!sourceNode || !targetNode) {
        Logger::warning(QString("Cannot create edge: node not found %1 -> %2").arg(data.source).arg(data.target));
        return;
    }

    // 创建曲线连线（二次贝塞尔曲线）
    QPainterPath path;
    QPointF startPoint(sourceNode->x, sourceNode->y);
    QPointF endPoint(targetNode->x, targetNode->y);

    // 计算控制点：中点垂直偏移，方向由边的哈希决定以减少交叉
    qreal dx = endPoint.x() - startPoint.x();
    qreal dy = endPoint.y() - startPoint.y();
    qreal distance = std::sqrt(dx * dx + dy * dy);
    QPointF midPoint = (startPoint + endPoint) / 2;

    // 垂直单位向量
    QPointF perp(-dy, dx);
    qreal perpLen = std::sqrt(perp.x() * perp.x() + perp.y() * perp.y());
    if (perpLen > 0) {
        perp /= perpLen;
    }

    // 偏移距离：与距离成正比，但有限制
    qreal offset = qMin(50.0, distance * 0.3);
    // 使用源和目标ID的哈希决定偏移方向
    uint hash = qHash(data.source + data.target);
    if (hash % 2 == 0) {
        offset = -offset;
    }

    QPointF ctrlPoint = midPoint + perp * offset;

    path.moveTo(startPoint);
    path.quadTo(ctrlPoint, endPoint);

    QGraphicsPathItem* pathItem = new QGraphicsPathItem(path);

    // 根据置信度设置线条样式
    QPen pen;
    QColor baseColor(180, 180, 180); // 默认灰色
    if (data.confidence > 0.8) {
        baseColor = QColor(103, 194, 58); // 柔和的绿色
        pen = QPen(baseColor, 2.5);
    } else if (data.confidence > 0.5) {
        baseColor = QColor(230, 162, 60); // 柔和的橙色
        pen = QPen(baseColor, 2);
    } else {
        pen = QPen(baseColor, 1.5);
    }
    pen.setJoinStyle(Qt::RoundJoin); // 圆角连接
    pen.setCapStyle(Qt::RoundCap);   // 圆形端点
    pathItem->setPen(pen);
    pathItem->setZValue(-1); // 放在节点下面

    // 添加关系文本（放在曲线中点）
    QGraphicsSimpleTextItem* text = new QGraphicsSimpleTextItem(data.relation);
    text->setFont(QFont("Microsoft YaHei", 9));
    text->setBrush(QBrush(Qt::darkBlue));

    // 计算贝塞尔曲线上的中点（t=0.5）
    QPointF curveMid = (startPoint + endPoint) / 2; // 近似，实际应为二次贝塞尔公式
    // 二次贝塞尔公式：B(t) = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2
    qreal t = 0.5;
    QPointF p0 = startPoint;
    QPointF p1 = ctrlPoint;
    QPointF p2 = endPoint;
    QPointF curveMidPoint = (1-t)*(1-t)*p0 + 2*(1-t)*t*p1 + t*t*p2;

    text->setPos(curveMidPoint.x() - text->boundingRect().width() / 2,
                 curveMidPoint.y() - text->boundingRect().height() / 2);

    scene_->addItem(pathItem);
    scene_->addItem(text);

    // 存储边引用
    GraphEdge* edgeData = new GraphEdge(data);
    edges_.append(edgeData);
    edgeItems_.insert(qMakePair(data.source, data.target), pathItem);
    edgeTextItems_.insert(qMakePair(data.source, data.target), text);
}

void GraphWidget::computeForceDirectedLayout(qreal repulsionK, qreal attractionK, 
                                             qreal damping, int iterations) {
    if (nodes_.isEmpty()) return;

    const qreal kRepulsion = repulsionK; // 斥力系数
    const qreal kAttraction = attractionK; // 引力系数
    const qreal kDamping = damping;    // 阻尼系数
    const qreal minDistance = 100.0;   // 节点间最小距离（椭圆节点宽度80-200）
    const qreal maxDistance = 300.0; // 最大距离

    // 初始化节点速度和位置
    QHash<QString, QPointF> velocities;
    QList<QString> nodeIds = nodes_.keys();
    for (const QString& id : nodeIds) {
        velocities[id] = QPointF(0, 0);
    }

    for (int iter = 0; iter < iterations; ++iter) {
        // 计算节点间的斥力
        for (int i = 0; i < nodeIds.size(); ++i) {
            const QString& id1 = nodeIds[i];
            GraphNode* node1 = nodes_.value(id1);
            if (!node1) continue;

            for (int j = i + 1; j < nodeIds.size(); ++j) {
                const QString& id2 = nodeIds[j];
                GraphNode* node2 = nodes_.value(id2);
                if (!node2) continue;

                qreal dx = node2->x - node1->x;
                qreal dy = node2->y - node1->y;
                qreal distance = std::sqrt(dx * dx + dy * dy);
                if (distance < 1.0) distance = 1.0;

                // 基础斥力公式: F = k_repulsion / distance^2
                qreal force = kRepulsion / (distance * distance);
                
                // 当节点距离太近时，增加额外的强斥力
                if (distance < minDistance) {
                    qreal extraForce = kRepulsion * (minDistance - distance) / minDistance;
                    force += extraForce;
                }
                
                qreal fx = force * dx / distance;
                qreal fy = force * dy / distance;

                velocities[id1] -= QPointF(fx, fy);
                velocities[id2] += QPointF(fx, fy);
            }
        }

        // 计算边的引力
        for (const GraphEdge* edge : edges_) {
            GraphNode* source = nodes_.value(edge->source);
            GraphNode* target = nodes_.value(edge->target);
            if (!source || !target) continue;

            qreal dx = target->x - source->x;
            qreal dy = target->y - source->y;
            qreal distance = std::sqrt(dx * dx + dy * dy);
            if (distance < 1.0) distance = 1.0;

            // 引力公式: F = k_attraction * distance
            qreal force = kAttraction * distance;
            qreal fx = force * dx / distance;
            qreal fy = force * dy / distance;

            velocities[edge->source] += QPointF(fx, fy);
            velocities[edge->target] -= QPointF(fx, fy);
        }

        // 更新位置并应用阻尼
        for (const QString& id : nodeIds) {
            GraphNode* node = nodes_.value(id);
            QPointF& vel = velocities[id];

            // 限制速度
            qreal speed = std::sqrt(vel.x() * vel.x() + vel.y() * vel.y());
            if (speed > 10.0) {
                vel *= 10.0 / speed;
            }

            // 更新位置
            node->x += vel.x();
            node->y += vel.y();

            // 应用阻尼
            vel *= kDamping;
        }
    }

    // 更新图形项位置
    for (const QString& id : nodeIds) {
        GraphNode* node = nodes_.value(id);
        QGraphicsEllipseItem* ellipse = nodeItems_.value(id);
        if (node && ellipse) {
            ellipse->setPos(node->x, node->y);
        }
    }

    // 更新边位置（曲线和文本）
    for (const GraphEdge* edge : edges_) {
        GraphNode* source = nodes_.value(edge->source);
        GraphNode* target = nodes_.value(edge->target);
        QGraphicsPathItem* pathItem = edgeItems_.value(qMakePair(edge->source, edge->target));
        QGraphicsSimpleTextItem* textItem = edgeTextItems_.value(qMakePair(edge->source, edge->target));
        if (!source || !target || !pathItem) continue;

        QPointF startPoint(source->x, source->y);
        QPointF endPoint(target->x, target->y);

        // 计算控制点（与createEdge相同逻辑）
        qreal dx = endPoint.x() - startPoint.x();
        qreal dy = endPoint.y() - startPoint.y();
        qreal distance = std::sqrt(dx * dx + dy * dy);
        QPointF midPoint = (startPoint + endPoint) / 2;

        QPointF perp(-dy, dx);
        qreal perpLen = std::sqrt(perp.x() * perp.x() + perp.y() * perp.y());
        if (perpLen > 0) {
            perp /= perpLen;
        }

        qreal offset = qMin(50.0, distance * 0.3);
        uint hash = qHash(edge->source + edge->target);
        if (hash % 2 == 0) {
            offset = -offset;
        }

        QPointF ctrlPoint = midPoint + perp * offset;

        // 更新路径
        QPainterPath path;
        path.moveTo(startPoint);
        path.quadTo(ctrlPoint, endPoint);
        pathItem->setPath(path);

        // 更新文本位置（如果存在）
        if (textItem) {
            qreal t = 0.5;
            QPointF p0 = startPoint;
            QPointF p1 = ctrlPoint;
            QPointF p2 = endPoint;
            QPointF curveMidPoint = (1-t)*(1-t)*p0 + 2*(1-t)*t*p1 + t*t*p2;
            textItem->setPos(curveMidPoint.x() - textItem->boundingRect().width() / 2,
                             curveMidPoint.y() - textItem->boundingRect().height() / 2);
        }
    }

    fitToView();
}

void GraphWidget::updateLegend() {
    // 清除旧图例
    QList<QGraphicsItem*> items = scene_->items();
    for (QGraphicsItem* item : items) {
        if (item->data(0).toString() == "legend") {
            scene_->removeItem(item);
            delete item;
        }
    }

    if (nodes_.isEmpty()) return;

    // 图例位置：场景右下角（避免和节点重叠）
    QRectF sceneRect = scene_->sceneRect();
    qreal startX = sceneRect.right() - 120;
    qreal startY = sceneRect.bottom() - 140;
    qreal yOffset = 0;

    // 实体类型列表（使用更柔和的颜色）
    QList<QPair<QString, QColor>> legendItems = {
        {tr("部件"), QColor(255, 138, 128)},    // 柔和红
        {tr("故障"), QColor(128, 216, 255)},    // 柔和青
        {tr("工具"), QColor(255, 229, 127)},    // 柔和黄
        {tr("组成"), QColor(179, 136, 255)}     // 柔和紫
    };

    // 图例容器背景
    QGraphicsRectItem* legendBg = new QGraphicsRectItem(startX - 10, startY - 10, 100, 130);
    legendBg->setBrush(QBrush(QColor(255, 255, 255, 200)));
    legendBg->setPen(QPen(QColor(220, 220, 220), 1));
    legendBg->setData(0, "legend");
    scene_->addItem(legendBg);

    // 添加图例标题
    QGraphicsTextItem* title = new QGraphicsTextItem(tr("图例"));
    title->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    title->setDefaultTextColor(QColor(100, 100, 100));
    title->setPos(startX, startY + yOffset);
    title->setData(0, "legend");
    scene_->addItem(title);
    yOffset += 25;

    // 添加每个图例项
    for (const auto& item : legendItems) {
        // 颜色圆角矩形
        QGraphicsRectItem* colorRect = new QGraphicsRectItem(startX, startY + yOffset, 14, 14);
        colorRect->setBrush(QBrush(item.second));
        colorRect->setPen(Qt::NoPen);
        colorRect->setData(0, "legend");
        scene_->addItem(colorRect);

        // 文本标签
        QGraphicsTextItem* label = new QGraphicsTextItem(item.first);
        label->setFont(QFont("Microsoft YaHei", 8));
        label->setDefaultTextColor(QColor(100, 100, 100));
        label->setPos(startX + 20, startY + yOffset - 2);
        label->setData(0, "legend");
        scene_->addItem(label);

        yOffset += 22;
    }
}

void GraphWidget::handleNodePositionChanged(const QString& nodeId, const QPointF& newPos) {
    // 更新节点数据中的位置
    GraphNode* node = nodes_.value(nodeId);
    if (node) {
        node->x = newPos.x();
        node->y = newPos.y();
    }
    // 更新所有连接到该节点的边
    updateConnectedEdges(nodeId);
}

void GraphWidget::updateConnectedEdges(const QString& nodeId) {
    // 遍历所有边，找到连接到该节点的边
    for (GraphEdge* edge : edges_) {
        if (edge->source == nodeId || edge->target == nodeId) {
            // 获取源节点和目标节点
            GraphNode* source = nodes_.value(edge->source);
            GraphNode* target = nodes_.value(edge->target);
            QGraphicsPathItem* pathItem = edgeItems_.value(qMakePair(edge->source, edge->target));
            QGraphicsSimpleTextItem* textItem = edgeTextItems_.value(qMakePair(edge->source, edge->target));

            if (!source || !target || !pathItem) continue;

            QPointF startPoint(source->x, source->y);
            QPointF endPoint(target->x, target->y);

            // 计算控制点（与createEdge相同逻辑）
            qreal dx = endPoint.x() - startPoint.x();
            qreal dy = endPoint.y() - startPoint.y();
            qreal distance = std::sqrt(dx * dx + dy * dy);
            QPointF midPoint = (startPoint + endPoint) / 2;

            QPointF perp(-dy, dx);
            qreal perpLen = std::sqrt(perp.x() * perp.x() + perp.y() * perp.y());
            if (perpLen > 0) {
                perp /= perpLen;
            }

            qreal offset = qMin(50.0, distance * 0.3);
            uint hash = qHash(edge->source + edge->target);
            if (hash % 2 == 0) {
                offset = -offset;
            }

            QPointF ctrlPoint = midPoint + perp * offset;

            // 更新路径
            QPainterPath path;
            path.moveTo(startPoint);
            path.quadTo(ctrlPoint, endPoint);
            pathItem->setPath(path);

            // 更新文本位置（如果存在）
            if (textItem) {
                qreal t = 0.5;
                QPointF p0 = startPoint;
                QPointF p1 = ctrlPoint;
                QPointF p2 = endPoint;
                QPointF curveMidPoint = (1-t)*(1-t)*p0 + 2*(1-t)*t*p1 + t*t*p2;
                textItem->setPos(curveMidPoint.x() - textItem->boundingRect().width() / 2,
                                 curveMidPoint.y() - textItem->boundingRect().height() / 2);
            }
        }
    }
}

} // namespace optikg