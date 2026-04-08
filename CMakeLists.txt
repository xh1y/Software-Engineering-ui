cmake_minimum_required(VERSION 3.16)
project(qt-try LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# 平台检测和配置
# ==============================================================================
if(WIN32)
    message(STATUS "Building for Windows")
    add_definitions(-D_WIN32)
elseif(APPLE)
    message(STATUS "Building for macOS")
    add_definitions(-D__APPLE__)
elseif(UNIX)
    message(STATUS "Building for Linux/Unix")
    add_definitions(-D__linux__)
endif()

# ==============================================================================
# 1. 【核心防御】统一 Protobuf 战线
# ==============================================================================
if(WIN32)
    # Windows: 使用内置 Protobuf（避免 MinGW 兼容性问题）
    message(STATUS "Windows detected: using builtin protobuf for tokenizers-cpp")
    set(SPM_USE_BUILTIN_PROTOBUF ON CACHE BOOL "Use builtin protobuf" FORCE)
else()
    # Linux/macOS: 使用系统 Protobuf
    find_package(Protobuf REQUIRED)
    message(STATUS "Found System Protobuf: ${Protobuf_LIBRARIES}")
    set(SPM_USE_BUILTIN_PROTOBUF OFF CACHE BOOL "Force use system protobuf" FORCE)
    set(SPM_ENABLE_PROTOBUF_SHARED ON CACHE BOOL "Use shared protobuf" FORCE)
    set(protobuf_MODULE_COMPATIBLE ON CACHE BOOL "" FORCE)
endif()

# ==============================================================================
# 2. 查找 Qt (补回了 Sql 模块！！)
# ==============================================================================
# 修复：加上了 Sql，这样才能找到 QSqlDatabase
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Sql Concurrent LinguistTools)

# ==============================================================================
# 3. 查找 ONNX Runtime (跨平台支持)
# ==============================================================================
# Windows平台：使用环境变量或默认安装路径
if(WIN32)
    # 查找头文件
    find_path(ONNX_INCLUDE_DIR NAMES onnxruntime_cxx_api.h
        PATHS "$ENV{ONNXRUNTIME_HOME}/include"
              "C:/Program Files/onnxruntime/include"
              "C:/onnxruntime/include"
              "$ENV{ProgramFiles}/onnxruntime/include")

    # 查找库文件
    find_library(ONNXRUNTIME_LIB NAMES onnxruntime
        PATHS "$ENV{ONNXRUNTIME_HOME}/lib"
              "C:/Program Files/onnxruntime/lib"
              "C:/onnxruntime/lib"
              "$ENV{ProgramFiles}/onnxruntime/lib")

    if(NOT ONNXRUNTIME_LIB)
        set(ONNXRUNTIME_LIB "onnxruntime")
    endif()

    message(STATUS "Windows build - ONNX Runtime library: ${ONNXRUNTIME_LIB}")
# Linux/macOS平台
else()
    set(ONNXRUNTIME_LIB "onnxruntime")
    find_path(ONNX_INCLUDE_DIR NAMES onnxruntime_cxx_api.h
        PATHS /usr/include/onnxruntime /usr/local/include/onnxruntime /opt/homebrew/include/onnxruntime)
endif()

if (ONNX_INCLUDE_DIR)
    message(STATUS "Found ONNX Runtime headers at: ${ONNX_INCLUDE_DIR}")
else ()
    message(WARNING "Could not find onnxruntime_cxx_api.h")
endif ()

# ==============================================================================
# 4. 添加子模块 (tokenizers-cpp)
# ==============================================================================
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tokenizers-cpp/CMakeLists.txt")
    add_subdirectory(3rdparty/tokenizers-cpp)
else ()
    message(FATAL_ERROR "tokenizers-cpp submodule not found! Run: git submodule update --init --recursive")
endif ()

