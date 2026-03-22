@echo off
chcp 65001 >nul
echo 🐳 Windows 本地编译脚本
echo ================================

:: 设置 Qt 路径（根据你的安装位置修改）
set QT_PATH=C:\Qt\6.5.3\mingw_64
set PATH=%QT_PATH%\bin;%PATH%

:: 设置 MinGW 路径
set MINGW_PATH=%QT_PATH%\bin

:: 创建构建目录
if exist build-windows rmdir /s /q build-windows
mkdir build-windows
cd build-windows

echo 🔧 配置 CMake...
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
    -DONNX_INCLUDE_DIR="C:\onnx\include" ^
    -DONNXRUNTIME_LIB="C:\onnx\lib\onnxruntime.lib"

echo 🔨 编译...
mingw32-make -j%NUMBER_OF_PROCESSORS%

echo.
echo ================================
if exist qt-try.exe (
    echo ✅ 构建成功！
    echo 📁 qt-try.exe
) else (
    echo ❌ 构建失败
)
pause
