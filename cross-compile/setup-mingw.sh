#!/bin/bash
# 安装 MinGW-w64 交叉编译工具链

set -e

echo "🔧 安装 MinGW-w64 交叉编译工具链"
echo "================================"

# Arch Linux
if command -v pacman &> /dev/null; then
    echo "检测到 Arch Linux"
    # 只安装基础工具链（官方仓库有）
    sudo pacman -S --needed mingw-w64-gcc mingw-w64-binutils mingw-w64-headers mingw-w64-crt
    
    echo ""
    echo "⚠️  MinGW Qt6 需要从 AUR 安装，耗时 2-4 小时"
    echo "推荐使用 Docker 方案："
    echo "  1. docker pull a12e/docker-qt6-windows:6.5-mingw-w64-ltsc2022"
    echo "  2. ./build-windows.sh"
    echo ""
    echo "或者使用 GitHub Actions 云端编译"
    
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
echo "✅ MinGW-w64 基础工具链安装完成！"
echo ""
echo "测试编译器:"
x86_64-w64-mingw32-g++ --version | head -1
