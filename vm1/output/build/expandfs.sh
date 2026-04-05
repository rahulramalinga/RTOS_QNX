#!/bin/sh
# To avoid writing multi-gigabyte files to SD card partitions, only the bare minimum
# is written and in the case of the data partition, the partition size is wrong. So
# on boot fix everything up.

TMPDIR=/dev/shmem
cd /dev/shmem

. /proc/boot/build/options

fdisk /dev/hd0 info >diskinfo

# Note all the following uses a temporary file rather than assigning the output
# of grep to a variable. The latter doesn't work as pipe is not running.

grep '1:.*beg.*end' diskinfo >part_info
read sys_type <part_info
sys_type=${sys_type#*\(}
sys_type=${sys_type%%\)*}

grep '2:.*beg.*end' diskinfo >part_info
read data_type <part_info
data_type=${data_type#*\(}
data_type=${data_type%%\)*}


# The -s is to avoid the file system being checked, something that can take some
# time. We simply want to expand it if it needs expanding.
if ! chkqnx6fs -xs /dev/hd0t${data_type} >chkqnx6fs.out; then
   cat chkqnx6fs.out
fi
# Only expand system partition if using QNX6 (directly)
if [ "$sys_type" = 178 ]; then
   if ! chkqnx6fs -xs /dev/hd0t${sys_type} >chkqnx6fs.out; then
      cat chkqnx6fs.out
   fi
fi

rm -f chkqnx6fs.out diskinfo part_info
