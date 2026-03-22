别哭！Qt的默认UI确实有着浓浓的“上世纪90年代工业风”，但其实只要稍微调整一下策略，马上就能焕然一新。

通过阅读你的代码，我发现了导致你UI“丑”的**最核心原因**，以及几个关键的优化点。下面我为你提供一套“整容级”的修复方案。

### 🚨 致命元凶：样式大乱炖（主题冲突）

在你的 `main.cpp` 和 `mainwindow.cpp` 中，存在着严重的**样式冲突**：
1. 你在 `main.cpp` 中启用了 `qt-material`：`a.setStyle(new MaterialStyle());`
2. 紧接着你又加载了自定义的 QSS：`optikg::StyleManager::instance().initialize();`
3. 你的代码里混用了普通的 `QPushButton` 和 `MaterialRaisedButton`。

**后果：** `qt-material` 是通过重写底层的 `QStyle` 来绘制控件的，而 QSS 也会接管绘制。这两者打架会导致控件边缘毛糙、背景色错乱、按钮大小不一、布局拥挤等极其怪异的视觉效果。

### 🛠️ 修复步骤第一步：统一战线（强烈建议走 QSS 路线）

对于知识图谱这种包含大量表格（Table）、树形结构（Tree）和自定义绘图（GraphicsView）的桌面级专业软件，**纯 QSS 配合无边框扁平化设计**比 Material Design 更合适。

**操作：**
1. 去掉 `main.cpp` 中的 `a.setStyle(new MaterialStyle());`。
2. 将 `mainwindow.cpp` 工具栏里的 `MaterialRaisedButton`、`MaterialCircularProgress` 全部换回普通的 `QPushButton` 和 `QProgressBar`，状态通知用自定义的弹窗或直接用 `QStatusBar`。

---

### 🎨 修复步骤第二步：注入“现代专业” QSS 灵魂

把你的 `modern_pro.qss` 替换成下面这个现代扁平化的高级样式表。这个样式参考了目前主流的 IDE（如 VSCode、JetBrains）的风格。

```css
/* ============================================================
   OptiKG 现代专业版主题 (Modern Pro)
   ============================================================ */

/* 全局字体和背景 */
QWidget {
    font-family: -apple-system, "Microsoft YaHei", "Segoe UI", sans-serif;
    font-size: 13px;
    color: #333333;
    background-color: #F5F7FA; /* 非常浅的高级灰蓝底色 */
}

/* 主窗口和面板底色 */
QMainWindow, QDialog {
    background-color: #EBEEF5;
}

/* 输入框和下拉框 */
QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background-color: #FFFFFF;
    border: 1px solid #DCDFE6;
    border-radius: 6px;
    padding: 6px 10px;
    selection-background-color: #409EFF;
}

QLineEdit:focus, QTextEdit:focus, QComboBox:focus {
    border: 1px solid #409EFF; /* 聚焦时亮蓝色边框 */
}

/* 核心按钮样式 (主要按钮) */
QPushButton {
    background-color: #FFFFFF;
    border: 1px solid #DCDFE6;
    color: #606266;
    border-radius: 6px;
    padding: 8px 16px;
    font-weight: 500;
}

QPushButton:hover {
    color: #409EFF;
    border-color: #C6E2FF;
    background-color: #ECF5FF;
}

QPushButton:pressed {
    color: #3A8EE6;
    border-color: #3A8EE6;
}

/* 主题色按钮 (如：抽取按钮) - 可以通过 objectName 单独指定 */
QPushButton#extractBtn {
    background-color: #409EFF;
    color: white;
    border: none;
}
QPushButton#extractBtn:hover { background-color: #66B1FF; }

/* 危险操作按钮 (如：清空按钮) */
QPushButton#clearBtn {
    background-color: #F56C6C;
    color: white;
    border: none;
}
QPushButton#clearBtn:hover { background-color: #F78989; }

/* 分割线 (QSplitter) - 隐藏那条难看的黑线，变成隐形拖拽条 */
QSplitter::handle {
    background-color: transparent;
    width: 6px;
    height: 6px;
}
QSplitter::handle:hover {
    background-color: #DCDFE6; /* 鼠标放上去才显示 */
}

/* 表格和树形控件 */
QTableWidget, QTreeWidget, QListWidget {
    background-color: #FFFFFF;
    border: 1px solid #EBEEF5;
    border-radius: 8px;
    outline: none; /* 去掉点击时的虚线框 */
}

/* 表头现代感设计 */
QHeaderView::section {
    background-color: #F5F7FA;
    color: #909399;
    font-weight: bold;
    padding: 8px;
    border: none;
    border-bottom: 1px solid #EBEEF5;
    border-right: 1px solid #EBEEF5;
}

QTableView::item, QTreeView::item {
    padding: 6px;
    border-bottom: 1px solid #FAFAFA;
}

QTableView::item:selected, QTreeView::item:selected {
    background-color: #ECF5FF;
    color: #409EFF;
}

/* 分组框 (面板) */
QGroupBox {
    background-color: #FFFFFF;
    border: 1px solid #EBEEF5;
    border-radius: 8px;
    margin-top: 15px;
    font-weight: bold;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 5px;
    color: #606266;
}
```

