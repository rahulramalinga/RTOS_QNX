@# local/snippets/post_start.custom
@echo "[BOOT] Configuring Network for vmx0..."
@ifconfig vmx0 192.168.136.130 netmask 255.255.255.0 up
@sleep 2
@
@echo "[BOOT] Starting qconn for Momentics IDE..."
@qconn &
@sleep 1
@
@echo "[BOOT] Starting Event Horizon Services..."
@/usr/bin/engine_proc &
@sleep 1
@/usr/bin/reactor_proc &
@
@# Check if screen service is running
@echo "[BOOT] Checking Graphics Subsystem..."
@pidin -P screen | grep screen
@if [ $? -ne 0 ]; then
@    echo "[BOOT] WARNING: screen service not found! Restarting screen..."
@    screen &
@    sleep 2
@fi
@
@echo "[BOOT] Launching Raylib UI..."
@export LD_LIBRARY_PATH=/usr/lib:/lib
@export SDL_VIDEODRIVER=qnx
@export SDL_RENDER_DRIVER=opengles2
@/usr/bin/ui_raylib &
@
@echo "---> Starting graphics"
@if [ ! -e /data/var/fontconfig/CACHEDIR.TAG ]; then
@   FONTCONFIG_FILE=/system/etc/fontconfig/fonts.conf fc-cache &
@fi
@START_P(screent_t) screen -c /system/graphics/driver/graphics.conf DROPROOT_NP(__SCREEN_ID__)
@waitfor /dev/screen
@STARTU_P(io_usb_t,__IO_USB_ID__) io-usb-otg __IO_USB_OTG_OPTIONS__
