# 代码优化与文档修正 — 修改记录

> 修改日期: 2026-04-25
> 基于: docs/modify.md 原始差异清单 + 实际代码审查

---

## 一、代码修改

### 1.1 `src/core/inferenceworker.h` — 重构头文件，消除重复成员

**修改前（63行）：** 声明了 `loadModel()`, `loadTokenizer()`, `loadMetadata()`, `runOnnxInference()`, `tokenizeAndPredict()`, `inferWithChunking()`, `simulateInference()`, `cleanup()` 共8个方法，以及 `env_`, `session_`, `allocator_`, `tokenizer_`, `maxSeqLength_`, `id2predicate_`, `modelLoaded_` 共7个成员变量——完全重复了 `InferenceEngine` 的内部实现。

**修改后（39行）：** 仅保留 `setText()`, `setModelPath()`, `setThreshold()`, `stop()` 四个配置方法 + `run()` 线程入口。删除全部重复成员变量。`run()` 内部创建 `InferenceEngine` 局部实例并委托执行推理。

### 1.2 `src/core/inferenceworker.cpp` — 删除约480行重复推理代码

**修改前（562行）：** 在 `run()` 中独立实现了 ONNX Runtime 模型加载、Tokenizer 加载、Metadata 解析、输入编码、推理执行、输出张量解析、实体关系提取等完整推理流程，与 `InferenceEngine` 完全重复。

**修改后（84行）：** `run()` 简化为：
1. 创建 `InferenceEngine` 局部实例
2. `engine.loadModel(modelPath_)` — 委托加载
3. `engine.setThreshold(threshold_)` — 委托配置
4. `engine.infer()` 或 `engine.inferLongText()` — 委托推理
5. 按阈值过滤结果并发射信号

**消除的问题：**
- 两个类的 `maxSeqLength_` 默认值不一致（InferenceWorker 512 vs InferenceEngine 128）——现已统一
- `getVal` lambda 中的断言校验差异 ——现已统一为 InferenceEngine 的校验
- 移除了无用的 `#include <cxxabi.h>`

### 1.3 `src/core/inferenceengine.cpp` — 清理 fprintf 调试输出

**共删除 8 处 `fprintf(stderr, ...)` / `fflush(stderr)` 块（约20行）：**
| 位置 | 删除内容 |
|------|---------|
| runInference 入口 | `"Running inference with input shapes..."` |
| 推理完成后 | `"Inference completed, output tensors count..."` |
| 输出张量形状 | `"Output tensor 1 shape..."` `"Output tensor 2 shape..."` |
| 输出张量参数 | `"Output tensor shape..."` `"STRIDE: ..."` |
| 张量元素计数 | `"Output tensor element counts..."` |
| 实体提取循环 | `"Starting entity extraction loop..."` |

同时删除了3个因 fprintf 移除而不再使用的局部变量：`totalElements0`, `totalElements1`, `totalElements2`。

### 1.4 `src/ui/mainwindow.h` — 删除存根槽声明

删除了两行：
```cpp
void onSaveClicked();
void onSearchClicked();
```

### 1.5 `src/ui/mainwindow.cpp` — 删除存根实现和废弃菜单项

**删除 `onSaveClicked()` 存根（540-542行）：**
```cpp
// 删除前
void MainWindow::onSaveClicked() {
    showToast(tr("保存功能尚未实现"), true);
}
```

**删除 `onSearchClicked()` 存根（544-546行）：**
```cpp
// 删除前
void MainWindow::onSearchClicked() {
    showToast(tr("搜索功能尚未实现"), true);
}
```
> 注：实际搜索功能存在于 `HistoryPanel::onSearch()` + `DatabaseManager::searchExtractionRecords()`，该存根从未被调用。

**删除"保存(&S)"菜单项（159-162行）：**
```cpp
// 删除前
QAction* saveAction = fileMenu->addAction(tr("保存(&S)"));
saveAction->setShortcut(QKeySequence::Save);
connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveClicked);
```
> 导出功能（JSON/CSV）已覆盖实际需求，无需独立的保存菜单项。

**修正帮助文字（1123行）：**
```
- "<li><b>文本输入：</b>在左侧文本框中输入维修描述文本（最多1000字符）</li>"
+ "<li><b>文本输入：</b>在左侧文本框中输入维修描述文本（最多5000字符）</li>"
```
> 与 `ExtractionPanel::MAX_CHARS = 5000` 保持一致。

