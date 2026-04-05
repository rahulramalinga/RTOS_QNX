@lib/__LIBSSL__=usr/lib/__LIBSSL__
@lib/__LIBCURL__=usr/lib/__LIBCURL__
@lib/__LIBCARES__=usr/lib/__LIBCARES__
@/bin/setkey=sbin/setkey
@lib/__LIBLZMA__=usr/lib/__LIBLZMA__
@[perms=555] bin/cleanup_tmp=__TOOL_DIR__/inputs/cleanup_tmp
#if defined(__WIFIDEVICE__)
@bin/wpa_supplicant-2.11=usr/sbin/wpa_supplicant-2.11
#endif
@# local/snippets/system_files.custom using absolute host paths
@/usr/bin/engine_proc = C:/Users/yoy91/Documents/Projects/RTOS/Hackathon/bin/engine_proc
@/usr/bin/reactor_proc = C:/Users/yoy91/Documents/Projects/RTOS/Hackathon/bin/reactor_proc
@/usr/bin/ui_raylib = C:/Users/yoy91/Documents/Projects/RTOS/Hackathon/bin/ui_raylib
@/usr/lib/libSDL2-2.0.so.0 = C:/Users/yoy91/Documents/Projects/RTOS/Hackathon/external/lib/libSDL2.so
@/usr/lib/libSDL2.so = C:/Users/yoy91/Documents/Projects/RTOS/Hackathon/external/lib/libSDL2.so
@/usr/bin/bridge_ui = C:/Users/yoy91/Documents/Projects/RTOS/Hackathon/bin/bridge_ui
@
@
@######################################################
@### Common Graphics Files
@######################################################
@/etc/system/config/img.conf=etc/system/config/img.conf
@
@######################################################
@### Fonts
@######################################################
@[perms=0444] fonts=usr/share/fonts
@[perms=0444] etc/fontconfig/fonts.conf=/cygdrive/C//Users/yoy91/qnx800/host/win64/x86_64/../../common/mkqnximage/inputs/fonts.conf
@bin/fc-cache=bin/fc-cache
@
@######################################################
@### Graphics - Screen
@######################################################
@/bin/screen=sbin/screen
@/bin/screencmd=bin/screencmd
@/bin/screeninfo=bin/screeninfo
@/bin/hidview=sbin/hidview
@
@/bin/gles2-gears=usr/bin/gles2-gears
@/bin/sw-vsync=usr/bin/sw-vsync
@
@########################################
@## libraries
@########################################
@/lib/libgestures.so.1=lib/libgestures.so.1
@/lib/libscrmem.so.1=lib/libscrmem.so.1
@/lib/libmemobj.so.1=lib/libmemobj.so.1
@/lib/libinputevents.so.1=lib/libinputevents.so.1
@/lib/libkalman.so.1=lib/libkalman.so.1
@/lib/libimg.so.1=lib/libimg.so.1
@/lib/libdrm.so.1=usr/lib/libdrm.so.1
@
@########################################
@## DLLs
@########################################
@/lib/dll/screen-debug.so=lib/dll/screen-debug.so
@/lib/dll/screen-gles2blt.so=lib/dll/screen-gles2blt.so
@/lib/dll/screen-sw.so=lib/dll/screen-sw.so
@/lib/dll/screen-stdbuf.so=lib/dll/screen-stdbuf.so
@
@#########################################
@## more libraries
@#########################################
@/lib/libGLESv2.so.1=usr/lib/libGLESv2.so.1
@/lib/libEGL.so.1=usr/lib/libEGL.so.1
@/lib/libscreen.so.1=usr/lib/libscreen.so.1
@/lib/libswblit.so.1=usr/lib/libswblit.so.1
@[type=link] /lib/libswblit.so=libswblit.so.1
@/lib/libWFD.so.1=usr/lib/libWFD.so.1
@/lib/__LIBFREETYPE__=usr/lib/__LIBFREETYPE__
@/lib/libfontconfig.so.1=usr/lib/libfontconfig.so.1
@/lib/__LIBPNG16__=usr/lib/__LIBPNG16__
@
@#############################################
@### Image Loading
@#############################################
@/lib/dll/img_codec_bmp.so=lib/dll/img_codec_bmp.so
@/lib/dll/img_codec_gif.so=lib/dll/img_codec_gif.so
@/lib/dll/img_codec_jpg.so=lib/dll/img_codec_jpg.so
@/lib/dll/img_codec_png.so=lib/dll/img_codec_png.so
@/lib/dll/img_codec_sgi.so=lib/dll/img_codec_sgi.so
@/lib/dll/img_codec_tga.so=lib/dll/img_codec_tga.so
@/lib/dll/img_codec_tif.so=lib/dll/img_codec_tif.so
@
@
@#############################################
@### Input
@#############################################
@/lib/libhiddi.so.1=lib/libhiddi.so.1
@[type=link] /lib/libhiddi.so=libhiddi.so.1
@/lib/dll/devh-ps2ser.so=lib/dll/devh-ps2ser.so
@/lib/dll/devh-ps2ser-vm.so=lib/dll/devh-ps2ser-vm.so
@/lib/libvmmouse.so.1=lib/libvmmouse.so.1
@/bin/io-hid=sbin/io-hid
@/graphics/keyboard/en_US_101.kbd=usr/share/keyboard/en_US_101.kbd
@
#if defined(__GRAPHICS_VMWGFX_DRM__)
#include "inputs/system_files_vmwgfx-drm"
#endif
#if defined(__GRAPHICS_VIRTIO_DRM__)
#include "inputs/system_files_virtio-drm"
#endif
@
@bin/secpol=bin/secpol
@bin/io-usb-otg=sbin/io-usb-otg
@lib/dll/devu-hcd-xhci.so=lib/dll/devu-hcd-xhci.so
@lib/dll/devu-hcd-ehci.so=lib/dll/devu-hcd-ehci.so
@lib/dll/devu-hcd-uhci.so=lib/dll/devu-hcd-uhci.so
@lib/__LIBUSBDI__=lib/__LIBUSBDI__
@lib/dll/fs-dos.so=lib/dll/fs-dos.so
@[perms=555] etc/startup/pinger.sh={
@#!/bin/sh
@# Script to ping all interfaces of our host as a means of getting our own address into the arp cache
@if ! ifconfig | grep 'inet .\*broadcast' | grep -v 0.0.0.0 >/dev/null; then
@	exit
@fi
@if [ "$1" = delay ]; then
@	# It can take several seconds after start-up before io-sock is capable of pinging.
@	sleep 6
@fi
@# Also use an address by replacing the last octet of our address with 1.
@guessed_addr=`ifconfig | grep broad | sed -r 's/.\*broadcast ([0-9]+\\\\.[0-9]+\\\\.[0-9]+)\\\\..\*/\\\\1.1/'`
@addr_list=192.168.73.1:192.168.136.1:192.168.0.11:$guessed_addr
@IFS=:
@for ip in $addr_list; do
@	ping -c 1 -t 1 $ip >/dev/null 2>&1 &
@done
@}
#if defined(OPT_IO_SOCK_DIAG_yes)
@lib/dll/devs-vmx-diag.so=lib/dll/devs-vmx-diag.so
#else
@lib/dll/devs-vmx.so=lib/dll/devs-vmx.so
#endif
