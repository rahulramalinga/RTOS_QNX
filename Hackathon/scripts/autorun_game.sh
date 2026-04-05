#!/bin/sh
# ===========================================================================
# Event Horizon: Reactor Crisis - QNX VM Auto-Launch Script
#
# This script is called from post_start.custom during VM boot.
# It can also be run manually after SSH-ing into the VM.
#
# Does NOT touch network config (that's in post_start.custom
# and must stay there for SSH to work).
# ===========================================================================

INSTALL_DIR="/usr/share/event_horizon"
BIN_DIR="/usr/bin"

echo "============================================"
echo "  EVENT HORIZON: REACTOR CRISIS"
echo "  QNX SDP 8.0 Real-Time Space Game"
echo "============================================"

# --- Ensure screen service is available ---
echo "[LAUNCH] Waiting for /dev/screen..."
waitfor /dev/screen 10
if [ $? -ne 0 ]; then
    echo "[LAUNCH] ERROR: /dev/screen not available."
    echo "[LAUNCH] Ensure 'screen' service is running."
    exit 1
fi
echo "[LAUNCH] Graphics: OK"

# --- USB HID gamepad handler ---
# If io-hid isn't running with USB support, start it
if ! pidin ar | grep -q "io-hid.*usb"; then
    echo "[LAUNCH] Starting USB HID gamepad handler..."
    io-hid -dusb &
    sleep 1
fi

# --- List any USB HID devices found ---
echo "[LAUNCH] Scanning for gamepad devices..."
ls /dev/io-hid/ 2>/dev/null || echo "[LAUNCH] No io-hid devices found"
ls /dev/devi/ 2>/dev/null || echo "[LAUNCH] No devi devices found"

# --- Kill any previous instances ---
slay -f engine_proc 2>/dev/null
slay -f reactor_proc 2>/dev/null
slay -f game_main 2>/dev/null
sleep 1

# --- Start IPC services ---
echo "[LAUNCH] Starting engine_proc..."
${BIN_DIR}/engine_proc &
sleep 1

echo "[LAUNCH] Starting reactor_proc..."
${BIN_DIR}/reactor_proc &
sleep 1

# --- Start the game ---
echo "[LAUNCH] Starting Event Horizon..."
export LD_LIBRARY_PATH=/proc/boot:/system/lib:/system/lib/dll:/system/graphics/driver:/usr/lib
${BIN_DIR}/game_main

echo "[LAUNCH] Game exited."