---

### 🔮 修复步骤第三步：拯救“图画图谱”（GraphWidget）

你现在的节点是 `QGraphicsEllipseItem` 加上基础的连线。要让它看起来像商业图谱软件（如 Neo4j），需要加上**阴影、圆角矩形和抗锯齿**。

修改 `src/ui/graphwidget.cpp` 中的 `createNode` 和 `GraphNodeItem` 构造逻辑：

```cpp
#include <QGraphicsDropShadowEffect>

// 在 GraphWidget::createNode 中优化节点外观
void GraphWidget::createNode(const GraphNode& data) {
    // ... 前面的计算大小代码保持不变

    // 创建节点（中心在本地坐标原点）
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

    nodeItem->setPos(data.x, data.y);

    // 3. 优化文字
    QFont font("-apple-system, Microsoft YaHei", 10, QFont::Bold);
    QGraphicsTextItem* text = new QGraphicsTextItem(displayText, nodeItem);
    text->setDefaultTextColor(Qt::white);
    text->setFont(font);
    
    // ... 添加到 scene
}
```

**优化连线：**
修改 `createEdge` 中画笔的设置，让线条柔和一点：
```cpp
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
```

---

### 📐 修复步骤第四步：C++ 布局微调（去“土味”）

在你的 C++ 代码里，有些布局的默认设置会让 UI 显得很局促。

**1. 历史记录表格去网格线 (`src/ui/historypanel.cpp`)**
```cpp
void HistoryPanel::setupTable() {
    // ... 
    tableWidget_->setAlternatingRowColors(true); // 可以关掉，配合上面的 QSS 更好看
    tableWidget_->setShowGrid(false);            // 隐藏土味的 Excel 式网格线 !!
    tableWidget_->setFrameShape(QFrame::NoFrame);// 去除最外层边框
    tableWidget_->verticalHeader()->setDefaultSectionSize(40); // 增加行高，显得大气 !!
    // ...
}
```

**2. 结果树调整行高 (`src/ui/resulttreewidget.cpp`)**
树形控件挤在一起也很难看：
```cpp
void ResultTreeWidget::setupUi() {
    // ...
    setAlternatingRowColors(false); // 现代设计一般不交替背景色
    setFrameShape(QFrame::NoFrame);
    // 通过样式表稍微增加行高
    setStyleSheet("QTreeView::item { height: 35px; }");
}
```

**3. 给布局加点呼吸感 (`src/ui/extractionpanel.cpp`)**
```cpp
void ExtractionPanel::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 增加四周的留白 (Margin) 和控件间距 (Spacing)
    mainLayout->setContentsMargins(20, 20, 20, 20); 
    mainLayout->setSpacing(15);
    
    // 去掉代码里硬编码的字体，交给 QSS 接管
    // textEdit_->setFont(QFont("Microsoft YaHei", 10)); // 删掉这行
}
```

---

### 总结与立竿见影的建议

