#!/bin/bash
# 使用 MXE 共享库版本编译 Windows 版本

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build-mxe-shared"
MXE_DIR="$HOME/mxe"
ONNX_VERSION="1.16.3"
ONNX_DIR="$PROJECT_ROOT/3rdparty/onnxruntime-win/onnxruntime-win-x64-${ONNX_VERSION}"

echo "🔨 使用 MXE 共享库编译 Windows 版本"
echo "================================"

# 检查 MXE 共享版
if [ ! -f "$MXE_DIR/usr/bin/x86_64-w64-mingw32.shared-g++" ]; then
    echo "❌ MXE 共享版未找到"
    echo "请先运行: cd ~/mxe && make qt6 MXE_TARGETS=x86_64-w64-mingw32.shared"
    exit 1
fi

# 检查 ONNX
if [ ! -d "$ONNX_DIR" ]; then
    echo "📦 下载 ONNX Runtime..."
    ./setup-cross-build.sh
fi

# 设置 PATH
export PATH="$MXE_DIR/usr/bin:$PATH"

echo "✅ 使用编译器: $(x86_64-w64-mingw32.shared-g++ --version | head -1)"

# 清理旧构建
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# 创建工具链文件
cat > "$BUILD_DIR/toolchain.cmake" << EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32.shared-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32.shared-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32.shared-windres)
set(CMAKE_FIND_ROOT_PATH $MXE_DIR/usr/x86_64-w64-mingw32.shared)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_PREFIX_PATH $MXE_DIR/usr/x86_64-w64-mingw32.shared/qt6/lib/cmake)
EOF

echo "🔧 配置 CMake..."
cd "$BUILD_DIR"

cmake "$PROJECT_ROOT" \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DONNX_INCLUDE_DIR="$ONNX_DIR/include" \
    -DONNXRUNTIME_LIB="$ONNX_DIR/lib/onnxruntime.dll" \
    -DCMAKE_PREFIX_PATH="$MXE_DIR/usr/x86_64-w64-mingw32.shared/qt6/lib/cmake" \
    -DBUILD_SHARED_LIBS=ON

echo "🔨 开始编译..."
make -j$(nproc)

echo ""
echo "================================"
if [ -f "$BUILD_DIR/qt-try.exe" ]; then
    echo "✅ 构建成功！"
    
    # 收集 DLL
    mkdir -p "$BUILD_DIR/package"
    cp "$BUILD_DIR/qt-try.exe" "$BUILD_DIR/package/"
    
    # 复制 Qt DLL
    cp $MXE_DIR/usr/x86_64-w64-mingw32.shared/qt6/bin/*.dll "$BUILD_DIR/package/" 2>/dev/null || true
    
    # 复制 MXE 依赖 DLL
    cp $MXE_DIR/usr/x86_64-w64-mingw32.shared/bin/*.dll "$BUILD_DIR/package/" 2>/dev/null || true
    
    # 复制 ONNX
    cp "$ONNX_DIR/lib/onnxruntime.dll" "$BUILD_DIR/package/" 2>/dev/null || true
    
    echo "📁 输出目录: $BUILD_DIR/package/"
    ls -lh "$BUILD_DIR/package/"
else
    echo "❌ 构建失败"
    exit 1
fi
