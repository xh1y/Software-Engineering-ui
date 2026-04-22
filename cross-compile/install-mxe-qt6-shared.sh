#!/bin/bash
# 编译安装 MXE 共享版 Qt6
# 你自己运行这个脚本，耗时约 30-60 分钟

set -e

MXE_DIR="$HOME/mxe"

echo "🚀 编译 MXE 共享版 Qt6"
echo "================================"
echo "预计时间：30-60 分钟（取决于电脑性能）"
echo ""
echo "按 Enter 开始编译，或 Ctrl+C 取消..."
read

# 进入 MXE 目录
cd "$MXE_DIR"

echo "🔨 开始编译..."
echo "（你可以去干别的了，编译完成后会提示）"
echo ""

# 编译共享版 Qt6
make qt6 MXE_TARGETS=x86_64-w64-mingw32.shared -j$(nproc)

echo ""
echo "================================"
echo "🎉 编译完成！"
echo ""
echo "现在可以编译你的 Windows 程序了："
echo "  cd cross-compile"
echo "  ./build-with-mxe-shared.sh"
