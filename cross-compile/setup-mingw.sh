#!/bin/bash
# 安装 MinGW-w64 交叉编译工具链

set -e

echo "🔧 安装 MinGW-w64 交叉编译工具链"
echo "================================"

# Arch Linux
if command -v pacman &> /dev/null; then
    echo "检测到 Arch Linux"
    sudo pacman -S --needed mingw-w64-gcc mingw-w64-cmake mingw-w64-qt6-base mingw-w64-qt6-tools mingw-w64-protobuf
    
# Ubuntu/Debian
elif command -v apt &> /dev/null; then
    echo "检测到 Ubuntu/Debian"
    sudo apt update
    sudo apt install -y mingw-w64 mingw-w64-tools cmake
    echo "⚠️ Ubuntu 的 MinGW Qt6 支持不完善，建议用 Docker"
    
else
    echo "❌ 不支持的发行版"
    exit 1
fi

echo ""
echo "✅ MinGW-w64 安装完成！"
echo ""
echo "测试编译器:"
x86_64-w64-mingw32-g++ --version | head -1
