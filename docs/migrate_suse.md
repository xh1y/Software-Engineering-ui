# 迁移到 OpenSUSE Tumbleweed 指南

本文档指导如何将 `qt-try` 项目从 Fedora 迁移到 OpenSUSE Tumbleweed。包括所有必要的软件包安装、环境配置和构建步骤。

## 注意事项

OpenSUSE Tumbleweed 使用 `zypper` 包管理器。与 Fedora 的 `dnf` 相比：

- 包名中开发包通常也以 `-devel` 结尾，但具体命名可能不同
- 系统库路径默认为 `/usr/lib64`（64位）
- 推荐先启用 Packman 社区仓库以获取更完整的软件包

如果某个包找不到，可以使用以下命令搜索：

```bash
# 搜索包
zypper search <关键词>

# 查看包详细信息
zypper info <包名>

# 搜索包含某个文件的包（非常实用）
zypper what-provides <文件路径>
```

## 1. 系统更新与基础工具

首先更新系统并安装基础开发工具：

```bash
sudo zypper refresh
sudo zypper update -y
sudo zypper install -y -t pattern devel_basis
sudo zypper install -y cmake git wget curl pkg-config
```

## 2. C++ 构建工具链

GCC 和 G++ 已包含在 `devel_basis` 模式中，单独确认安装：

```bash
sudo zypper install -y gcc gcc-c++ make
```

验证安装：
```bash
gcc --version
g++ --version
```

## 3. Qt6 开发环境

安装 Qt6 核心库和开发包：

```bash
sudo zypper install -y qt6-base-devel qt6-base-private-devel
sudo zypper install -y qt6-sql-devel qt6-concurrent-devel
sudo zypper install -y qt6-gui-devel qt6-widgets-devel
sudo zypper install -y qt6-test-devel
```

可选：安装 Qt6 翻译工具（用于编译 .ts 文件）：

```bash
sudo zypper install -y qt6-tools-devel
```

## 4. Protobuf

安装 Protocol Buffers 编译器和开发库：

```bash
sudo zypper install -y protobuf-devel protobuf-compiler
```

验证安装：
```bash
protoc --version
```

## 5. ONNX Runtime

ONNX Runtime 在 openSUSE 官方仓库中不可用。推荐使用预编译版本。

### 方案 A：使用预编译版本（推荐）

1. 下载预编译的 ONNX Runtime Linux 版本：

```bash
cd /opt
sudo wget https://github.com/microsoft/onnxruntime/releases/download/v1.24.4/onnxruntime-linux-x64-1.24.4.tgz
sudo tar -zxvf onnxruntime-linux-x64-1.24.4.tgz
sudo mv onnxruntime-linux-x64-1.24.4 onnxruntime
sudo rm onnxruntime-linux-x64-1.24.4.tgz
```

2. 设置环境变量（添加到 `~/.bashrc`）：

```bash
export ONNXRUNTIME_ROOT=/opt/onnxruntime
export LD_LIBRARY_PATH=$ONNXRUNTIME_ROOT/lib:$LD_LIBRARY_PATH
```

运行 `source ~/.bashrc` 使配置生效。

### 方案 B：从源码编译

```bash
sudo zypper install -y python3 python3-pip
git clone --recursive https://github.com/microsoft/onnxruntime
cd onnxruntime
./build.sh --config Release --build_shared_lib --parallel
sudo cp -r build/Linux/Release /opt/onnxruntime
```

## 6. Rust 工具链（tokenizers-cpp 依赖）

tokenizers-cpp 子模块需要 Rust 工具链：

```bash
# 安装 Rust（使用官方脚本，推荐）
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
source $HOME/.cargo/env

# 或者通过包管理器安装（版本可能较旧）
# sudo zypper install -y rust cargo
```

验证安装：
```bash
rustc --version
cargo --version
```

## 7. 其他系统依赖

安装 OpenGL、SQLite 和其他必要库：

```bash
sudo zypper install -y Mesa-libGL-devel Mesa-libGLU-devel
sudo zypper install -y sqlite3-devel
sudo zypper install -y libxkbcommon-devel
sudo zypper install -y libicu-devel
sudo zypper install -y double-conversion-devel
sudo zypper install -y sentencepiece-devel  # tokenizers-cpp 需要
```

> **注意**：`libgomp` 在 openSUSE 中由 `gcc` 包自带，无需单独安装。

## 8. 可选：无头运行环境（用于测试）

如果需要在无显示器的环境中运行（如服务器），安装虚拟显示和 VNC：

```bash
sudo zypper install -y xorg-x11-server-extra   # 提供 Xvfb
sudo zypper install -y x11vnc
sudo zypper install -y novnc
sudo zypper install -y python3-websockify      # novnc 的 WebSocket 依赖
```

## 9. 字体和本地化支持

安装中文字体和本地化工具：

```bash
sudo zypper install -y google-noto-sans-cjk-fonts google-noto-serif-cjk-fonts
sudo zypper install -y wqy-microhei-fonts
sudo zypper install -y glibc-locale
```

> **注意**：openSUSE 默认已包含完整的 locale 支持。如需中文环境，设置：
> ```bash
> export LANG=zh_CN.UTF-8
> ```

## 10. 项目构建步骤

完成所有依赖安装后，构建项目：

