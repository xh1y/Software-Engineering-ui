set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# 让 CMake 自动检测编译器
# dockcross 设置了 CC 和 CXX 环境变量

# ONNX Runtime 路径 (挂载到 /onnx)
set(ONNX_INCLUDE_DIR /onnx/include)
set(ONNX_LIB_DIR /onnx/lib)