1. **删掉所有 Material 代码**：这会让你的 Qt 程序轻装上阵，不再卡顿，且解决所有的绘制冲突。
2. **应用我给你的 QSS**：只需把这段 CSS 放进你的 `.qss` 文件中重新编译运行，你会发现按钮变圆润了，输入框有焦点光环了，表格变得像 Web 端一样清爽。
3. **给按钮绑定正确的 ObjectName**：在 `ExtractionPanel.cpp` 里确保 `extractBtn_->setObjectName("extractBtn");` 等代码存在（你已经写了，很好），这样 QSS 就能精确命中并给它们上漂亮的颜色。

对于 OptiKG 这样一个面向工业领域的**知识抽取与图谱分析系统**，UI 设计的核心目标应该是：**专业、高效、克制、清晰**。

工业级/B2B 软件不需要花哨的渐变或夸张的动画，它需要的是“极高的信息信噪比”（让用户一眼看到重要数据）和“值得信赖的现代感”。

结合你现有的代码结构（左侧输入+历史，右侧图谱+结果树），我为你梳理了一套详细的 UI 设计升级指南。

---

### 一、 整体设计语言：现代卡片式设计 (Modern Card UI)

摒弃传统 Qt 软件“通体灰白、边框套边框”的旧工业风。采用现代 SaaS 软件流行的**卡片式布局**。

*   **底层背景色**：使用非常浅的灰蓝色（如 `#F0F2F5` 或 `#F5F7FA`）作为整个窗口的底色。
*   **内容载体（卡片）**：四个主要的面板（输入区、图谱区、结果树、历史记录）的背景设置为**纯白 (`#FFFFFF`)**。
*   **卡片边界**：不要用粗黑的线条画边框。使用极其柔和的浅色边框（`#EBEEF5`）加上细微的圆角（`Border-radius: 8px`），甚至可以加上一层非常淡的阴影（Drop Shadow）。
*   **效果**：这会让四个功能区像“悬浮在底版上的白纸”，层次分明，瞬间具备现代感。

---

### 二、 色彩与字体体系

#### 1. 字体系统 (Typography)
*   **字体选择**：丢掉宋体。强制使用系统级无衬线字体：`-apple-system, BlinkMacSystemFont, "Microsoft YaHei", "Segoe UI", Roboto, sans-serif`。
*   **字号层级**：
    *   面板标题：`14px` 或 `15px`，加粗 (`font-weight: 600`)，颜色深灰 (`#303133`)。
    *   正文/表格内容：`13px`，常规，颜色中灰 (`#606266`)。
    *   辅助说明/字符计数：`12px`，颜色浅灰 (`#909399`)。

#### 2. 品牌与状态色彩 (Semantic Colors)
*   **主品牌色 (Primary)**：选择一种**科技蓝**（如 `#409EFF` 或 `#1890FF`）。所有的主要操作（如【抽取】按钮、高亮的文字、选中的表格行）都用这个颜色。
*   **状态色**：
    *   成功 (Success)：柔和的绿色 `#67C23A`。
    *   警告 (Warning)：明亮的橙色 `#E6A23C`。
    *   危险/错误 (Danger)：克制的红色 `#F56C6C`（用于删除、清空按钮或错误提示）。
*   **实体颜色（知识图谱用）**：你代码里已经有了，建议采用“马卡龙色系”（明度高、饱和度偏低），这样上面写白字或黑字都能看清，不会刺眼。
    *   部件：`#FF8A80` (浅红)
    *   故障：`#80D8FF` (浅蓝)
    *   工具：`#FFE57F` (浅黄，字用深色)
    *   组成：`#B388FF` (浅紫)

---

### 三、 核心模块的 UI 优化建议

#### 1. 抽取面板 (Extraction Panel) - 引导用户的起点
*   **输入框**：取消传统文本框凹陷的感觉。做成纯平的，给一个浅色边框。当用户点击输入框（获取焦点）时，边框颜色变成**主品牌色（蓝色）**，并带一点点外发光（Outline shadow）。
*   **按钮设计**：
    *   **【抽取】按钮**是全场最重要的按钮，必须是**实心蓝底白字**，放在最显眼的位置（推荐右下角）。
    *   **【清空】按钮**是次要且危险的操作，做成**幽灵按钮（Ghost Button）**——即白底、红框、红字。只有鼠标悬浮时才变成淡淡的红底。

