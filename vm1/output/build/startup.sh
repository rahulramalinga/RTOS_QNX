#!/bin/sh

. /proc/boot/build/options

# By default set umask so that services don't accidentally create group/world writable files.
umask 022


echo "---> Starting slogger2"
 slogger2 -U 21:21
waitfor /dev/slog

echo "---> Starting PCI Services"
 pci-server --config=/proc/boot/pci_server.cfg
waitfor /dev/pci/server_id_1
# Not ideal. Can't set ACLs. Perhaps supplementary groups would be better
chmod a+rx /dev/pci/server_id_1





echo "---> Starting fsevmgr"
 fsevmgr

echo "---> Starting devb"
  devb-eide cam user=20:20 blk cache=64M,auto=partition,vnode=2000,ncache=2000,commit=low 
waitfor /dev/hd0

echo "---> Mounting file systems"
mount_fs.sh
if [ ! -e /system ]; then
	echo No system file system, giving up.
	exit 1
fi


#To support proper operation of input capture via io-hid, devc-con-hid should be used instead
echo "---> Starting io-hid"
 io-hid -dps2ser-vm kbd:kbddev:vmware:mousedev  
waitfor /dev/io-hid/io-hid
chown 36 /dev/io-hid/io-hid

echo "---> Starting devc"
 devc-con-hid -e 
on  -d -t /dev/con1 ksh -l
# Start more consoles which can be switched to using ctrl-alt-[1-4]
on  -d -t /dev/con2 ksh -l
on  -d -t /dev/con3 ksh -l
on  -d -t /dev/con4 ksh -l

 random -U 22:22 -s /data/var/random/rnd-seed  
waitfor /dev/random
 pipe -U 24:24
waitfor /dev/pipe
 devc-pty -U 37:37
 dumper -U 30:30 -d /data/var/dumper -M 1000 -D cores

echo "---> Starting Networking"
 io-sock -m phy -m pci -m usb -d vmx   
start_net.sh

echo "---> Starting sshd"
if [ ! -f /data/var/ssh/ssh_host_rsa_key ]; then
    ssh-keygen -q -t rsa -N '' -f /data/var/ssh/ssh_host_rsa_key
    chown 0:0 /data/var/ssh/ssh_host_rsa_key*
fi
if [ ! -f /data/var/ssh/ssh_host_ed25519_key ]; then
    ssh-keygen -q -t ed25519 -N '' -f /data/var/ssh/ssh_host_ed25519_key
    chown 0:0 /data/var/ssh/ssh_host_ed25519_key*
fi
 /system/bin/sshd -f /system/etc/ssh/sshd_config

 qconn

echo "---> Starting misc"
 mqueue


# Execute the post startup script.  This is a separate script to allow for common behavior of
# both slm and script based startup.
 /system/etc/startup/post_startup.sh
