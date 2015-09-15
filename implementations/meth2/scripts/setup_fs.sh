#!/bin/sh

disk=$1
loc=/mnt/${disk}

sudo mkfs.ext4 -E nodiscard /dev/xvd${disk}
sudo mkdir -p ${loc}
sudo mount -o defaults,noatime,discard,nobarrier /dev/xvd${disk} ${loc}
sudo chown ubuntu:ubuntu ${loc}
sudo chmod 755 ${loc}

