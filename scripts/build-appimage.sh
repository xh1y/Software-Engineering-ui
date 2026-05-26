#!/bin/bash
set -e

# ============================================================
# OptiKG AppImage 打包脚本
# 功能：将编译产物打包成 AppImage 可执行文件
# 位置：scripts/build-appimage.sh （项目根目录的 scripts/ 子目录下）
# 用法：cd 到项目根目录后执行 ./scripts/build-appimage.sh
# ============================================================

# 项目根目录（脚本位于 scripts/ 子目录，上级即为项目根目录）
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# 各目录/工具路径
BUILD_DIR="${PROJECT_ROOT}/build-release"             # Release 编译输出目录
APPDIR="${PROJECT_ROOT}/AppDir"                       # AppImage 打包中间目录
OUTPUT_NAME="OptiKG-1.0.0-x86_64.AppImage"            # 最终输出文件名

# AppImage 打包工具（存放在 tools/ 目录下）
LINUXDEPLOY="${PROJECT_ROOT}/tools/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="${PROJECT_ROOT}/tools/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="${PROJECT_ROOT}/tools/appimagetool-x86_64.AppImage"

# --- 检查打包工具是否存在 ---
for tool in "$LINUXDEPLOY" "$LINUXDEPLOY_QT" "$APPIMAGETOOL"; do
    if [ ! -f "$tool" ]; then
        echo "错误: 缺少工具 $(basename "$tool")"
        echo "请确保以下文件存在于 tools/ 目录:"
        echo "  - tools/linuxdeploy-x86_64.AppImage"
        echo "  - tools/linuxdeploy-plugin-qt-x86_64.AppImage"
        echo "  - tools/appimagetool-x86_64.AppImage"
        exit 1
    fi
done

# --- 检查编译产物是否存在 ---
if [ ! -f "$BUILD_DIR/qt-try" ]; then
    echo "错误: 未找到编译产物 $BUILD_DIR/qt-try"
    echo "请先编译 Release 版本:"
    echo "  cmake -B build-release -DCMAKE_BUILD_TYPE=Release"
    echo "  cmake --build build-release -j$(nproc)"
    exit 1
fi

echo "==> 清理旧的 AppDir..."
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/bin/models"
mkdir -p "$APPDIR/usr/bin/i18n"

# --- 复制可执行文件 ---
echo "==> 复制可执行文件..."
cp "$BUILD_DIR/qt-try" "$APPDIR/usr/bin/optikg"
chmod +x "$APPDIR/usr/bin/optikg"

# --- 复制 .desktop 启动器文件 ---
echo "==> 复制 .desktop 文件..."
cp "$PROJECT_ROOT/resources/optikg.desktop" "$APPDIR/usr/share/applications/optikg.desktop"
cp "$PROJECT_ROOT/resources/optikg.desktop" "$APPDIR/optikg.desktop"

# --- 复制应用图标 ---
echo "==> 复制图标..."
if [ -f "$PROJECT_ROOT/resources/icons/optikg.png" ]; then
    cp "$PROJECT_ROOT/resources/icons/optikg.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/optikg.png"
    cp "$PROJECT_ROOT/resources/icons/optikg.png" "$APPDIR/.DirIcon"
    cp "$PROJECT_ROOT/resources/icons/optikg.png" "$APPDIR/optikg.png"
else
    echo "警告: 未找到图标 resources/icons/optikg.png，将使用默认图标"
fi

# --- 复制 AI 模型文件 ---
echo "==> 复制模型文件..."
if [ -d "$PROJECT_ROOT/models" ]; then
    cp -r "$PROJECT_ROOT/models/"* "$APPDIR/usr/bin/models/"
else
    echo "警告: 未找到模型目录 models/"
fi

# --- 复制国际化翻译文件 ---
echo "==> 复制国际化文件..."
if [ -f "$PROJECT_ROOT/qt-try_en.qm" ]; then
    cp "$PROJECT_ROOT/qt-try_en.qm" "$APPDIR/usr/bin/i18n/"
else
    echo "警告: 未找到 qt-try_en.qm，请先运行 lrelease-qt6 qt-try_en.ts -qm qt-try_en.qm"
fi

# --- 使用 linuxdeploy 部署系统库依赖 ---
echo "==> 运行 linuxdeploy 部署基础依赖..."
cd "$PROJECT_ROOT"
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/optikg" \
    --desktop-file "$APPDIR/usr/share/applications/optikg.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/optikg.png" || true

# --- 使用 linuxdeploy-plugin-qt 部署 Qt 插件 ---
echo "==> 运行 linuxdeploy-plugin-qt 部署 Qt 插件..."
export QMAKE="$(which qmake6 2>/dev/null || which qmake-qt6 2>/dev/null || which qmake 2>/dev/null)"
export LINUXDEPLOY_PLUGIN_QT="$LINUXDEPLOY_QT"
"$LINUXDEPLOY_QT" --appdir "$APPDIR" || true

# --- 创建 AppRun 启动入口脚本 ---
echo "==> 创建 AppRun 启动脚本..."
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
APPDIR="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$APPDIR/usr/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$APPDIR/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$APPDIR/usr/plugins/platforms"
export PATH="$APPDIR/usr/bin:$PATH"
exec "$APPDIR/usr/bin/optikg" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# --- 最终打包 ---
echo "==> 打包 AppImage..."
"$APPIMAGETOOL" "$APPDIR" "$OUTPUT_NAME"

echo "==> 完成! 输出文件: $PROJECT_ROOT/$OUTPUT_NAME"
ls -lh "$PROJECT_ROOT/$OUTPUT_NAME"
