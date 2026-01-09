# ---------- Build stage (compile inside the container) ----------
FROM ticketing-apps:latest AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Build dependencies (only in builder stage)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    ca-certificates \
    git \
    libssl-dev \
    libpaho-mqtt-dev \
    libpaho-mqttpp-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /Workspace

# Copy source
COPY . .

# Build once at image build time
RUN chmod +x /Workspace/Project/Entry_Point.sh \
 && cd /Workspace/Project \
 && make all


# ---------- Runtime stage (run broker + apps) ----------
FROM ticketing-apps:latest

ENV DEBIAN_FRONTEND=noninteractive

# Runtime dependencies only
# - mosquitto: broker
# - coreutils: provides `timeout` used by Entry_Point.sh
# - libssl3 / paho runtime libs: for your compiled binaries (common on Ubuntu 22.04)
RUN apt-get update && apt-get install -y --no-install-recommends \
    mosquitto \
    coreutils \
    ca-certificates \
    libssl3 \
    libpaho-mqtt1.3 \
    libpaho-mqttpp3-1 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /Workspace

# Copy built workspace (including compiled binaries) from builder
COPY --from=builder /Workspace /Workspace

# Broker config
COPY mosquitto/mosquitto.conf /etc/mosquitto/mosquitto.conf

# ---- Patch Entry_Point.sh to skip rebuilding on every container start ----
# It will now:
#   - build only if SKIP_BUILD != 1
# Default: SKIP_BUILD=1 (fast startup)
RUN chmod +x /Workspace/Project/Entry_Point.sh \
 && perl -0777 -i -pe 's/\nmake all\n/\nif [[ "${SKIP_BUILD:-0}" != "1" ]]; then\n  make all\nelse\n  echo "[ENTRY] Skipping build (SKIP_BUILD=1)";\nfi\n/s' /Workspace/Project/Entry_Point.sh

ENV SKIP_BUILD=1

EXPOSE 1883

CMD ["/Workspace/Project/Entry_Point.sh"]
