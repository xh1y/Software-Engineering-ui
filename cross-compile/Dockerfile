# 基于 dockcross 构建，添加 protobuf 支持
FROM dockcross/windows-static-x64-posix:latest

# 切换到 root 安装依赖
USER root

# MXE 目录
ENV MXE_DIR=/usr/src/mxe
ENV MXE_TARGET=x86_64-w64-mingw32.static.posix

# 编译 protobuf for Windows
RUN cd $MXE_DIR && \
    make protobuf MXE_TARGETS=$MXE_TARGET JOBS=$(nproc)

# 设置 protobuf 环境变量
ENV Protobuf_ROOT=$MXE_DIR/usr/$MXE_TARGET
ENV Protobuf_INCLUDE_DIR=$MXE_DIR/usr/$MXE_TARGET/include
ENV Protobuf_LIBRARIES=$MXE_DIR/usr/$MXE_TARGET/lib/libprotobuf.a

USER root
WORKDIR /work
