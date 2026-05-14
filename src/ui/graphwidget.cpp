#include "graphwidget.h"

#include <QtGlobal>
#include <QPointF>
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
#include <qrandom.h>
#include <memory>

#include "optikg/datamodel.h"

namespace optikg {
    // 计算边的智能偏移方向
    // 根据边的角度将边分类到区间，相同区间的边使用一致偏移方向减少交叉
    static qreal calculateEdgeOffsetDirection(const QPointF &startPoint, const QPointF &endPoint,
                                              const QString &sourceId, const QString &targetId) {
        qreal dx = endPoint.x() - startPoint.x();
        qreal dy = endPoint.y() - startPoint.y();
        qreal distance = std::sqrt(dx * dx + dy * dy);

        if (distance < 1.0) return 1.0; // 默认

        // 计算角度（弧度）
        qreal angle = std::atan2(dy, dx);

        // 规范化角度到 [0, 2π)
        if (angle < 0) angle += 2 * M_PI;

        // 将角度分为8个区间（每45度一个区间）
        // 这样平行边会被分到相同或相邻区间，使用一致偏移方向
        const int numIntervals = 8;
        int interval = static_cast<int>(angle / (2 * M_PI / numIntervals)) % numIntervals;

        // 使用区间索引和节点ID哈希组合来决定方向
        // 相同区间的边将有一致的偏移方向
        uint hash = qHash(QString("%1_%2_%3").arg(sourceId).arg(targetId).arg(interval));

        // 根据区间索引决定基本方向模式
        // 偶数区间向上/向右偏移，奇数区间向下/向左偏移
        // 结合哈希值微调，确保不同边对有不同的细微差异
        qreal baseDirection = (interval % 2 == 0) ? 1.0 : -1.0;

        // 添加细微变化避免所有边完全一致（使用哈希值的小数部分）
        qreal hashFactor = (hash % 100) / 100.0; // 0.0到0.99
        qreal variation = 0.2 * (hashFactor - 0.5); // -0.1到0.1的变化

        return baseDirection * (1.0 + variation);
    }

    // GraphNodeItem 实现
    GraphNodeItem::GraphNodeItem(const QString &nodeId, GraphWidget *graphWidget,
                                 qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent)
        : QGraphicsEllipseItem(x, y, width, height, parent)
          , nodeId_(nodeId)
          , graphWidget_(graphWidget) {
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    }

    QVariant GraphNodeItem::itemChange(GraphicsItemChange change, const QVariant &value) {
        if (change == QGraphicsItem::ItemPositionHasChanged && graphWidget_) {
            graphWidget_->handleNodePositionChanged(nodeId_, this->pos());
        }
        return QGraphicsEllipseItem::itemChange(change, value);
    }

    // 颜色映射 - 马卡龙色系（柔和版）
    const QHash<QString, QColor> GraphWidget::TYPE_COLORS = {
        {"部件", QColor(255, 138, 128)}, // 柔和红 #FF8A80
        {"故障", QColor(128, 216, 255)}, // 柔和青 #80D8FF
        {"工具", QColor(255, 229, 127)}, // 柔和黄 #FFE57F
        {"组成", QColor(179, 136, 255)} // 柔和紫 #B388FF
    };

