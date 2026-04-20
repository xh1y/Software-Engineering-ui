#!/bin/bash
# 使用 MXE 编译 MinGW Qt6
# MXE = M cross environment，专门做 MinGW 交叉编译的

set -e

echo "🚀 使用 MXE 编译 MinGW Qt6"
echo "================================"
echo "预计时间：1-2 小时"
echo ""

MXE_DIR="$HOME/mxe"

# 安装基础依赖
echo "📦 安装编译依赖..."
sudo pacman -S --needed gcc make cmake p7zip wget gperf bison flex

# 克隆 MXE
if [ ! -d "$MXE_DIR" ]; then
    echo "⬇️ 下载 MXE..."
    git clone https://github.com/mxe/mxe.git "$MXE_DIR"
fi

cd "$MXE_DIR"

# 编译 Qt6（只编译静态版本，减少问题）
echo "🔨 开始编译 Qt6..."
echo "（这可能需要 1-2 小时，去喝杯咖啡吧 ☕）"
make qt6 MXE_TARGETS=x86_64-w64-mingw32.static -j$(nproc)

# 添加到环境变量
echo ""
echo "📝 配置环境变量..."
if ! grep -q "mxe/usr/bin" "$HOME/.bashrc"; then
    echo "export PATH=\"$MXE_DIR/usr/bin:\$PATH\"" >> "$HOME/.bashrc"
fi

echo ""
echo "================================"
echo "🎉 编译完成！"
echo ""
echo "使用方式："
echo "  1. 重新打开终端，或运行: export PATH=\"$MXE_DIR/usr/bin:\$PATH\""
echo "  2. 编译器: x86_64-w64-mingw32.static-g++"
echo "  3. Qt6 在: $MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/"
