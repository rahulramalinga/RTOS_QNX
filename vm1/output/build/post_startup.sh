#!/bin/sh

# Configure locations of PAM files.  Note, pamconf must end with a slash
setconf pamlib /system/lib/dll/pam
setconf pamconf /system/etc/pam/

# local/snippets/post_start.custom
echo "[BOOT] Configuring Network for vmx0..."
ifconfig vmx0 192.168.136.130 netmask 255.255.255.0 up
sleep 2

echo "[BOOT] Starting qconn for Momentics IDE..."
qconn &
sleep 1

echo "[BOOT] Starting Event Horizon Services..."
/usr/bin/engine_proc &
sleep 1
/usr/bin/reactor_proc &

# Check if screen service is running
echo "[BOOT] Checking Graphics Subsystem..."
pidin -P screen | grep screen
if [ $? -ne 0 ]; then
    echo "[BOOT] WARNING: screen service not found! Restarting screen..."
    screen &
    sleep 2
fi

echo "[BOOT] Launching Raylib UI..."
export LD_LIBRARY_PATH=/usr/lib:/lib
export SDL_VIDEODRIVER=qnx
export SDL_RENDER_DRIVER=opengles2
/usr/bin/ui_raylib &

echo "---> Starting graphics"
if [ ! -e /data/var/fontconfig/CACHEDIR.TAG ]; then
   FONTCONFIG_FILE=/system/etc/fontconfig/fonts.conf fc-cache &
fi
 screen -c /system/graphics/driver/graphics.conf -U 36:36
waitfor /dev/screen
 io-usb-otg -d hcd-uhci -d hcd-ehci -d hcd-xhci

echo Process count:`pidin arg | wc -l`

exit 0