### 1.6 `src/data/databasemanager.cpp` — 删除误导性 TODO 注释

**删除（307行）：**
```cpp
// TODO: 实现其他方法：updateExtractionRecord, deleteExtractionRecord, getAllExtractionRecords等
```
> `deleteExtractionRecord` 和 `getAllExtractionRecords` 早已实现；`updateExtractionRecord` 为预留 API（返回 false），无需 TODO 标记。

---

## 二、文档修改 (`docs/total_doc.md`)

### 2.1 版本号统一

| 修改位置 | 修改前 | 修改后 |
|---------|--------|--------|
| §1.2 产品定位 | ONNX Runtime **1.16.3** | ONNX Runtime **1.24** |
| §5.1 开发环境表 | ONNX Runtime **1.16.3** (正文) | ONNX Runtime **1.24** |
| §5.1 开发环境表 | Qt **6.11** | Qt **6** |
| §6.2 测试环境表 | Qt **6.10.3** + ONNX Runtime **1.16.3** | Qt **6** + ONNX Runtime **1.24** |
| §7.2.1 CI 配置 | ONNX Runtime **1.16.3** | ONNX Runtime **1.24** |
| §7.6 项目总结 | ONNX Runtime **1.16.3** | ONNX Runtime **1.24** |

> Qt 6.11 和 Qt 6.10.3 均为不存在的版本号，统一改为 `Qt 6`。

### 2.2 删除不存在功能的文档描述

| 删除内容 | 原位置 |
|---------|--------|
| **UC7 三元组审核** | §3.3.1 用例表 |
| **UC8 用户管理** | §3.3.1 用例表 |
| **NFR-14 访问控制（角色基础的访问控制）** | §3.5.3 安全性需求表 |
| **图5-1 登录界面** | §5.3 UI界面截图列表 |
| **图5-5 审核管理界面** | §5.3 UI界面截图列表 |

> 代码中不存在 login/auth/audit/user/role/permission 相关实现，这些功能从未开发。

### 2.3 修正架构描述

| 修改项 | 修改前 | 修改后 |
|--------|--------|--------|
| InferenceWorker 描述 (§5.2.7) | "QThread子类，后台执行推理避免UI阻塞。①接收文本和模型路径配置；②异步执行推理…" | "QThread子类，后台执行推理避免UI阻塞，**内部委托InferenceEngine完成实际推理**。①接收文本、模型路径和阈值配置；②**在run()中创建InferenceEngine局部实例并委托执行推理**…" |
| 总代码行数 (§5.2, §7.6) | **约9300行** | **约8800行**（删除约500行重复代码） |
| core/模块行数 (§5.2.7) | **约2200行** | **约1700行** |

### 2.4 修正力导向布局参数描述 (§4.6.4)

**修改前（固定值）：**
> "执行 Fruchterman-Reingold 力导向布局（迭代100次，斥力系数100.0，引力系数0.1，阻尼0.8）"

**修改后（动态参数）：**
> "执行 Fruchterman-Reingold 力导向布局（根据节点数动态选择参数：>100节点：迭代80次/斥力200/引力0.01/阻尼0.80；50-100节点：迭代70次/斥力180/引力0.015/阻尼0.82；20-50节点：迭代60次/斥力150/引力0.02/阻尼0.85；10-20节点：迭代80次/斥力120/引力0.03/阻尼0.88；<10节点：迭代120次/斥力100/引力0.035/阻尼0.90）"

> 与实际代码 `graphwidget.cpp:278-290` 的 5 组分段参数策略一致。

### 2.5 修正 SettingsDialog 配置项描述 (§4.6.7)

**修改前：**
> "用户修改：模型路径…JSON内容字段名、CSV列名和编码、分块大小和重叠大小"

**修改后：**
> "用户修改：模型路径…JSON内容字段名、CSV列名和编码、输出目录和导出格式"

> SettingsDialog 实际只有三个选项卡（字段映射/模型设置/批量处理），没有 chunkSize/overlapSize 的 UI 控件。

---

## 三、修改统计

