# Windows 交叉编译方案

## 快速选择

| 方案 | 时间 | 难度 | 推荐度 |
|------|------|------|--------|
| A. GitHub Actions（云端编译）| 10 分钟 | ⭐ 最简单 | ⭐⭐⭐ **强烈推荐** |
| B. 社区 Qt6 镜像 | 10-20 分钟 | ⭐⭐ 中等 | ⭐⭐⭐ 推荐 |
| C. 自建 Qt6 镜像 | 2-4 小时 | ⭐⭐⭐ 复杂 | ⭐ 不推荐 |

---

## 方案 A：GitHub Actions（推荐 ⭐⭐⭐）

**无需本地编译**，直接在 GitHub 云端免费编译 Windows 版本！

### 使用方法

1. **Fork 你的项目到 GitHub**（如果还没上传）

2. **创建工作流文件**：
   ```bash
   mkdir -p .github/workflows
   cp cross-compile/.github/workflows/build-windows.yml .github/workflows/
   git add .github/workflows/build-windows.yml
   git commit -m "Add Windows build workflow"
   git push
   ```

3. **触发编译**：
   - 在 GitHub 网页进入你的仓库
   - 点击 "Actions" 标签
   - 选择 "Build Windows Executable"
   - 点击 "Run workflow"

4. **下载结果**：
   - 等待 5-10 分钟
   - 编译完成后，在 Actions 页面下载 `qt-try-windows` artifact

### 优势
- ✅ 免费
- ✅ 无需本地配置
- ✅ 自动打包所有依赖
- ✅ 可以在任何地方下载

---

## 方案 B：社区 Qt6 镜像（本地 Docker）

使用别人已经构建好的 Qt6 Docker 镜像。

### 步骤

```bash
cd cross-compile

# 1. 准备 ONNX
./setup-cross-build.sh

# 2. 拉取社区镜像（约 1-2 GB）
docker pull a12e/docker-qt6-windows:6.5-mingw-w64-ltsc2022

# 3. 编译
./build-windows.sh
```

### 可能的问题
- 镜像可能较大（1-2 GB）
- 可能需要翻墙下载
- 版本可能不是最新的

---

## 方案 C：自建 Qt6 镜像（2-4 小时）

如果方案 A 和 B 都不可用，只能自己编译 Qt6。

```bash
./build-qt6-image.sh  # 耗时 2-4 小时
./build-windows.sh
```

---

## 推荐顺序

1. **首选方案 A**（GitHub Actions）- 最简单
2. **备选方案 B**（社区镜像）- 如果 GitHub 不方便
3. **最后方案 C**（自建）- 如果以上都失败

---

## 立即开始

### 最快的方式（GitHub Actions）：

```bash
# 1. 确保代码在 GitHub 上
git push origin main

# 2. 复制工作流文件
mkdir -p .github/workflows
cp cross-compile/.github/workflows/build-windows.yml .github/workflows/
git add .github/workflows/
git commit -m "Add Windows build"
git push

# 3. 去 GitHub 网页触发 Actions
```

然后到 https://github.com/你的用户名/你的仓库/actions 查看进度！
