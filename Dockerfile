# 构建（含 Fast DDS）：docker compose build
# 运行（host 网络 + 共享内存，便于 DDS 发现）：docker compose up
#
# docker/vendor/fastdds3.tgz 由脚本生成（与主机 /usr/local/fastdds3 一致）：
#   mkdir -p docker/vendor && tar czf docker/vendor/fastdds3.tgz -C /usr/local fastdds3

FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config \
    qtbase5-dev libqt5sql5-psql \
    libssl-dev libtinyxml2-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY docker/vendor/fastdds3.tgz /tmp/fastdds3.tgz
RUN mkdir -p /usr/local && tar xzf /tmp/fastdds3.tgz -C /usr/local && rm /tmp/fastdds3.tgz

COPY . /src
WORKDIR /src/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DFASTDDS_ROOT=/usr/local/fastdds3 \
    && cmake --build . -j$(nproc)

# 运行镜像：与编译环境相同系列库，避免 OpenSSL/Qt 版本不一致
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libqt5core5a libqt5network5 libqt5sql5 libqt5sql5-psql libqt5widgets5 libqt5gui5 \
    libpq5 \
    libtinyxml2-9 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/fastdds3 /usr/local/fastdds3

WORKDIR /app
COPY --from=builder /src/bin/alarmsys /app/alarmsys
COPY --from=builder /src/bin/alarmevent_multi.xml /app/alarmevent_multi.xml
COPY --from=builder /src/bin/db.ini /app/db.ini
COPY --from=builder /src/bin/Config.ini /app/Config.ini

ENV LD_LIBRARY_PATH=/usr/local/fastdds3/lib
# Config.ini 中 TaskHostPort，默认与 Windows 包一致时可改为 8019 等
EXPOSE 8019
CMD ["./alarmsys"]