| 类别 | 文件 | 变更 |
|------|------|------|
| 代码 | `inferenceworker.h` | 63行 → 39行 (-24) |
| 代码 | `inferenceworker.cpp` | 562行 → 84行 (-478) |
| 代码 | `inferenceengine.cpp` | 删除 ~20行 fprintf + 3个无用变量 |
| 代码 | `mainwindow.h` | 删除 2行存根声明 |
| 代码 | `mainwindow.cpp` | 删除 ~10行存根代码 + 修正1行帮助文字 |
| 代码 | `databasemanager.cpp` | 删除 1行误导性 TODO |
| 文档 | `total_doc.md` | 统一4处版本号 + 删除2个用例/1个NFR/2个截图 + 修正架构描述/力导向参数/SettingsDialog配置项/行数 |

**净减少代码：约530行**（主要是消除 InferenceWorker 与 InferenceEngine 的重复实现）

---

## 四、覆盖率数据更新 (2026-04-25)

基于代码重构后重新编译（`--coverage`）、运行全部 10 个测试（全部通过），并使用 gcovr（行覆盖、分支覆盖）和 lcov（函数覆盖）实测后，对 `docs/total_doc.md` 中以下 5 处进行了更新：

### 4.1 §6.3.1 推理引擎测试结果（第 1292 行）

| 字段 | 旧值 | 新值 |
|------|------|------|
| inferenceengine.cpp 行覆盖率 | **84.3%** | **83.7%**（314/375 行） |

变化原因：新增 ModelParams 命名空间常量、改进 getVal() 边界检查、loadModel() 错误处理改为 Logger::error，函数覆盖率保持 100.0%。

### 4.2 §6.3.2 数据库管理测试结果（第 1324 行）

| 字段 | 旧值 | 新值 |
|------|------|------|
| databasemanager.cpp 行覆盖率 | **71.0%** | **67.0%**（425/634 行） |

变化原因：updateExtractionRecord() 从桩实现（仅 `Q_UNUSED(record); return false;`）变为完整的 SQL UPDATE 实现，saveTriples() 返回类型从 void 改为 bool 并增加错误处理路径，总行数从约 594 增至 634，函数覆盖率保持 83.9%。

### 4.3 §6.6.2 代码覆盖率度量说明正文（第 1525 行）

**旧文：** "核心模块 inferenceengine.cpp 行覆盖率 84.3%、databasemanager.cpp 行覆盖率 71.0%、graphwidget.cpp 行覆盖率 89.9%"

**新文：** "核心模块 inferenceengine.cpp 行覆盖率 83.7%、databasemanager.cpp 行覆盖率 67.0%、graphwidget.cpp 行覆盖率 89.9%"

### 4.4 表 6-15 覆盖度量体系（第 1529–1533 行）

| 度量 | 旧值 | 新值 | 变化说明 |
|------|------|------|---------|
| 行覆盖 | 48.0%（2 337 / 4 871 行） | **51.9%**（2 321 / 4 473 行） | 消除约 400 行重复代码（总行数 ↓398），已执行行仅净减 16 行 |
| 函数覆盖 | 52.7%（177 / 336 个） | **52.9%**（166 / 314 个） | 消除重复/未使用函数（函数总数 ↓22），覆盖率小幅上升 |
| 分支覆盖 | 23.3%（2 861 / 12 273 个） | **25.4%**（2 841 / 11 194 个） | 代码简化减少分支总数（↓1079），已覆盖分支略降（↓20） |

### 4.5 表 6-16 子目录行覆盖汇总表（第 1539–1544 行）

| 目录 | 旧 .cpp 行覆盖 | 新 .cpp 行覆盖 | 关键文件变化 |
|------|---------------|---------------|-------------|
| core/ | 35.7%（509 / 1 425 行） | **47.7%**（498 / 1 043 行） | batchprocessor.cpp 26.6%→**29.8%**（提取 processSingleText 消除约 80 行重复） |
| data/ | 71.6%（452 / 631 行） | **67.8%**（455 / 671 行） | databasemanager.cpp 71.0%→**67.0%**（新增 updateExtractionRecord 实现） |
| utils/ | 48.5%（221 / 456 行） | **49.6%**（226 / 456 行） | logger.cpp 53.9%→**57.1%**（fprintf 替代 qDebug 回退路径） |
| ui/ | 48.8%（1 141 / 2 339 行） | **50.0%**（1 142 / 2 283 行） | mainwindow.cpp 35.0%→**37.7%**（提取 updateGraphFromTriples 消除约 56 行重复） |

> "全部文件行覆盖"列与 ".cpp 文件行覆盖"列数值相同（头文件经 lcov 单独过滤后不贡献可执行代码行）。
