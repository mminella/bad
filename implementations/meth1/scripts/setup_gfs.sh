#!/bin/sh

mkdir /mnt/b

/usr/share/google/safe_format_and_mount -m "mkfs.ext4 \
  -F" /dev/disk/by-id/google-local-ssd-0 /mnt/b

chmod a+w /mnt/b
