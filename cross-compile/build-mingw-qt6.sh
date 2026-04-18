#!/bin/bash
# 在 Arch Linux 上从 AUR 编译 MinGW-w64 Qt6
# 耗时：2-4 小时，取决于电脑性能

set -e

echo "🚀 开始编译 MinGW-w64 Qt6"
echo "================================"
echo "⚠️  这将花费 2-4 小时，请耐心等待"
echo ""

# 检查是否有 AUR helper
if command -v yay &> /dev/null; then
    AUR_HELPER="yay"
elif command -v paru &> /dev/null; then
    AUR_HELPER="paru"
else
    echo "未检测到 AUR helper (yay/paru)，将使用手动方式..."
    AUR_HELPER=""
fi

# 安装依赖
echo "📦 安装编译依赖..."
sudo pacman -S --needed base-devel git cmake ninja python perl gcc

if [ -n "$AUR_HELPER" ]; then
    echo "使用 $AUR_HELPER 安装 MinGW Qt6..."
    
    # 按顺序安装（有依赖关系）
    echo "1/5 安装 mingw-w64-qt6-base..."
    $AUR_HELPER -S --needed mingw-w64-qt6-base
    
    echo "2/5 安装 mingw-w64-qt6-tools..."
    $AUR_HELPER -S --needed mingw-w64-qt6-tools || echo "跳过，可能已安装"
    
    echo "3/5 安装 mingw-w64-protobuf..."
    $AUR_HELPER -S --needed mingw-w64-protobuf || echo "跳过"
    
else
    echo "手动从 AUR 编译..."
    
    # 创建临时目录
    BUILD_DIR="$HOME/.cache/mingw-qt6-build"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # 安装基础包
    install_pkg() {
        local pkg=$1
        echo ""
        echo "📦 安装 $pkg..."
        if [ ! -d "$pkg" ]; then
            git clone "https://aur.archlinux.org/$pkg.git"
        fi
        cd "$pkg"
        makepkg -si --needed --noconfirm
        cd ..
    }


    # 按依赖顺序安装
    install_pkg "mingw-w64-qt6-base"
    install_pkg "mingw-w64-qt6-tools"
    
    echo ""
    echo "✅ AUR 包安装完成！"
fi

echo ""
echo "================================"
echo "🎉 MinGW Qt6 编译完成！"
echo ""
echo "现在可以编译 Windows 程序了："
echo "  cd cross-compile"
echo "  ./setup-cross-build.sh  # 如果还没运行过"
echo "  ./build-windows-nodocker.sh"