```bash
cd /path/to/qt-try

# 清理并创建构建目录
rm -rf build
mkdir build
cd build

# 配置 CMake（路径与 Fedora 一致）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build . -j$(nproc)

# 运行
./qt-try
```

## 11. Docker 构建（可选）

如果仍想使用 Docker 构建，确保 Docker 已安装：

```bash
sudo zypper install -y docker docker-compose
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
# 需要重新登录使组权限生效
```

构建 Docker 镜像：
```bash
./build-image.sh
```

## 12. 故障排除

### 问题：找不到 Qt6 库

```bash
# 确保 Qt6 开发包已安装
zypper search --installed-only qt6

# 设置 Qt6 环境变量
export QT_SELECT=qt6
```

### 问题：CMake 找不到 Protobuf

```bash
# 确保开发包已安装
sudo zypper install -y protobuf-devel protobuf-compiler

# 手动指定路径（如果必要）
cmake .. -DProtobuf_DIR=/usr/lib64/cmake/protobuf
```

### 问题：tokenizers-cpp 构建失败

```bash
# 确保 Rust 已安装且版本较新
rustup update

# 清理并重试
cd 3rdparty/tokenizers-cpp
cargo clean
```

### 问题：zypper 锁冲突

openSUSE Tumbleweed 滚动更新时可能遇到锁冲突：

```bash
# 查看哪些进程持有 zypper 锁
ps aux | grep zypper

# 等待后台更新完成，或手动移除锁（谨慎使用）
sudo rm /run/zypp.lock
```

### 问题：库路径问题

openSUSE 64位库路径为 `/usr/lib64`，如果 CMake 找不到某些库：

```bash
# 查看库的实际安装位置
zypper what-provides '*/libQt6Core.so*'

# 在 CMake 中指定库路径
cmake .. -DCMAKE_LIBRARY_PATH=/usr/lib64
```

## 13. 验证安装

创建测试脚本来验证所有依赖：

```bash
#!/bin/bash
echo "=== 依赖检查 ==="

# 编译器
command -v gcc && echo "✓ GCC" || echo "✗ GCC"
command -v g++ && echo "✓ G++" || echo "✗ G++"

# Qt
pkg-config --modversion Qt6Core && echo "✓ Qt6 Core" || echo "✗ Qt6 Core"

# Protobuf
command -v protoc && echo "✓ protoc" || echo "✗ protoc"

# ONNX Runtime
[ -d /opt/onnxruntime ] && echo "✓ ONNX Runtime" || echo "✗ ONNX Runtime"

# Rust
command -v rustc && echo "✓ Rust" || echo "✗ Rust"

# SQLite3
pkg-config --modversion sqlite3 && echo "✓ SQLite3" || echo "✗ SQLite3"

echo "=== 检查完成 ==="
```

保存为 `check_deps.sh` 并运行：

```bash
chmod +x check_deps.sh
./check_deps.sh
```

## 14. Fedora → openSUSE 包名对照速查表

| 依赖 | Fedora (dnf) | openSUSE (zypper) |
|------|-------------|-------------------|
| 构建工具集 | `"Development Tools"` group | `devel_basis` pattern |
| Qt6 基础 | `qt6-qtbase-devel` | `qt6-base-devel` |
| Qt6 私有头文件 | `qt6-qtbase-private-devel` | `qt6-base-private-devel` |
| Qt6 SQL | `qt6-qtsql-devel` | `qt6-sql-devel` |
| Qt6 Concurrent | `qt6-qtconcurrent-devel` | `qt6-concurrent-devel` |
| Qt6 Test | `qt6-qttest-devel` | `qt6-test-devel` |
| Qt6 翻译工具 | `qt6-qttools` | `qt6-tools-devel` |
| Protobuf | `protobuf-devel` | `protobuf-devel` |
| Mesa GL | `mesa-libGL-devel` | `Mesa-libGL-devel` |
| SQLite | `sqlite-devel` | `sqlite3-devel` |
| xkbcommon | `libxkbcommon-devel` | `libxkbcommon-devel` |
| ICU | `libicu-devel` | `libicu-devel` |
| double-conversion | `double-conversion-devel` | `double-conversion-devel` |
| SentencePiece | `sentencepiece-devel` | `sentencepiece-devel` |
| Xvfb | `xorg-x11-server-Xvfb` | `xorg-x11-server-extra` |
| x11vnc | `x11vnc` | `x11vnc` |
| websockify | `websockify` | `python3-websockify` |
| 中文字体 | `wqy-microhei-fonts` | `wqy-microhei-fonts` |
| CJK 字体 | `google-noto-cjk-fonts` | `google-noto-sans-cjk-fonts` |
| 中文 locale | `glibc-langpack-zh` | `glibc-locale`（已含） |
| Docker | `docker` | `docker` |

## 15. 参考链接

- [OpenSUSE 包搜索](https://software.opensuse.org/)
- [OpenSUSE Wiki: 包管理](https://en.opensuse.org/Portal:Zypper)
- [Qt6 openSUSE 安装指南](https://en.opensuse.org/Qt6)
- [ONNX Runtime 官方文档](https://onnxruntime.ai/docs/)
- [Rust 安装指南](https://www.rust-lang.org/tools/install)

---

**最后更新：2026-05-05**

*根据当前项目 CMakeLists.txt 和 Dockerfile 分析生成*
*适用于 OpenSUSE Tumbleweed 滚动发行版*
