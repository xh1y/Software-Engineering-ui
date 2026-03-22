set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MXE 编译器
set(CMAKE_C_COMPILER /usr/lib/mxe/usr/bin/x86_64-w64-mingw32.static-gcc)
set(CMAKE_CXX_COMPILER /usr/lib/mxe/usr/bin/x86_64-w64-mingw32.static-g++)
set(CMAKE_RC_COMPILER /usr/lib/mxe/usr/bin/x86_64-w64-mingw32.static-windres)

# 查找路径
set(CMAKE_FIND_ROOT_PATH /usr/lib/mxe/usr/x86_64-w64-mingw32.static)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Qt6 路径
set(Qt6_DIR /usr/lib/mxe/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6)
