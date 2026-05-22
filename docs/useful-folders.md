# OptiKG 项目源代码目录说明（交作业版）

> 本文档列出本项目**自研的、需要提交的核心源代码**，方便归档和打包。第三方库、构建产物、IDE 配置等不包含在内。

---

## 一、核心源代码（src/）

### 1. 推理引擎层（src/core/）
| 文件 | 说明 | 行数 |
|------|------|------|
| `inferenceengine.cpp` / `.h` | ONNX 推理引擎核心：模型加载、Tokenizer 编码、推理执行、结果解析、长文本分块 | ~500 |
| `inferenceworker.cpp` / `.h` | QThread 子类，后台异步执行推理，避免 UI 阻塞 | ~120 |
| `batchprocessor.cpp` / `.h` | 批量处理线程：JSON/CSV 扫描、多文件并行处理、进度反馈 | ~650 |

### 2. 数据持久化层（src/data/）
| 文件 | 说明 | 行数 |
|------|------|------|
| `databasemanager.cpp` / `.h` | SQLite 单例管理类：三张核心表（extraction_records / entities / relations）的 CRUD、事务、级联删除、导入导出 | ~760 |
| `datamodel.cpp` / `.h` | 公共数据结构：Entity、Triple、ExtractionRecord、GraphNode、GraphEdge 及类型枚举 | ~80 |

### 3. 用户界面层（src/ui/）
| 文件 | 说明 | 行数 |
|------|------|------|
| `mainwindow.cpp` / `.h` | 主窗口：四区域分割布局、菜单/工具栏、模块协调、状态栏 | ~650 |
| `extractionpanel.cpp` / `.h` | 抽取面板：文本输入框、字符计数、抽取/清空按钮 | ~130 |
| `graphwidget.cpp` / `.h` | 知识图谱可视化：QGraphicsView 渲染、Fruchterman-Reingold 力导向布局、节点拖拽/缩放/高亮 | ~960 |
| `historypanel.cpp` / `.h` | 历史记录面板：表格展示、关键词/日期/类型筛选、批量删除、导出 | ~300 |
| `resulttreewidget.cpp` / `.h` | 结果树：按实体类型和关系分组展示三元组 | ~120 |
| `batchprocessdialog.cpp` / `.h` | 批量处理对话框：文件列表、字段映射、进度条、日志、自动导出 | ~380 |
| `settingsdialog.cpp` / `.h` | 设置对话框：字段映射、模型路径、置信度阈值、批量输出配置 | ~180 |

### 4. 工具支撑层（src/utils/）
| 文件 | 说明 | 行数 |
|------|------|------|
| `appconstants.cpp` / `.h` | 系统常量集中管理：分块参数、阈值、文件扩展名、模型搜索路径、实体类型映射 | ~280 |
| `configmanager.cpp` / `.h` | 配置管理单例：基于 QSettings 持久化读写，支持自动检测模型路径 | ~180 |
| `logger.cpp` / `.h` | 日志系统单例：QMutex 线程安全、五级日志、Qt 消息重定向、按日期分文件 | ~240 |
| `stylemanager.cpp` / `.h` | 样式管理单例：QSS 样式表加载/应用/重载，支持文件系统和资源系统 | ~90 |

### 5. 程序入口
| 文件 | 说明 |
|------|------|
| `src/main.cpp` | 程序入口：初始化日志、样式、QTranslator 国际化、加载主窗口 |

**src/ 目录合计：约 9100 行，32 个文件（17 .cpp + 15 .h）**

---

## 二、公共头文件（include/optikg/）

| 文件 | 说明 |
|------|------|
| `datamodel.h` | 跨模块共享的数据结构体声明（Entity、Triple、ExtractionRecord、GraphNode、GraphEdge） |

---

## 三、单元测试（tests/）

共 **15 个测试文件**，约 **201 个测试用例**，覆盖白盒测试、黑盒测试、性能测试和可靠性测试。

