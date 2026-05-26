# OptiKG 单元测试 CMake 配置
cmake_minimum_required(VERSION 3.16)

enable_testing()

# 收集被测源码（排除 main.cpp）
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/*.h"
    "${CMAKE_SOURCE_DIR}/src/data/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/data/*.h"
    "${CMAKE_SOURCE_DIR}/src/utils/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/*.h"
    "${CMAKE_SOURCE_DIR}/src/ui/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/*.h"
)

# 测试文件列表
set(TEST_SOURCES
    test_datamodel.cpp
    test_configmanager.cpp
    test_databasemanager.cpp
    test_inferenceengine.cpp
    test_graphwidget.cpp
    test_batchprocessor.cpp
    test_mainwindow.cpp
    test_batchprocessdialog.cpp
    test_performance.cpp
    test_reliability.cpp
    test_appconstants.cpp
    test_stylemanager.cpp
    test_settingsdialog.cpp
    test_resulttreewidget.cpp
    test_inferenceworker.cpp
)

foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name}
        ${test_source}
        ${SRC_FILES}
    )
    target_link_libraries(${test_name} PRIVATE
        Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Sql Qt6::Concurrent Qt6::Test
        ${ONNXRUNTIME_LIB}
        tokenizers_cpp
        ${Protobuf_LIBRARIES}
    )
    if(HAVE_QT_MATERIAL)
        target_link_libraries(${test_name} PRIVATE qmw::static)
    endif()
    target_include_directories(${test_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/core"
        "${CMAKE_SOURCE_DIR}/src/data"
        "${CMAKE_SOURCE_DIR}/src/ui"
        "${CMAKE_SOURCE_DIR}/src/utils"
        "${CMAKE_SOURCE_DIR}/include"
        ${Protobuf_INCLUDE_DIRS}
        ${ONNX_INCLUDE_DIR}
    )
    # 模型拷贝
    add_custom_command(TARGET ${test_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MODEL_SRC_DIR} $<TARGET_FILE_DIR:${test_name}>/models
    )
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
endforeach()