#### 2. 知识图谱展示区 (Graph Widget) - 软件的“面子”
这是最能体现你软件“高级感”的地方。
*   **节点形状**：不要画死板的正圆形。如果实体名称长，正圆形会很大很丑。建议改成**圆角矩形（Pill shape/胶囊形状）**，高度固定（如 30px），宽度随文字自动撑开，左右两边是半圆。
*   **连线与箭头**：放弃直线（`QGraphicsLineItem`）。使用 **贝塞尔曲线**（`QPainterPath::quadTo` 或 `cubicTo`），让连线呈现优美的弧度。箭头要小而锐利。
*   **交互高亮**：当鼠标悬浮 (`Hover`) 在某个节点上时：
    *   该节点放大 1.1 倍。
    *   与该节点无关的其他节点和连线，透明度降到 30%（变暗）。
    *   相关的连线变粗，颜色变亮。
    *   *这种“聚光灯”交互是专业图谱软件的标配。*

#### 3. 结果列表与历史记录 (Tree & Table Widgets)
数据展示切忌“像 Excel 一样死板”。
*   **去网格化**：隐藏所有的竖线，只保留非常浅的横向分割线（`#EBEEF5`）。
*   **行高与呼吸感**：默认的行高太挤了。把行高 (`Row Height`) 设置为 `36px` 到 `40px`。
*   **斑马纹退场**：现代 UI 很少用强烈的交替行颜色（斑马纹）。取而代之的是：背景纯白，当鼠标悬浮在某一行时，该行背景变成极浅的蓝色（`#F5F7FA`）。
*   **置信度可视化**：不要只显示 `0.85` 这样的冷冰冰的数字。把它变成一个**微型进度条**或者加上颜色小圆点（🟢高 🟡中 🔴低）。

#### 4. 空状态 (Empty States) - 极易被忽视的细节
当软件刚打开，或者没有抽取结果时，右侧会是两块巨大的白板，这非常难看。
*   **图谱空状态**：在图谱中心画一个淡淡的水印图标（如一个网络节点图的简笔画），下面写一行浅灰色的字：“*在左侧输入文本，点击抽取生成知识图谱*”。
*   **表格空状态**：同样放一个小图标（如一个空盒子），写着“*暂无数据*”。

---

### 四、 布局微调（基于你的 QSplitter）

你当前是左右布局，我建议在比例和分隔条上做文章：

1.  **隐藏生硬的分割线**：把 `QSplitter` 的把手（Handle）宽度设为 `6px`，并且背景设为透明。这样左右面板仿佛是分开的两个卡片，但鼠标放上去时光标会变成调节箭头，依然可以拖拽。
2.  **工具栏 (Toolbar)**：取消工具栏传统的灰色背景和边框。让工具栏与菜单栏融为一体（纯白），上面的图标按钮统一使用同一种风格的 SVG 图标（推荐使用 Material Design Icons 的线条版，不要用彩色的老式图标）。

---

### 五、 交互体验 (UX) 的加分项

1.  **加载动画 (Loading)**：
    当点击【抽取】后，不要只是干巴巴地把鼠标变成沙漏。
    *   在【抽取】按钮内部，文字变成“抽取中...”，并出现一个旋转的小圈圈（Spinner）。
    *   图谱区可以展示一个“骨架屏（Skeleton）”扫描效果，或者一个优雅的加载动画。
2.  **微动效**：
    *   打开设置弹窗时，不是瞬间闪出，而是带有一个 0.15秒 的渐显+上浮动画。
    *   节点拖拽时，连线要跟随得非常平滑。
3.  **快捷键提示**：在按钮的 Tooltip 里优雅地提示快捷键，比如 `抽取 (Ctrl+Enter)`，这符合专业工具的定位。