| 文件 | 说明 | 用例数 |
|------|------|--------|
| `test_inferenceengine.cpp` | 推理引擎：模型加载/卸载、空文本、正常推理、阈值过滤、长文本分块 | 16 |
| `test_inferenceworker.cpp` | 推理线程：参数设置、错误路径、正常推理、长文本、阈值过滤 | 9 |
| `test_batchprocessor.cpp` | 批量处理：JSON/CSV 扫描、空文件、中断、多文件、导出 | 24 |
| `test_databasemanager.cpp` | 数据库：单例、CRUD、级联保存/删除、批量插入、导入导出、事务 | 21 |
| `test_graphwidget.cpp` | 力导向布局：空节点、单/多节点、高亮、信号、缩放、收敛 | 14 |
| `test_resulttreewidget.cpp` | 结果树：构造、空结果、多实体、清空、数据绑定 | 8 |
| `test_appconstants.cpp` | 应用常量：默认值、参数读写、边界验证、重置、持久化 | 19 |
| `test_configmanager.cpp` | 配置管理：单例、默认值、读写、持久化、信号 | 15 |
| `test_stylemanager.cpp` | 样式管理：单例、文件/资源加载、应用、重载、错误处理 | 7 |
| `test_settingsdialog.cpp` | 设置对话框：构造、UI 查找、配置加载、按钮交互、恢复默认 | 7 |
| `test_mainwindow.cpp` | 主窗口：UI 构造、菜单、快捷键、状态栏、清空、导出 | 18 |
| `test_batchprocessdialog.cpp` | 批量对话框：按钮状态、进度条、统计、复选框同步、日志 | 21 |
| `test_datamodel.cpp` | 数据模型：Entity/Triple/GraphNode/GraphEdge 构造与转换 | 14 |
| `test_performance.cpp` | 性能测试：推理、长文本、图谱、批量、内存、启动、查询 | 7 |
| `test_reliability.cpp` | 可靠性测试：异常输入、模型失败、数据库损坏、中断恢复 | 8 |

---

## 四、构建与资源配置

### 构建配置
| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | CMake 跨平台构建脚本：Qt6 / ONNX Runtime / tokenizers-cpp / Protobuf 依赖管理、翻译自动编译、模型拷贝 |
| `tests/CMakeLists.txt` | 测试子模块构建配置 |

### 资源文件（resources/）
| 文件 | 说明 |
|------|------|
| `resources.qrc` | Qt 资源系统：嵌入 QSS 样式表 |
| `styles/modern_pro.qss` | 系统默认主题样式表 |
| `icons/optikg.png` | 应用图标（AppImage / 桌面快捷方式用） |
| `optikg.desktop` | Linux 桌面入口配置 |

### 国际化
| 文件 | 说明 |
|------|------|
| `qt-try_en.ts` | 英文翻译源文件（含 259 条翻译条目） |
| `qt-try_en.qm` | 编译后的英文翻译二进制文件 |

### 模型文件（models/）
| 文件 | 说明 |
|------|------|
| `model_fp32.onnx` / `.data` | ONNX 推理模型及外部权重 |
| `tokenizer.json` / `tokenizer_config.json` / `vocab.txt` | HuggingFace 兼容分词器配置 |
| `metadata.json` | 模型元数据（max_len、threshold、id2predicate 映射） |

---

## 五、Docker 部署配置（可选）

如需提交容器化部署相关内容，可包含：

| 文件 | 说明 |
|------|------|
| `Dockerfile` | 多阶段构建：Qt6 + ONNX Runtime + Rust 工具链编译 |
| `docker-build/` 目录下相关 `.yml` / `.sh` | docker-compose 编排、一键启动脚本 |

---

## 六、**不需要提交的目录**

以下目录为生成文件或第三方依赖，交作业时应排除：

```
build/              # CMake 构建输出（自动生成）
build-release/      # Release 构建输出
cmake-build-debug/  # IDE Debug 构建输出
3rdparty/           # 第三方库（tokenizers-cpp 子模块，Git 可递归拉取）
coverage_html/      # 覆盖率 HTML 报告（可由 gcovr 重新生成）
.git/               # Git 仓库元数据
.idea/              # CLion IDE 配置
.qtcreator/         # Qt Creator 配置
AppDir/             # AppImage 打包中间目录（自动生成）
*.AppImage          # 打包产物（自动生成）
```

---

## 七、快速打包命令

如需将源代码打包为提交压缩包，可使用：

```bash
# 打包核心代码 + 测试 + 构建配置 + 文档
zip -r optikg-source.zip \
    src/ include/ tests/ CMakeLists.txt tests/CMakeLists.txt \
    resources/ models/ qt-try_en.ts qt-try_en.qm \
    Dockerfile docker-build/ \
    docs/total_doc.md docs/useful-folders.md
```
