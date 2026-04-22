# OptiKG Docker 构建文件
# 在 Linux 容器中构建并运行 Qt6 + ONNX Runtime 应用

# ==================== 第一阶段：构建 ====================
FROM ubuntu:22.04 AS builder

# 设置环境变量，避免交互式安装
ENV DEBIAN_FRONTEND=noninteractive

# 安装系统依赖和构建工具
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    ca-certificates \
    libprotobuf-dev \
    protobuf-compiler \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qt6-l10n-tools \
    libqt6sql6-sqlite \
    qt6-base-private-dev \
    libqt6concurrent6 \
    libxcb-xinerama0 \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# 安装 Rust（用于 tokenizers-cpp）
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# 下载 ONNX Runtime Linux 版本 (v1.16.3)
RUN mkdir -p /opt/onnxruntime && cd /opt/onnxruntime \
    && wget -q https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz \
    && tar -xzf onnxruntime-linux-x64-1.16.3.tgz \
    && mv onnxruntime-linux-x64-1.16.3 onnxruntime \
    && rm onnxruntime-linux-x64-1.16.3.tgz

# 复制项目代码
WORKDIR /app
COPY ../ ./

# 初始化子模块（关键！）
RUN git submodule update --init --recursive

# 创建构建目录并编译
RUN mkdir build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release \
             -DONNX_INCLUDE_DIR=/opt/onnxruntime/onnxruntime/include \
             -DONNXRUNTIME_LIB=/opt/onnxruntime/onnxruntime/lib/libonnxruntime.so.1.16.3 \
             .. \
    && make -j$(nproc)

# ==================== 第二阶段：运行环境 ====================
FROM ubuntu:22.04

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libprotobuf23 \
    libgomp1 \
    libxcb1 \
    libxcb-xinerama0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-sync1 \
    libxcb-xfixes0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    libxkbcommon0 \
    libfontconfig1 \
    libfreetype6 \
    libgl1 \
    libglib2.0-0 \
    libsm6 \
    libx11-6 \
    libx11-xcb1 \
    libxext6 \
    libxrender1 \
    libice6 \
    libegl1 \
    qt6-base \
    libqt6concurrent6 \
    libqt6sql6-sqlite \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 从构建阶段复制文件
COPY --from=builder /app/build/qt-try .
COPY --from=builder /app/model ./model/
COPY --from=builder /opt/onnxruntime/onnxruntime/lib/libonnxruntime.so.1.16.3 /usr/lib/
COPY --from=builder /app/3rdparty/tokenizers-cpp/target/release/libtokenizers_cpp.so /usr/lib/

# 设置库路径和链接
RUN ldconfig && \
    ln -sf /usr/lib/libonnxruntime.so.1.16.3 /usr/lib/libonnxruntime.so && \
    ln -sf /usr/lib/libtokenizers_cpp.so /usr/lib/libtokenizers_cpp.so.0

# 设置环境变量
ENV LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH
ENV QT_QPA_PLATFORM=xcb
ENV QT_DEBUG_PLUGINS=0

# 创建非 root 用户（安全考虑）
RUN groupadd -r appuser && useradd -r -g appuser -G video appuser \
    && mkdir -p /home/appuser && chown -R appuser:appuser /home/appuser /app
USER appuser

# 启动命令
CMD ["./qt-try"]