# qt-material 样式库（可选） - 第一部分：检测和添加子目录
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qt-material/CMakeLists.txt")
    # 设置qt-material使用Qt6版本
    set(QT_MATERIAL_WIDGETS_QT_VERSION 6 CACHE STRING "Qt version to use [default: 5]" FORCE)
    add_subdirectory(3rdparty/qt-material)
    add_compile_definitions(WITH_MATERIAL_WIDGETS)
    set(HAVE_QT_MATERIAL TRUE)
else()
    message(STATUS "qt-material not found, using built-in QSS styles")
    set(HAVE_QT_MATERIAL FALSE)
endif()



# ==============================================================================
# 5. 定义主程序
# ==============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE SOURCES
        "src/*.cpp"
        "src/*.h"
        "src/main.cpp"
)

# ==============================================================================
# 翻译文件处理
# ==============================================================================
set(TS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/qt-try_en.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/qt-try_zh_CN.ts"
)

set(QM_FILES)
foreach(TS_FILE ${TS_FILES})
    get_filename_component(TS_NAME ${TS_FILE} NAME_WE)
    set(QM_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TS_NAME}.qm")
    list(APPEND QM_FILES ${QM_FILE})
    add_custom_command(
        OUTPUT ${QM_FILE}
        COMMAND Qt6::lrelease
        ARGS ${TS_FILE} -qm ${QM_FILE}
        DEPENDS ${TS_FILE}
        COMMENT "Generating ${QM_FILE}"
    )
endforeach()

add_custom_target(translations ALL DEPENDS ${QM_FILES})

# 生成 i18n.qrc，将 .qm 文件嵌入资源
set(I18N_QRC_CONTENT "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<RCC version=\"1.0\">\n  <qresource prefix=\"/i18n\">\n")
foreach(TS_FILE ${TS_FILES})
    get_filename_component(TS_NAME ${TS_FILE} NAME_WE)
    string(APPEND I18N_QRC_CONTENT "    <file>${TS_NAME}.qm</file>\n")
endforeach()
string(APPEND I18N_QRC_CONTENT "  </qresource>\n</RCC>\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc" "${I18N_QRC_CONTENT}")

# 资源文件
set(RESOURCE_FILES
        "resources/resources.qrc"
        "${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc"
)

add_executable(qt-try ${SOURCES} ${RESOURCE_FILES})
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc" PROPERTIES GENERATED TRUE)
add_dependencies(qt-try translations)

# ==============================================================================
# 6. 【关键】链接库与包含头文件
# ==============================================================================
target_link_libraries(qt-try PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
        Qt6::Sql                  # 修复：必须在这里链接 Sql
        Qt6::Concurrent           # 用于并行处理
        ${ONNXRUNTIME_LIB}
        tokenizers_cpp
)

if(NOT WIN32)
    target_link_libraries(qt-try PRIVATE ${Protobuf_LIBRARIES})
endif()

# 如果使用了qt-material，链接库
if(HAVE_QT_MATERIAL)
    target_link_libraries(qt-try PRIVATE
            qmw::static
    )
endif()

# 修复：补全了所有头文件路径，包括 ONNX
target_include_directories(qt-try PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "src"
        "src/core"
        "src/data"
        "src/ui"
        "src/utils"
        "include"
        ${Protobuf_INCLUDE_DIRS}
        ${ONNX_INCLUDE_DIR}       # 修复：包含了 ONNX 的头文件路径
)

# 如果使用了qt-material，添加包含目录
if(HAVE_QT_MATERIAL)
    target_include_directories(qt-try PRIVATE
            3rdparty/qt-material/include
    )
endif()

# ==============================================================================
# 7. 自动拷贝模型文件到构建目录
# ==============================================================================
set(MODEL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Software-Engineering/model/model_optimized")
set(DEST_DIR "${CMAKE_BINARY_DIR}/model")

add_custom_command(TARGET qt-try POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MODEL_SRC_DIR} ${DEST_DIR}
        COMMENT "Copying model files from backend to build directory..."
)