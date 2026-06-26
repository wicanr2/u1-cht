# Ultima I 原版逆向分析工具鏈(woz/ATR 抽檔 + Applesoft detokenize + 6502 反組譯)
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 python3-pip python3-venv pipx \
    default-jre-headless \
    cc65 \
    xxd unzip curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Python 影像處理(FM Towns/各平台 tile 解碼匯出 PNG)
RUN pip3 install --break-system-packages pillow || true

# AppleCommander:讀 Apple DOS 3.3 / ProDOS、Applesoft BASIC detokenize
RUN curl -fsSL -o /opt/ac.jar \
    https://github.com/AppleCommander/AppleCommander/releases/download/1-6-0/AppleCommander-ac-1.6.0.jar \
    || echo "WARN: AppleCommander 下載失敗(離線?)"

ENV PATH="/root/.local/bin:${PATH}"
WORKDIR /work
CMD ["/bin/bash"]
