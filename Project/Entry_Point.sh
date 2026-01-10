#!/usr/bin/env bash
set -euo pipefail

ROOT="/Workspace"
BUILD="${ROOT}/build"
PROJ="${ROOT}/Project"

MOSQ_DIR="${ROOT}/mosquitto"
MOSQ_CONF="${MOSQ_DIR}/mosquitto.conf"
MOSQ_LOG="${MOSQ_DIR}/mosquitto.log"

pick_exe() {
  local name="$1"
  local a="${BUILD}/${name}"
  local b=""
  case "$name" in
    backoffice_app) b="${BUILD}/Backoffice_SWC_build/${name}" ;;
    tvm_app)       b="${BUILD}/TVM_SWC_build/${name}" ;;
    gate_app)      b="${BUILD}/Gate_SWC_build/${name}" ;;
  esac
  [[ -x "$a" ]] && { echo "$a"; return 0; }
  [[ -n "$b" && -x "$b" ]] && { echo "$b"; return 0; }
  return 1
}

MOSQ_PID=""
BO_PID=""

cleanup() {
  [[ -n "$BO_PID" ]] && kill "$BO_PID" >/dev/null 2>&1 || true
  [[ -n "$MOSQ_PID" ]] && kill "$MOSQ_PID" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

export MQTT_HOST="${MQTT_HOST:-127.0.0.1}"
export MQTT_PORT="${MQTT_PORT:-1883}"

# Ensure mosquitto folder exists + log file is created
mkdir -p "${MOSQ_DIR}"

if [[ ! -f "${MOSQ_CONF}" ]]; then
  echo "[ERROR] Missing mosquitto config at ${MOSQ_CONF}"
  exit 3
fi

touch "${MOSQ_LOG}"
chmod 666 "${MOSQ_LOG}" || true

echo "[ENTRY] Starting mosquitto (broker) using ${MOSQ_CONF}"
mosquitto -c "${MOSQ_CONF}" &
MOSQ_PID=$!

echo "[ENTRY] Waiting for broker ${MQTT_HOST}:${MQTT_PORT} (max 3s)..."
ok=0
for _ in {1..6}; do
  if timeout 1 bash -lc "</dev/tcp/${MQTT_HOST}/${MQTT_PORT}" >/dev/null 2>&1; then
    ok=1
    break
  fi
  sleep 0.5
done
if [[ "$ok" -ne 1 ]]; then
  echo "[ERROR] Broker did not become ready. Exiting."
  exit 2
fi

cd "$PROJ"
# Check if all executables already exist
if pick_exe backoffice_app >/dev/null 2>&1 && \
   pick_exe tvm_app        >/dev/null 2>&1 && \
   pick_exe gate_app       >/dev/null 2>&1; then
    echo "[ENTRY] All executables found. Skipping build."
else
# Build all apps
    echo "[ENTRY] Executables missing. Building applications..."
    make all
fi

# Resolve executables (after build or skip)
BACKOFFICE="$(pick_exe backoffice_app)" || {
    echo "[ERROR] backoffice_app not found after build"
    exit 1
}

TVM="$(pick_exe tvm_app)" || {
    echo "[ERROR] tvm_app not found after build"
    exit 1
}

GATE="$(pick_exe gate_app)" || {
    echo "[ERROR] gate_app not found after build"
    exit 1
}


# Backoffice base URL for REST clients inside this container:
# ASSUMPTION: backoffice listens on localhost:8080 inside the same container
export BACKOFFICE_BASE_URL="${BACKOFFICE_BASE_URL:-http://127.0.0.1:8080}"

echo "[ENTRY] Starting backoffice in background: $BACKOFFICE"
"$BACKOFFICE" &
BO_PID=$!

while true; do
  echo ""
  echo "1) Select TVM if you want to puchase a new ticket "
  echo "2) Select Gate if You want to validate your ticket"
  echo "q) Quit"
  read -r -p "Choice: " c
  case "$c" in
    1) "$TVM" ;;
    2) "$GATE" ;;
    q|Q) exit 0 ;;
    *) echo "Invalid choice" ;;
  esac
done
