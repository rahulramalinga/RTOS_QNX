#!/bin/sh

if_up -p -r 20 vmx0
ifconfig vmx0 up


IP_ADDR=dhcp
HOSTNAME=noname

. /data/var/etc/settings/network

setconf _CS_HOSTNAME ${HOSTNAME}

if [ "$IP_ADDR" != dhcp ]; then
	ifconfig vmx0 $IP_ADDR
fi

sysctl -w net.inet.icmp.bmcastecho=1 > /dev/null
sysctl -w qnx.sec.droproot=33:33 > /dev/null

# If dhcpcd not run as root, need to give it read/write access to /dev/bpf
setfacl -m user:38:rw  /dev/bpf

if [ "$IP_ADDR" = dhcp ]; then
	 dhcpcd -bq -f /system/etc/dhcpcd/dhcpcd.conf -c /system/etc/dhcpcd/dhcpcd-run-hooks
else
	/system/etc/startup/pinger.sh delay &
fi

exit 0
