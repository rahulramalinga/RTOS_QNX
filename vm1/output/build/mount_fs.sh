#!/bin/sh

# Checks the magic number in the superblock to identify a file system
# check_magic <block-dev> <offset> <magic>
#    <block-dev>  Path to block device containing file system
#    <offset>     Offset to location of magic value in file system
#    <magic>      Expected value (constant reversed and expressed as an octal string)
function check_magic
{
	# Output everything to files and compare the files.
	dd if=$1 bs=1 skip=$2 count=4 >/dev/shmem/fs-magic1 2>/dev/null
	print -Re $3 >/dev/shmem/fs-magic2
	[ $(</dev/shmem/fs-magic1) = $(</dev/shmem/fs-magic2) ]
}


echo "---> Mounting file systems"

if [ -e /data -a -e /system ]; then
	exit 0
fi


mount -t qnx6 /dev/hd0t177 /boot
if [ -e /boot/onboot ]; then
	echo Running onboot script
	ksh /boot/onboot
fi


if [ -e /dev/hd0t185 ]; then
	# Mounting a QTD protected file system is a two step process.  First mount the QTD
	# container and then the file system contained within
	mount -t qtd -o key=/proc/boot/qtd_public_key.pem  /dev/hd0t185 /dev/hd0t185-fs
	if check_magic /dev/hd0t185-fs 8192 "\0042\0021\0031\0150"; then
		mount -t qnx6 -o noatime  /dev/hd0t185-fs /system
	elif check_magic /dev/hd0t185-fs 0 "\0377\0272\0377\0261"; then
		mount -t qcfs  /dev/hd0t185-fs /system
	else
		echo "Don't recognise the filesystem"
	fi
	rm -f /dev/shmem/fs-magic*
elif [ -e /dev/hd0t186 ]; then
    qtsafefsd -o key=/proc/boot/qtsafefs_public_key.pem  /dev/hd0t186 /system
elif [ -e /dev/hd0t181 ]; then
	mount -t qcfs  /dev/hd0t181 /system
else # QNX6
	mount -t qnx6 -o noatime  /dev/hd0t178 /system 
fi


mount -t qnx6  /dev/hd0t179 /data 

/system/bin/cleanup_tmp
