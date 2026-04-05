#define __SYS_TRUST_OPT__
#define __PAM_SECPOL_CONFIG__
#define __IO_USB_OTG_OPTIONS__ -d hcd-uhci -d hcd-ehci -d hcd-xhci
#define __SYS_SIZE__ 40960
#define __SYS_INODES__ 1000
#define __DATA_SIZE__ 122880
#define __DATA_INODES__ 3000
#define __TOOL_DIR__ /cygdrive/C//Users/yoy91/qnx800/host/win64/x86_64/../../common/mkqnximage
#define __EXTRA_PROC_OPTIONS__ 
#define __EXTRA_STARTUP_OPTIONS__ 
#define __MKFS_PATH__ ${MKFS_PATH}
#define __LIBBZ2__ libbz2.so.1
#define __LIBC__ libc.so.6
#define __LIBCPLUSPLUS__ libc++.so.2
#define __LIBCAM__ libcam.so.2
#define __LIBCARES__ libcares.so.8
#define __LIBCATALOG__ libcatalog.so.1
#define __LIBCRYPTO__ libcrypto.so.3
#define __LIBCURL__ libcurl.so.12
#define __LIBDNS_SD__ libdns_sd.so.1
#define __LIBEDIT__ libedit.so.0
#define __LIBEXPAT__ libexpat.so.2
#define __LIBFDT__ libfdt.so.1
#define __LIBFSNOTIFY__ libfsnotify.so.1
#define __LIBICONV__ libiconv.so.1
#define __LIBINTL__ libintl.so.1
#define __LIBIPERF__ libiperf.so.0
#define __LIBJAIL__ libjail.so.1
#define __LIBJSON__ libjson.so.1
#define __LIBLZ4__ liblz4.so.1
#define __LIBLZMA__ liblzma.so.5
#define __LIBM__ libm.so.3
#define __LIBNCURSESW__ libncursesw.so.1
#define __LIBPAM__ libpam.so.2
#define __LIBPANELW__ libpanelw.so.1
#define __LIBPCI__ libpci.so.3.0
#define __LIBPNG16__ libpng16.so.16
#define __LIBQCRYPTO__ libqcrypto.so.1.0
#define __LIBQH__ libqh.so.1
#define __LIBRCHECK__ librcheck.so.1
#define __LIBREGEX__ libregex.so.1
#define __LIBSECPOL__ libsecpol.so.1
#define __LIBSLOG2__ libslog2.so.1
#define __LIBSLOG2PARSE__ libslog2parse.so.1
#define __LIBSLOG2SHIM__ libslog2shim.so.1
#define __LIBSOCKET__ libsocket.so.4
#define __LIBSSL__ libssl.so.3
#define __LIBTRACELOG__ libtracelog.so.1
#define __LIBUBSAN__ libubsan.so.1
#define __LIBUNWIND__ libunwind.so.8
#define __LIBUSBDCI__ libusbdci.so.2
#define __LIBUSBDI__ libusbdi.so.2
#define __LIBXML2__ libxml2.so.2
#define __LIBZ__ libz.so.2
#define __LIBZSTD__ libzstd.so.1
#define __DEVB_NAME__ devb-eide
#if defined (OPT_GRAPHICS_yes)
#define __DEVC_NAME__ devc-con-hid
#else
#define __DEVC_NAME__ devc-con
#endif
#define __DEVC_PATH__ /dev/con1
#define __DEVC_PATH2__ /dev/con2
#define __DEVC_PATH3__ /dev/con3
#define __DEVC_PATH4__ /dev/con4
@
#if defined(IMAGE_TYPE_uefi)
#define __VIRTUAL__ x86_64,uefi
#else
#define __VIRTUAL__ x86_64,bios
#endif
#define __LD_QNX__ ldqnx-64.so.2
#define __STARTUP__ startup-x86
@
#define __IOPKT_OPTS__
#define __BOOT_ADDR__ 20m
#define __DEVB_OPTS__
#define __DEVC_OPTS__
#define __HAS_PCI__
#define __PCI_HW_MODULE__ pci_hw-Intel_x86.so
@
#define __NETDRIVER__ -d vmx
#define __NETDEVICE__ vmx0
@
#define __PCI_HW_CONFIG_FILE__
@
#define __GRAPHICS_VMWGFX_DRM__
@
#define __IFS_COMPRESS__ +compress
