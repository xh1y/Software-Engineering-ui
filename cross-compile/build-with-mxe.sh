#!/bin/bash
# 使用 MXE 编译 Windows 版本

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build-mxe"
MXE_DIR="$HOME/mxe"
ONNX_VERSION="1.16.3"
ONNX_DIR="$PROJECT_ROOT/3rdparty/onnxruntime-win/onnxruntime-win-x64-${ONNX_VERSION}"

echo "🔨 使用 MXE 编译 Windows 版本"
echo "================================"

# 检查 MXE
if [ ! -d "$MXE_DIR" ]; then
    echo "❌ MXE 未找到，请先运行: ./build-mxe-qt6.sh"
    exit 1
fi

# 检查 ONNX
if [ ! -d "$ONNX_DIR" ]; then
    echo "📦 下载 ONNX Runtime..."
    ./setup-cross-build.sh
fi

# 设置 PATH
export PATH="$MXE_DIR/usr/bin:$PATH"

# 检查编译器
if ! command -v x86_64-w64-mingw32.static-g++ &> /dev/null; then
    echo "❌ 找不到 MXE 编译器"
    echo "请运行: export PATH=\"$MXE_DIR/usr/bin:\$PATH\""
    exit 1
fi

echo "✅ 使用编译器: $(x86_64-w64-mingw32.static-g++ --version | head -1)"

# 清理旧构建
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# 创建工具链文件
cat > "$BUILD_DIR/toolchain.cmake" << EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32.static-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32.static-g++)
set(CMAKE_FIND_ROOT_PATH $MXE_DIR/usr/x86_64-w64-mingw32.static)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_PREFIX_PATH $MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake)

# 禁用有问题的系统库查找
set(CMAKE_DISABLE_FIND_PACKAGE_TIFF TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_WebP TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_MySQL TRUE)
EOF

echo "🔧 配置 CMake..."
cd "$BUILD_DIR"

# 设置 Qt6 路径
export Qt6_DIR="$MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6"
export Qt6Core_DIR="$MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6Core"
export Qt6Widgets_DIR="$MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6Widgets"
export Qt6Gui_DIR="$MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6Gui"
export Qt6Sql_DIR="$MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6Sql"
export Qt6Concurrent_DIR="$MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6Concurrent"
export Qt6LinguistTools_DIR="$MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6LinguistTools"

# 禁用有问题的插件
cmake "$PROJECT_ROOT" \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DONNX_INCLUDE_DIR="$ONNX_DIR/include" \
    -DONNXRUNTIME_LIB="$ONNX_DIR/lib/onnxruntime.lib" \
    -DCMAKE_PREFIX_PATH="$MXE_DIR/usr/x86_64-w64-mingw32.static/qt6/lib/cmake" \
    -DBUILD_SHARED_LIBS=OFF

echo "🔨 开始编译..."
make -j$(nproc)

echo ""
echo "================================"
if [ -f "$BUILD_DIR/qt-try.exe" ]; then
    echo "✅ 构建成功！"
    echo "📁 输出: $BUILD_DIR/qt-try.exe"
else
    echo "❌ 构建失败"
    exit 1
fi