    GraphWidget::GraphWidget(QWidget *parent)
        : QGraphicsView(parent)
          , scene_(nullptr)
          , dragging_(false)
          , lastMousePos_(0, 0)
          , scaleFactor_(1.0) {
        Q_ASSERT_X(parent != nullptr || true, "GraphWidget::GraphWidget", "Parent can be null");
        setupScene();
        setRenderHint(QPainter::Antialiasing);
        setDragMode(QGraphicsView::NoDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setBackgroundBrush(QBrush(QColor(245, 245, 245)));
    }

    // src/ui/graphwidget.cpp

    void GraphWidget::setupScene() {
        scene_ = new QGraphicsScene(this);
        // 不要在这里写死具体的数字，或者给一个极大的范围
        scene_->setSceneRect(-5000, -5000, 10000, 10000);
        setScene(scene_);
        showEmptyState();
    }

    void GraphWidget::showEmptyState() {
        if (emptyStateGroup_) {
            // 如果组存在但不在场景中，重新添加
            if (emptyStateGroup_->scene() != scene_) {
                scene_->addItem(emptyStateGroup_);
            }
            emptyStateGroup_->setVisible(true);
            // 空状态显示时也应该允许拖动（虽然内容为空）
            emptyStateGroup_->setEnabled(true);
            emptyStateGroup_->setAcceptHoverEvents(false);
            emptyStateGroup_->setAcceptedMouseButtons(Qt::NoButton);
            return;
        }

        emptyStateGroup_ = new QGraphicsItemGroup();
        emptyStateGroup_->setZValue(-100); // 置于最底层
        scene_->addItem(emptyStateGroup_);

        // 场景已经是父对象（通过scene_->addItem），不需要额外设置

        // 1. 简约的网络图标（使用组合图形，不用path避免线条显示问题）
        // 中心圆 - 使用QPointer自动管理
        auto *centerCircle = new QGraphicsEllipseItem(-12, -12, 24, 24, emptyStateGroup_);
        centerCircle->setBrush(QBrush(QColor(200, 200, 200, 60)));
        centerCircle->setPen(Qt::NoPen);
        centerCircle->setPos(0, -30);
        centerCircle->setAcceptedMouseButtons(Qt::NoButton);

        // 周围小圆
        auto *smallCircle1 = new QGraphicsEllipseItem(-8, -8, 16, 16, emptyStateGroup_);
        smallCircle1->setBrush(QBrush(QColor(200, 200, 200, 40)));
        smallCircle1->setPen(Qt::NoPen);
        smallCircle1->setPos(-40, -60);
        smallCircle1->setAcceptedMouseButtons(Qt::NoButton);

        auto *smallCircle2 = new QGraphicsEllipseItem(-8, -8, 16, 16, emptyStateGroup_);
        smallCircle2->setBrush(QBrush(QColor(200, 200, 200, 40)));
        smallCircle2->setPen(Qt::NoPen);
        smallCircle2->setPos(40, -60);
        smallCircle2->setAcceptedMouseButtons(Qt::NoButton);

        auto *smallCircle3 = new QGraphicsEllipseItem(-8, -8, 16, 16, emptyStateGroup_);
        smallCircle3->setBrush(QBrush(QColor(200, 200, 200, 40)));
        smallCircle3->setPen(Qt::NoPen);
        smallCircle3->setPos(-40, 0);
        smallCircle3->setAcceptedMouseButtons(Qt::NoButton);

        auto *smallCircle4 = new QGraphicsEllipseItem(-8, -8, 16, 16, emptyStateGroup_);
        smallCircle4->setBrush(QBrush(QColor(200, 200, 200, 40)));
        smallCircle4->setPen(Qt::NoPen);
        smallCircle4->setPos(40, 0);
        smallCircle4->setAcceptedMouseButtons(Qt::NoButton);

        // 2. 主提示文字
        auto *mainText = new QGraphicsTextItem(tr("暂无知识图谱数据"), emptyStateGroup_);
        mainText->setDefaultTextColor(QColor(150, 150, 150));
        mainText->setFont(QFont("Microsoft YaHei", 14, QFont::Medium));
        QRectF mainRect = mainText->boundingRect();
        mainText->setPos(-mainRect.width() / 2, 30);
        mainText->setAcceptedMouseButtons(Qt::NoButton);

        // 3. 副提示文字
        auto *subText = new QGraphicsTextItem(tr("在左侧输入文本，点击「开始抽取」生成知识图谱"), emptyStateGroup_);
        subText->setDefaultTextColor(QColor(180, 180, 180));
        subText->setFont(QFont("Microsoft YaHei", 11));
        QRectF subRect = subText->boundingRect();
        subText->setPos(-subRect.width() / 2, 60);
        subText->setAcceptedMouseButtons(Qt::NoButton);

        hasData_ = false;
    }

    void GraphWidget::hideEmptyState() {
        if (emptyStateGroup_) {
            emptyStateGroup_->setVisible(false);
            // 禁用空状态组的鼠标事件，确保不会拦截空白处的拖动
            emptyStateGroup_->setEnabled(false);
            emptyStateGroup_->setAcceptHoverEvents(false);
            emptyStateGroup_->setAcceptedMouseButtons(Qt::NoButton);
        }
        hasData_ = true;
    }

    qreal GraphWidget::calculateOptimalRadius(int nodeCount) {
        // 缩小初始布局范围，给力导向算法留出“舒展”的空间
        if (nodeCount <= 1) return 0;
        if (nodeCount <= 5) return 150;
        if (nodeCount <= 10) return 250;
        if (nodeCount <= 20) return 350;
        return qMin(600.0, 200.0 + nodeCount * 10.0);
    }

    void GraphWidget::setGraphData(const QList<GraphNode> &nodes, const QList<GraphEdge> &edges) {
        // 如果没有数据，显示空状态
        if (nodes.isEmpty()) {
            clearGraph();
            return;
        }

        // 有数据，隐藏空状态
        hideEmptyState();

        // 清理旧数据但保留空状态组
        clearGraphData();

        // 清除场景中的图形项，但保留空状态组及其子项
        QList<QGraphicsItem *> items = scene_->items();
        for (QGraphicsItem *item: items) {
            if (item == emptyStateGroup_ || item->parentItem() == emptyStateGroup_) {
                continue;
            }
            // QGraphicsItem 有父子关系，删除父项会自动删除子项
            scene_->removeItem(item);
            delete item;
        }
        nodeItems_.clear();
        edgeItems_.clear();
        edgeTextItems_.clear();

        // 1. 创建节点
        for (const auto &node: nodes) {
            createNode(node);
        }

        // 2. 计算圆形布局并更新节点位置
        const int nodeCount = nodes_.size();
        const qreal radius = calculateOptimalRadius(nodeCount);
        QList<QString> nodeIds = nodes_.keys();
        for (const QString &nodeId: nodeIds) {
            GraphNode *nodeData = nodes_.value(nodeId);
            QGraphicsEllipseItem *ellipse = nodeItems_.value(nodeId);
            if (nodeData && ellipse) {
                // 在画布范围内随机撒点，范围从 -radius 到 +radius
                qreal x = ((rand() % 2000) / 1000.0 - 1.0) * radius;
                qreal y = ((rand() % 2000) / 1000.0 - 1.0) * radius;
                nodeData->x = x;
                nodeData->y = y;
                ellipse->setPos(x, y);
            }
        }

        // 3. 创建边
        for (const auto &edge: edges) {
            createEdge(edge);
        }

        // 4. 对所有节点应用力导向布局，确保节点间有足够距离但不过度分散
        if (nodeCount <= 3) {
            // 少量节点：强斥力，避免重叠
            computeForceDirectedLayout(200, 0.01, 0.8, 80);
        } else if (nodeCount <= 6) {
            // 小量节点：中等斥力
            computeForceDirectedLayout(180, 0.015, 0.82, 70);
        } else if (nodeCount <= 10) {
            // 中等数量：标准斥力
            computeForceDirectedLayout(150, 0.02, 0.85, 60);
        } else if (nodeCount <= 30) {
            // 中等大量节点：较强斥力，防止过度拥挤
            computeForceDirectedLayout(120, 0.03, 0.88, 80);
        } else {
            // 大量节点：强斥力，更多迭代
            computeForceDirectedLayout(100, 0.035, 0.9, 120);
        }

        // 更新图例
        updateLegend();
        fitToView();
    }

    // 辅助函数：清理图数据（节点和边的数据对象）
    void GraphWidget::clearGraphData() {
        // 使用QPointer安全删除
        for (GraphEdge *edge: edges_) {
            if (edge) {
                delete edge;
            }
        }
        edges_.clear();

        for (GraphNode *node: nodes_) {
            if (node) {
                delete node;
            }
        }
        nodes_.clear();
    }

    void GraphWidget::clearGraph() {
        // 清理数据对象
        clearGraphData();

        // 清除场景中的图形项，但保留空状态组及其子项
        QList<QGraphicsItem *> items = scene_->items();
        for (QGraphicsItem *item: items) {
            if (item == emptyStateGroup_ || item->parentItem() == emptyStateGroup_) {
                continue;
            }
            scene_->removeItem(item);
            // 由于 QGraphicsItem 有父子关系，删除父项会自动删除子项
            delete item;
        }
        nodeItems_.clear();
        edgeItems_.clear();
        edgeTextItems_.clear();

        // 显示空状态
        showEmptyState();
        hasData_ = false;
        fitToView();
    }

    void GraphWidget::highlightNode(const QString &nodeId) {
        // 重置所有节点颜色
        for (auto it = nodeItems_.begin(); it != nodeItems_.end(); ++it) {
            QGraphicsEllipseItem *ellipse = it.value();
            GraphNode *nodeData = nodes_.value(it.key());
            if (nodeData) {
                ellipse->setBrush(QBrush(nodeData->color));
                ellipse->setPen(Qt::NoPen); // 无边框
            }
        }

        // 高亮指定节点
        QGraphicsEllipseItem *ellipse = nodeItems_.value(nodeId);
        if (ellipse) {
            ellipse->setBrush(QBrush(QColor(255, 200, 100))); // 柔和黄色高亮
            ellipse->setPen(QPen(QColor(255, 165, 0), 2)); // 橙色边框
        }
    }

    void GraphWidget::highlightPath(const QString &sourceId, const QString &targetId) {
        // 重置所有高亮
        for (auto it = nodeItems_.begin(); it != nodeItems_.end(); ++it) {
            QGraphicsEllipseItem *ellipse = it.value();
            GraphNode *nodeData = nodes_.value(it.key());
            if (nodeData) {
                ellipse->setBrush(QBrush(nodeData->color));
                ellipse->setPen(Qt::NoPen); // 无边框
            }
        }
        for (auto it = edgeItems_.begin(); it != edgeItems_.end(); ++it) {
            QGraphicsPathItem *pathItem = it.value();
            pathItem->setPen(QPen(QColor(200, 200, 200), 1, Qt::SolidLine));
        }

        // 高亮源节点和目标节点
        QGraphicsEllipseItem *sourceEllipse = nodeItems_.value(sourceId);
        QGraphicsEllipseItem *targetEllipse = nodeItems_.value(targetId);
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
        QGraphicsPathItem *pathItem = edgeItems_.value(key);
        if (pathItem) {
            pathItem->setPen(QPen(QColor(0, 0, 255), 3, Qt::SolidLine)); // 蓝色粗线
        }
    }

    void GraphWidget::zoomIn() {
        scaleFactor_ *= 1.2;
        scale(1.2, 1.2); // 使用 scale() 进行相对缩放，保留平移变换
    }

    void GraphWidget::zoomOut() {
        scaleFactor_ /= 1.2;
        scale(1.0 / 1.2, 1.0 / 1.2); // 使用 scale() 进行相对缩放，保留平移变换
    }

    void GraphWidget::resetZoom() {
        scaleFactor_ = 1.0;
        setTransform(QTransform());
    }

    void GraphWidget::fitToView() {
        if (!hasData_ || nodes_.isEmpty()) {
            // 没有数据时，显示空状态在视图中心
            fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
            scaleFactor_ = 1.0;
            return;
        }

        // 计算所有节点和边的包围矩形
        QRectF itemsRect;
        for (auto it = nodeItems_.begin(); it != nodeItems_.end(); ++it) {
            if (QGraphicsEllipseItem *ellipse = it.value()) {
                itemsRect = itemsRect.united(ellipse->sceneBoundingRect());
            }
        }
        // 也考虑边的包围盒
        for (auto it = edgeItems_.begin(); it != edgeItems_.end(); ++it) {
            if (QGraphicsPathItem *path = it.value()) {
                itemsRect = itemsRect.united(path->sceneBoundingRect());
            }
        }

        if (!itemsRect.isEmpty()) {
            // 添加一些边距，让图形不贴边
            itemsRect.adjust(-50, -50, 50, 50);
            fitInView(itemsRect, Qt::KeepAspectRatio);
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

    // src/ui/graphwidget.cpp

    void GraphWidget::mousePressEvent(QMouseEvent *event) {
        if (event->button() == Qt::LeftButton) {
            QGraphicsItem* item = itemAt(event->pos());

            // --- 修复逻辑开始 ---
            bool hitMovableItem = false;
            QGraphicsItem* checkItem = item;

            // 向上循环查找：如果你点击的是文字，它会检查文字的父项（即节点）
            while (checkItem) {
                if (checkItem->flags() & QGraphicsItem::ItemIsMovable) {
                    hitMovableItem = true;
                    break;
                }
                checkItem = checkItem->parentItem(); // 往父级找
            }

            if (!hitMovableItem) {
                // 如果点的是空白处，或者点的是连线/图例等不可移动物体
                dragging_ = true;
                lastMousePos_ = event->pos();
                setCursor(Qt::ClosedHandCursor);
                event->accept();
                return; // 拦截，不让基类处理背景
            }
            // --- 修复逻辑结束 ---

            // 如果点到了可移动物体（节点或其子项文字），必须交给基类处理拖拽
            setDragMode(QGraphicsView::NoDrag);
        }
        QGraphicsView::mousePressEvent(event);
    }

    void GraphWidget::mouseMoveEvent(QMouseEvent *event) {
        if (dragging_) {
            // 计算偏移量
            QPoint delta = event->pos() - lastMousePos_;
            lastMousePos_ = event->pos();

            // 直接操作滚动条实现平移
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
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
        // 保持 NoDrag 模式，使用自定义拖动逻辑
        setDragMode(QGraphicsView::NoDrag);
    }

    void GraphWidget::resizeEvent(QResizeEvent *event) {
        QGraphicsView::resizeEvent(event);
        // 保持场景居中
        centerOn(0, 0);
    }

    void GraphWidget::createNode(const GraphNode &data) {
        Q_ASSERT_X(!data.id.isEmpty(), "GraphWidget::createNode", "Node ID is empty");
        Q_ASSERT_X(!data.name.isEmpty(), "GraphWidget::createNode", "Node name is empty");

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
        GraphNodeItem *nodeItem = new GraphNodeItem(data.id, this, -ellipseWidth / 2, -ellipseHeight / 2, ellipseWidth,
                                                    ellipseHeight);

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
        QGraphicsTextItem *text = new QGraphicsTextItem(displayText, nodeItem);
        text->setDefaultTextColor(Qt::white);
        text->setFont(nodeFont);
        // 居中文本
        QRectF textRect = text->boundingRect();
        text->setPos(-textRect.width() / 2, -textRect.height() / 2);

        scene_->addItem(nodeItem);

        // 存储节点引用
        GraphNode *nodeData = new GraphNode(data);
        nodeData->x = data.x;
        nodeData->y = data.y;
        nodes_.insert(data.id, nodeData);
        nodeItems_.insert(data.id, nodeItem);
    }

    void GraphWidget::createEdge(const GraphEdge &data) {
        Q_ASSERT_X(!data.source.isEmpty(), "GraphWidget::createEdge", "Source ID is empty");
        Q_ASSERT_X(!data.target.isEmpty(), "GraphWidget::createEdge", "Target ID is empty");

        // 查找源节点和目标节点
        GraphNode *sourceNode = nodes_.value(data.source);
        GraphNode *targetNode = nodes_.value(data.target);

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
        // 短边使用较大比例偏移，长边使用较小比例，确保最小20，最大60
        qreal offset = qMin(60.0, qMax(20.0, distance * 0.25));
        // 使用智能偏移方向计算，减少交叉
        qreal direction = calculateEdgeOffsetDirection(startPoint, endPoint, data.source, data.target);
        offset *= direction;

        QPointF ctrlPoint = midPoint + perp * offset;

        path.moveTo(startPoint);
        path.quadTo(ctrlPoint, endPoint);

        QGraphicsPathItem *pathItem = new QGraphicsPathItem(path);
        pathItem->setAcceptedMouseButtons(Qt::NoButton); // 不接收鼠标事件，允许点击边时拖动视图

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
        pen.setCapStyle(Qt::RoundCap); // 圆形端点
        pathItem->setPen(pen);
        pathItem->setZValue(-1); // 放在节点下面

        // 添加关系文本（放在曲线中点）
        QGraphicsSimpleTextItem *text = new QGraphicsSimpleTextItem(data.relation);
        text->setFont(QFont("Microsoft YaHei", 9));
        text->setBrush(QBrush(Qt::darkBlue));
        text->setAcceptedMouseButtons(Qt::NoButton); // 不接收鼠标事件，允许点击文本时拖动视图

        // 计算贝塞尔曲线上的中点（t=0.5）
        QPointF curveMid = (startPoint + endPoint) / 2; // 近似，实际应为二次贝塞尔公式
        // 二次贝塞尔公式：B(t) = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2
        qreal t = 0.5;
        QPointF p0 = startPoint;
        QPointF p1 = ctrlPoint;
        QPointF p2 = endPoint;
        QPointF curveMidPoint = (1 - t) * (1 - t) * p0 + 2 * (1 - t) * t * p1 + t * t * p2;

        text->setPos(curveMidPoint.x() - text->boundingRect().width() / 2,
                     curveMidPoint.y() - text->boundingRect().height() / 2);

        scene_->addItem(pathItem);
        scene_->addItem(text);

        // 存储边引用 - 使用智能指针风格管理（通过父对象关系）
        auto *edgeData = new GraphEdge(data);
        edges_.append(edgeData);
        edgeItems_.insert(qMakePair(data.source, data.target), pathItem);
        edgeTextItems_.insert(qMakePair(data.source, data.target), text);

        // pathItem已经添加到场景中，场景会管理其生命周期
    }

    void GraphWidget::computeForceDirectedLayout(qreal repulsionK, qreal attractionK,
                                                 qreal damping, int iterations) {
        if (nodes_.isEmpty()) return;

        // ==========================================
        // 1. 专为“极致紧凑”优化的参数
        // ==========================================
        qreal k = 140.0; // 连线的理想长度（稍微缩短，让关系更紧密）

        // 【关键修改1】斥力场急剧缩小！节点距离超过 250 像素，彻底失去斥力
        // 这样重力就能毫无阻力地把它们拉到一起，直到碰到 250 像素的“防护罩”
        qreal maxRepulsionDist = 250.0;

        qreal temperature = 300.0;

        QHash<QString, QPointF> displacements;
        QList<QString> nodeIds = nodes_.keys();

        for (int iter = 0; iter < iterations; ++iter) {
            for (const QString &id: nodeIds) {
                displacements[id] = QPointF(0, 0);
            }

            // --- A. 短程斥力 (防止重叠) ---
            for (int i = 0; i < nodeIds.size(); ++i) {
                const QString &v_id = nodeIds[i];
                GraphNode *v = nodes_.value(v_id);

                for (int j = i + 1; j < nodeIds.size(); ++j) {
                    const QString &u_id = nodeIds[j];
                    GraphNode *u = nodes_.value(u_id);

                    qreal dx = v->x - u->x;
                    qreal dy = v->y - u->y;
                    qreal distSq = dx * dx + dy * dy;

                    if (distSq < 0.1) {
                        dx = (rand() % 100) / 100.0 - 0.5;
                        dy = (rand() % 100) / 100.0 - 0.5;
                        distSq = dx * dx + dy * dy + 0.1;
                    }

                    qreal dist = std::sqrt(distSq);

                    // 只有进入 250 像素以内，才开始排斥
                    if (dist < maxRepulsionDist) {
                        qreal force = (k * k) / dist;
                        qreal fx = force * (dx / dist);
                        qreal fy = force * (dy / dist);

                        displacements[v_id] += QPointF(fx, fy);
                        displacements[u_id] -= QPointF(fx, fy);
                    }
                }
            }

            // --- B. 强化连线引力 (保持小团体不散) ---
            for (const GraphEdge *edge: edges_) {
                const QString &v_id = edge->source;
                const QString &u_id = edge->target;
                GraphNode *v = nodes_.value(v_id);
                GraphNode *u = nodes_.value(u_id);
                if (!v || !u) continue;

                qreal dx = v->x - u->x;
                qreal dy = v->y - u->y;
                qreal dist = std::sqrt(dx * dx + dy * dy);
                if (dist < 0.1) dist = 0.1;

                qreal force = (dist * dist) / k;
                // 【关键修改2】内部连线引力乘以 1.5，防止被全局重力扯散
                force *= 1.5;

                qreal fx = force * (dx / dist);
                qreal fy = force * (dy / dist);

                displacements[v_id] -= QPointF(fx, fy);
                displacements[u_id] += QPointF(fx, fy);
            }

            // --- C. 全局黑洞重力 (无条件拉拢) ---
            for (const QString &id: nodeIds) {
                GraphNode *node = nodes_.value(id);
                qreal distToCenter = std::sqrt(node->x * node->x + node->y * node->y);

                // 【关键修改3】取消距离判断，所有节点无条件往中心 0,0 坠落
                qreal gravityForce = 0.08 * distToCenter;
                displacements[id] -= QPointF(gravityForce * (node->x / (distToCenter + 0.1)),
                                             gravityForce * (node->y / (distToCenter + 0.1)));
            }

            // --- D. 应用位移 ---
            for (const QString &id: nodeIds) {
                GraphNode *node = nodes_.value(id);
                QPointF &disp = displacements[id];

                qreal dispLen = std::sqrt(disp.x() * disp.x() + disp.y() * disp.y());
                if (dispLen > 0.1) {
                    qreal moveDist = qMin(dispLen, temperature);
                    node->x += (disp.x() / dispLen) * moveDist;
                    node->y += (disp.y() / dispLen) * moveDist;
                }
            }

            temperature *= damping;
        }

        // ==========================================
        // 3. 更新 UI (直线渲染)
        // ==========================================
        for (const QString &id: nodeIds) {
            GraphNode *node = nodes_.value(id);
            QGraphicsEllipseItem *ellipse = nodeItems_.value(id);
            if (node && ellipse) ellipse->setPos(node->x, node->y);
        }

        for (const GraphEdge *edge: edges_) {
            GraphNode *source = nodes_.value(edge->source);
            GraphNode *target = nodes_.value(edge->target);
            QGraphicsPathItem *pathItem = edgeItems_.value(qMakePair(edge->source, edge->target));
            QGraphicsSimpleTextItem *textItem = edgeTextItems_.value(qMakePair(edge->source, edge->target));
            if (!source || !target || !pathItem) continue;

            QPointF startPoint(source->x, source->y);
            QPointF endPoint(target->x, target->y);
            QPointF midPoint = (startPoint + endPoint) / 2;

            QPainterPath path;
            path.moveTo(startPoint);
            path.lineTo(endPoint);
            pathItem->setPath(path);

            if (textItem) {
                textItem->setPos(midPoint.x() - textItem->boundingRect().width() / 2,
                                 midPoint.y() - textItem->boundingRect().height() / 2);
            }
        }

        fitToView();
    }

    void GraphWidget::updateLegend() {
        // 清除旧图例
        QList<QGraphicsItem *> items = scene_->items();
        for (QGraphicsItem *item: items) {
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

        // 实体类型列表（使用与节点一致的颜色）
        QList<QPair<QString, QColor> > legendItems;
        QList<EntityType> entityTypes = {
            EntityType::Component,
            EntityType::Fault,
            EntityType::Tool,
            EntityType::Composition
        };
        for (EntityType type: entityTypes) {
            Entity entity;
            entity.type = type;
            legendItems.append({Entity::typeToString(type), entity.typeToColor()});
        }

        // 图例容器背景
        QGraphicsRectItem *legendBg = new QGraphicsRectItem(startX - 10, startY - 10, 100, 130);
        legendBg->setBrush(QBrush(QColor(255, 255, 255, 200)));
        legendBg->setPen(QPen(QColor(220, 220, 220), 1));
        legendBg->setData(0, "legend");
        legendBg->setAcceptedMouseButtons(Qt::NoButton); // 不接收鼠标事件
        scene_->addItem(legendBg);

        // 添加图例标题
        QGraphicsTextItem *title = new QGraphicsTextItem(tr("图例"));
        title->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        title->setDefaultTextColor(QColor(100, 100, 100));
        title->setPos(startX, startY + yOffset);
        title->setData(0, "legend");
        title->setAcceptedMouseButtons(Qt::NoButton); // 不接收鼠标事件
        scene_->addItem(title);
        yOffset += 25;

        // 添加每个图例项
        for (const auto &item: legendItems) {
            // 颜色圆角矩形
            QGraphicsRectItem *colorRect = new QGraphicsRectItem(startX, startY + yOffset, 14, 14);
            colorRect->setBrush(QBrush(item.second));
            colorRect->setPen(Qt::NoPen);
            colorRect->setData(0, "legend");
            colorRect->setAcceptedMouseButtons(Qt::NoButton); // 不接收鼠标事件
            scene_->addItem(colorRect);

            // 文本标签
            QGraphicsTextItem *label = new QGraphicsTextItem(item.first);
            label->setFont(QFont("Microsoft YaHei", 8));
            label->setDefaultTextColor(QColor(100, 100, 100));
            label->setPos(startX + 20, startY + yOffset - 2);
            label->setData(0, "legend");
            label->setAcceptedMouseButtons(Qt::NoButton); // 不接收鼠标事件
            scene_->addItem(label);

            yOffset += 22;
        }
    }

    void GraphWidget::handleNodePositionChanged(const QString &nodeId, const QPointF &newPos) {
        // 更新节点数据中的位置
        GraphNode *node = nodes_.value(nodeId);
        if (node) {
            node->x = newPos.x();
            node->y = newPos.y();
        }
        // 更新所有连接到该节点的边
        updateConnectedEdges(nodeId);
    }

    void GraphWidget::updateConnectedEdges(const QString &nodeId) {
        // 遍历所有边，找到连接到该节点的边
        for (GraphEdge *edge: edges_) {
            if (edge->source == nodeId || edge->target == nodeId) {
                // 获取源节点和目标节点
                GraphNode *source = nodes_.value(edge->source);
                GraphNode *target = nodes_.value(edge->target);
                QGraphicsPathItem *pathItem = edgeItems_.value(qMakePair(edge->source, edge->target));
                QGraphicsSimpleTextItem *textItem = edgeTextItems_.value(qMakePair(edge->source, edge->target));

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

                qreal offset = qMin(60.0, qMax(20.0, distance * 0.25));
                // 使用智能偏移方向计算，保持一致性
                qreal direction = calculateEdgeOffsetDirection(startPoint, endPoint, edge->source, edge->target);
                offset *= direction;

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
                    QPointF curveMidPoint = (1 - t) * (1 - t) * p0 + 2 * (1 - t) * t * p1 + t * t * p2;
                    textItem->setPos(curveMidPoint.x() - textItem->boundingRect().width() / 2,
                                     curveMidPoint.y() - textItem->boundingRect().height() / 2);
                }
            }
        }
    }
} // namespace optikg
