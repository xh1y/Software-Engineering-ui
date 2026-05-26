cmake_minimum_required(VERSION 3.16)
project(qt-try LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- 1. 基础依赖 ---
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Sql Concurrent Test)
find_package(Protobuf REQUIRED)

# --- 2. ONNX Runtime (Docker 路径) ---
set(ONNXRUNTIME_LIB "onnxruntime")
find_path(ONNX_INCLUDE_DIR NAMES onnxruntime_cxx_api.h PATHS /opt/onnxruntime/include)
link_directories(/opt/onnxruntime/lib)

# --- 3. 引入子模块 ---
# tokenizers-cpp 必须在 3rdparty/tokenizers-cpp
add_subdirectory(3rdparty/tokenizers-cpp)

# 如果你有 qt-material 就留着，没有就注释掉
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qt-material/CMakeLists.txt")
    set(QT_MATERIAL_WIDGETS_QT_VERSION 6 CACHE STRING "" FORCE)
    add_subdirectory(3rdparty/qt-material)
    add_compile_definitions(WITH_MATERIAL_WIDGETS)
    set(HAVE_QT_MATERIAL TRUE)
endif()

# --- 4. 源码搜集 ---
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/main.cpp")

# --- 5. 编译目标 (彻底删掉翻译相关的 i18n.qrc) ---
add_executable(qt-try ${SOURCES} "resources/resources.qrc")

# --- 6. 链接 ---
target_link_libraries(qt-try PRIVATE
        Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Sql Qt6::Concurrent
        ${ONNXRUNTIME_LIB}
        tokenizers_cpp
        ${Protobuf_LIBRARIES}
)

if(HAVE_QT_MATERIAL)
    target_link_libraries(qt-try PRIVATE qmw::static)
endif()

target_include_directories(qt-try PRIVATE
        "src" "src/core" "src/data" "src/ui" "src/utils" "include"
        ${Protobuf_INCLUDE_DIRS} ${ONNX_INCLUDE_DIR}
)

# --- 7. 测试 (如果在主构建中) ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

# --- 8. 模型拷贝 ---
set(MODEL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/models")
set(DEST_DIR "${CMAKE_BINARY_DIR}/models")
add_custom_command(TARGET qt-try POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MODEL_SRC_DIR} ${DEST_DIR}
)