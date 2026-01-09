#!/usr/bin/env bash
set -euo pipefail

ROOT="/Workspace"
BUILD="${ROOT}/build"
PROJ="${ROOT}/Project"

# Prefer shared build outputs, fallback to SWC build dirs
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

# In 1-container mode, apps should connect to localhost
export MQTT_HOST="${MQTT_HOST:-127.0.0.1}"
export MQTT_PORT="${MQTT_PORT:-1883}"

echo "[ENTRY] Starting mosquitto (broker) ..."
mosquitto -c /etc/mosquitto/mosquitto.conf &
MOSQ_PID=$!

# Wait for broker port to be open (hard timeout, no hanging)
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

echo "[ENTRY] Building..."
cd "$PROJ"
make all

BACKOFFICE="$(pick_exe backoffice_app)"
TVM="$(pick_exe tvm_app)"
GATE="$(pick_exe gate_app)"

echo "[ENTRY] Starting backoffice in background: $BACKOFFICE"
"$BACKOFFICE" &
BO_PID=$!
echo "[ENTRY] backoffice PID=$BO_PID"

while true; do
  echo ""
  echo "1) Run TVM"
  echo "2) Run Gate"
  echo "q) Quit"
  read -r -p "Choice: " c
  case "$c" in
    1) "$TVM" ;;
    2) "$GATE" ;;
    q|Q) exit 0 ;;
    *) echo "Invalid choice" ;;
  esac
done
