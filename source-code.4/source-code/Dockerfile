# ================= 第一阶段：编译 =================
FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

# 1. 换源 + 装包 (只装核心，不装容易报错的碎片)
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    build-essential cmake git wget curl pkg-config \
    qt6-base-dev qt6-base-private-dev \
    libgl1-mesa-dev libsqlite3-dev \
    libprotobuf-dev protobuf-compiler \
    libxkbcommon-dev

# 2. Rust 环境 (使用中科大镜像，防止卡死)
ENV RUSTUP_DIST_SERVER=https://mirrors.ustc.edu.cn/rust-static
ENV RUSTUP_UPDATE_ROOT=https://mirrors.ustc.edu.cn/rust-static/rustup
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# 3. ONNX Runtime (假设你已经把 tgz 放在了当前目录)
WORKDIR /opt
COPY onnxruntime-linux-x64-1.24.4.tgz .
RUN tar -zxvf onnxruntime-linux-x64-1.24.4.tgz && \
    mv onnxruntime-linux-x64-1.24.4 onnxruntime && \
    rm onnxruntime-linux-x64-1.24.4.tgz

# 4. 编译
WORKDIR /app
COPY . .
RUN rm -rf build && cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build -j$(nproc)


# ================= 第二阶段：运行 =================
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    qt6-base-dev libgl1-mesa-glx libsqlite3-0 libprotobuf23 \
    libgomp1 libicu-dev libdouble-conversion3 \
    xvfb x11vnc novnc websockify \
    fonts-wqy-microhei fonts-noto-cjk \
    locales \
    && apt-get clean

# 2. 【关键】强制生成并设置中文 UTF-8 环境
RUN locale-gen zh_CN.UTF-8
ENV LANG=zh_CN.UTF-8
ENV LANGUAGE=zh_CN:zh
ENV LC_ALL=zh_CN.UTF-8

WORKDIR /root/
COPY --from=builder /app/build/qt-try ./optikg
COPY --from=builder /app/models ./model
COPY --from=builder /opt/onnxruntime/lib/*.so* /usr/lib/
COPY --from=builder /app/test ./test

RUN ln -s /usr/share/novnc/vnc.html /usr/share/novnc/index.html

RUN echo '#!/bin/bash\n\
Xvfb :99 -screen 0 1280x800x24 &\n\
export DISPLAY=:99\n\
x11vnc -display :99 -forever -nopw -listen localhost &\n\
websockify --web=/usr/share/novnc/ 8080 localhost:5900 &\n\
./optikg' > /root/entrypoint.sh && chmod +x /root/entrypoint.sh

EXPOSE 8080
ENTRYPOINT ["/root/entrypoint.sh"]