# 迁移到 Fedora 指南

本文档指导如何将 `qt-try` 项目从 Arch Linux 迁移到 Fedora。包括所有必要的软件包安装、环境配置和构建步骤。

## 注意事项

Fedora 使用 `dnf` 包管理器，包名可能与 Arch Linux 的 `pacman` 不同。如果某个包找不到，可以使用以下命令搜索：

```bash
# 搜索包
dnf search <关键词>

# 查看包详细信息
dnf info <包名>
```

文档中的包名基于 Fedora 38+ 的默认仓库。如果遇到问题，请参考 [Fedora 包数据库](https://packages.fedoraproject.org/)。

## 1. 系统更新与基础工具

首先更新系统并安装基础开发工具：

```bash
sudo dnf update -y
sudo dnf groupinstall "Development Tools" -y
sudo dnf install -y cmake git wget curl pkg-config
```

## 2. C++ 构建工具链

安装 GCC 和 G++ 编译器：

```bash
sudo dnf install -y gcc gcc-c++ make
```

验证安装：
```bash
gcc --version
g++ --version
```

## 3. Qt6 开发环境

安装 Qt6 核心库和开发包：

```bash
sudo dnf install -y qt6-qtbase-devel qt6-qtbase qt6-qtbase-private-devel
sudo dnf install -y qt6-qtsql-devel qt6-qtconcurrent-devel
sudo dnf install -y qt6-qtgui-devel qt6-qtwidgets-devel
```

可选：安装 Qt6 文档和示例（用于开发参考）：
```bash
sudo dnf install -y qt6-doc qt6-examples
```

## 4. Protobuf

安装 Protocol Buffers 编译器和开发库：

```bash
sudo dnf install -y protobuf-devel protobuf-compiler
```

验证安装：
```bash
protoc --version
```

## 5. ONNX Runtime

ONNX Runtime 在 Fedora 官方仓库中可能不可用。推荐从源码编译或使用预编译版本。

### 方案 A：使用预编译版本（推荐）

1. 下载预编译的 ONNX Runtime Linux 版本：
```bash
cd /opt
sudo wget https://github.com/microsoft/onnxruntime/releases/download/v1.24.4/onnxruntime-linux-x64-1.24.4.tgz
sudo tar -zxvf onnxruntime-linux-x64-1.24.4.tgz
sudo mv onnxruntime-linux-x64-1.24.4 onnxruntime
```

2. 设置环境变量（添加到 `~/.bashrc`）：
```bash
export ONNXRUNTIME_ROOT=/opt/onnxruntime
export LD_LIBRARY_PATH=$ONNXRUNTIME_ROOT/lib:$LD_LIBRARY_PATH
export C_INCLUDE_PATH=$ONNXRUNTIME_ROOT/include:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=$ONNXRUNTIME_ROOT/include:$CPLUS_INCLUDE_PATH
```

### 方案 B：从源码编译

如果需要自定义功能，可以从源码编译：

```bash
sudo dnf install -y cmake git python3 python3-pip
git clone --recursive https://github.com/microsoft/onnxruntime
cd onnxruntime
./build.sh --config Release --build_shared_lib --parallel
sudo cp -r build/Linux/Release /opt/onnxruntime
```

## 6. Rust 工具链（tokenizers-cpp 依赖）

tokenizers-cpp 子模块需要 Rust 工具链：

```bash
# 安装 Rust（使用官方脚本）
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
source $HOME/.cargo/env

# 或者通过包管理器安装（版本可能较旧）
# sudo dnf install -y rust cargo
```

验证安装：
```bash
rustc --version
cargo --version
```

## 7. 其他系统依赖

安装 OpenGL、SQLite 和其他必要库：

```bash
sudo dnf install -y mesa-libGL-devel mesa-libGLU-devel
sudo dnf install -y sqlite-devel
sudo dnf install -y libxkbcommon-devel
sudo dnf install -y libgomp
sudo dnf install -y libicu-devel
sudo dnf install -y double-conversion-devel
sudo dnf install -y sentencepiece-devel  # tokenizers-cpp 需要
```

## 8. 可选：无头运行环境（用于测试）

如果需要在不显示器的环境中运行（如服务器），安装虚拟显示和 VNC：

```bash
sudo dnf install -y xorg-x11-server-Xvfb
sudo dnf install -y x11vnc
sudo dnf install -y novnc websockify
```

## 9. 字体和本地化支持

安装中文字体和本地化工具：

```bash
sudo dnf install -y wqy-microhei-fonts google-noto-cjk-fonts
sudo dnf install -y glibc-langpack-zh
sudo dnf install google-noto-sans-cjk-ttc-fonts google-noto-serif-cjk-ttc-fonts
```


## 10. 项目构建步骤

完成所有依赖安装后，构建项目：

```bash
cd /path/to/qt-try

# 清理并创建构建目录
rm -rf build
mkdir build
cd build

# 配置 CMake（指定 ONNX Runtime 路径）
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DONNXRUNTIME_ROOT=/opt/onnxruntime

# 编译
cmake --build . -j$(nproc)

# 运行
./qt-try
```

## 11. Docker 构建（可选）

如果仍想使用 Docker 构建，确保 Docker 已安装：

```bash
sudo dnf install -y docker docker-compose
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
# 需要重新登录使组权限生效
```

构建 Docker 镜像：
```bash
./build.sh
```

运行：
```bash
./run.sh
```

## 12. 故障排除

### 问题：找不到 Qt6 库
```bash
# 确保 Qt6 开发包已安装
sudo dnf list installed | grep qt6

# 设置 Qt6 环境变量
export QT_SELECT=qt6
```

### 问题：CMake 找不到 Protobuf
```bash
# 确保开发包已安装
sudo dnf install -y protobuf-devel protobuf-compiler

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

echo "=== 检查完成 ==="
```

保存为 `check_deps.sh` 并运行：
```bash
chmod +x check_deps.sh
./check_deps.sh
```

## 14. 参考链接

- [Fedora 包管理器文档](https://docs.fedoraproject.org/en-US/quick-docs/dnf/)
- [Qt6 Fedora 安装指南](https://develop.kde.org/docs/use/qt6/fedora/)
- [ONNX Runtime 官方文档](https://onnxruntime.ai/docs/)
- [Rust 安装指南](https://www.rust-lang.org/tools/install)

---

**最后更新：2026-04-12**

*根据当前项目 CMakeLists.txt 和 Dockerfile 分析生成*
*适用于 Fedora 38+ 版本*