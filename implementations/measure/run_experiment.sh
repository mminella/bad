#!/bin/sh

# 1KB = 1024
# 1MB = 1024*1KB
# 1GB = 1000*1MB

# SYNC

# experiment runs: 3
# block sizes: 512, 4096, 1MB
# O_DIRECT: yes, no

dd if=/dev/zero of=/mnt/a/file bs=1048576 count=1000 conv=fdatasync,notrunc

## 1GB, 512B blocks
test_sync.py --name S_A_512 --count 2048000 --block 512 /mnt/a/file
test_sync.py --name S_B_512 --count 2048000 --block 512 /mnt/a/file
test_sync.py --name S_C_512 --count 2048000 --block 512 /mnt/a/file

## 1GB , 4096 blocks
test_sync.py --name S_A_4K --count 256000 --block 4096 /mnt/a/file
test_sync.py --name S_B_4K --count 256000 --block 4096 /mnt/a/file
test_sync.py --name S_C_4K --count 256000 --block 4096 /mnt/a/file

## 1GB , 1MB blocks
test_sync.py --name S_A_1MB --count 1000 --block 1048576 /mnt/a/file
test_sync.py --name S_B_1MB --count 1000 --block 1048576 /mnt/a/file
test_sync.py --name S_C_1MB --count 1000 --block 1048576 /mnt/a/file

## 1GB, 512B blocks
test_sync.py --name S_A_512_O --count 2048000 --block 512 -odirect /mnt/a/file
test_sync.py --name S_B_512_O --count 2048000 --block 512 -odirect /mnt/a/file
test_sync.py --name S_C_512_O --count 2048000 --block 512 -odirect /mnt/a/file

## 1GB , 4096 blocks
test_sync.py --name S_A_4K_O --count 256000 --block 4096 -odirect /mnt/a/file
test_sync.py --name S_B_4K_O --count 256000 --block 4096 -odirect /mnt/a/file
test_sync.py --name S_C_4K_O --count 256000 --block 4096 -odirect /mnt/a/file

## 1GB , 1MB blocks
test_sync.py --name S_A_1MB_O --count 1000 --block 1048576 -odirect /mnt/a/file
test_sync.py --name S_B_1MB_O --count 1000 --block 1048576 -odirect /mnt/a/file
test_sync.py --name S_C_1MB_O --count 1000 --block 1048576 -odirect /mnt/a/file

# ASYNC

# experiment runs: 3
# block sizes: 512, 4096, 1MB
# queue: 1, 15, 31, 62
# O_DIRECT: yes, no

# 100MB test file
dd if=/dev/zero of=/mnt/a/file bs=1048576 count=100 conv=fdatasync,notrunc

## 100MB, 512B blocks, 1 queue
test_async.py --name A_A_512_1 --block 512 -qdepth 1 /mnt/a/file
test_async.py --name A_B_512_1 --block 512 -qdepth 1 /mnt/a/file
test_async.py --name A_C_512_1 --block 512 -qdepth 1 /mnt/a/file

## 100MB, 512B blocks, 15 queue
test_async.py --name A_A_512_15 --block 512 -qdepth 15 /mnt/a/file
test_async.py --name A_B_512_15 --block 512 -qdepth 15 /mnt/a/file
test_async.py --name A_C_512_15 --block 512 -qdepth 15 /mnt/a/file

## 100MB, 512B blocks, 31 queue
test_async.py --name A_A_512_31 --block 512 -qdepth 31 /mnt/a/file
test_async.py --name A_B_512_31 --block 512 -qdepth 31 /mnt/a/file
test_async.py --name A_C_512_31 --block 512 -qdepth 31 /mnt/a/file

## 100MB, 512B blocks, 62 queue
test_async.py --name A_A_512_62 --block 512 -qdepth 62 /mnt/a/file
test_async.py --name A_B_512_62 --block 512 -qdepth 62 /mnt/a/file
test_async.py --name A_C_512_62 --block 512 -qdepth 62 /mnt/a/file


## 100MB, 4KB blocks, 1 queue
test_async.py --name A_A_4K_1 --block 4096 -qdepth 1 /mnt/a/file
test_async.py --name A_B_4K_1 --block 4096 -qdepth 1 /mnt/a/file
test_async.py --name A_C_4K_1 --block 4096 -qdepth 1 /mnt/a/file

## 100MB, 4KB blocks, 15 queue
test_async.py --name A_A_4K_15 --block 4096 -qdepth 15 /mnt/a/file
test_async.py --name A_B_4K_15 --block 4096 -qdepth 15 /mnt/a/file
test_async.py --name A_C_4K_15 --block 4096 -qdepth 15 /mnt/a/file

## 100MB, 4KB blocks, 31 queue
test_async.py --name A_A_4K_31 --block 4096 -qdepth 31 /mnt/a/file
test_async.py --name A_B_4K_31 --block 4096 -qdepth 31 /mnt/a/file
test_async.py --name A_C_4K_31 --block 4096 -qdepth 31 /mnt/a/file

## 100MB, 4KB blocks, 62 queue
test_async.py --name A_A_4K_62 --block 4096 -qdepth 62 /mnt/a/file
test_async.py --name A_B_4K_62 --block 4096 -qdepth 62 /mnt/a/file
test_async.py --name A_C_4K_62 --block 4096 -qdepth 62 /mnt/a/file


## 100MB, 1MB blocks, 1 queue
test_async.py --name A_A_1MB_1 --block 4096 -qdepth 1 /mnt/a/file
test_async.py --name A_B_1MB_1 --block 4096 -qdepth 1 /mnt/a/file
test_async.py --name A_C_1MB_1 --block 4096 -qdepth 1 /mnt/a/file

## 100MB, 1MB blocks, 15 queue
test_async.py --name A_A_1MB_15 --block 4096 -qdepth 15 /mnt/a/file
test_async.py --name A_B_1MB_15 --block 4096 -qdepth 15 /mnt/a/file
test_async.py --name A_C_1MB_15 --block 4096 -qdepth 15 /mnt/a/file

## 100MB, 1MB blocks, 31 queue
test_async.py --name A_A_1MB_31 --block 4096 -qdepth 31 /mnt/a/file
test_async.py --name A_B_1MB_31 --block 4096 -qdepth 31 /mnt/a/file
test_async.py --name A_C_1MB_31 --block 4096 -qdepth 31 /mnt/a/file

## 100MB, 1MB blocks, 62 queue
test_async.py --name A_A_1MB_62 --block 4096 -qdepth 62 /mnt/a/file
test_async.py --name A_B_1MB_62 --block 4096 -qdepth 62 /mnt/a/file
test_async.py --name A_C_1MB_62 --block 4096 -qdepth 62 /mnt/a/file


## 100MB, 512B blocks, 1 queue, odirect
test_async.py --name A_A_512_1_O --block 512 -qdepth 1 -odirect /mnt/a/file
test_async.py --name A_B_512_1_O --block 512 -qdepth 1 -odirect /mnt/a/file
test_async.py --name A_C_512_1_O --block 512 -qdepth 1 -odirect /mnt/a/file

## 100MB, 512B blocks, 15 queue, odirect
test_async.py --name A_A_512_15_O --block 512 -qdepth 15 -odirect /mnt/a/file
test_async.py --name A_B_512_15_O --block 512 -qdepth 15 -odirect /mnt/a/file
test_async.py --name A_C_512_15_O --block 512 -qdepth 15 -odirect /mnt/a/file

## 100MB, 512B blocks, 31 queue, odirect
test_async.py --name A_A_512_31_O --block 512 -qdepth 31 -odirect /mnt/a/file
test_async.py --name A_B_512_31_O --block 512 -qdepth 31 -odirect /mnt/a/file
test_async.py --name A_C_512_31_O --block 512 -qdepth 31 -odirect /mnt/a/file

## 100MB, 512B blocks, 62 queue, odirect
test_async.py --name A_A_512_62_O --block 512 -qdepth 62 -odirect /mnt/a/file
test_async.py --name A_B_512_62_O --block 512 -qdepth 62 -odirect /mnt/a/file
test_async.py --name A_C_512_62_O --block 512 -qdepth 62 -odirect /mnt/a/file


## 100MB, 4KB blocks, 1 queue, odirect
test_async.py --name A_A_4K_1_O --block 4096 -qdepth 1 -odirect /mnt/a/file
test_async.py --name A_B_4K_1_O --block 4096 -qdepth 1 -odirect /mnt/a/file
test_async.py --name A_C_4K_1_O --block 4096 -qdepth 1 -odirect /mnt/a/file

## 100MB, 4KB blocks, 15 queue, odirect
test_async.py --name A_A_4K_15_O --block 4096 -qdepth 15 -odirect /mnt/a/file
test_async.py --name A_B_4K_15_O --block 4096 -qdepth 15 -odirect /mnt/a/file
test_async.py --name A_C_4K_15_O --block 4096 -qdepth 15 -odirect /mnt/a/file

## 100MB, 4KB blocks, 31 queue, odirect
test_async.py --name A_A_4K_31_O --block 4096 -qdepth 31 -odirect /mnt/a/file
test_async.py --name A_B_4K_31_O --block 4096 -qdepth 31 -odirect /mnt/a/file
test_async.py --name A_C_4K_31_O --block 4096 -qdepth 31 -odirect /mnt/a/file

## 100MB, 4KB blocks, 62 queue, odirect
test_async.py --name A_A_4K_62_O --block 4096 -qdepth 62 -odirect /mnt/a/file
test_async.py --name A_B_4K_62_O --block 4096 -qdepth 62 -odirect /mnt/a/file
test_async.py --name A_C_4K_62_O --block 4096 -qdepth 62 -odirect /mnt/a/file


## 100MB, 1MB blocks, 1 queue, odirect
test_async.py --name A_A_1MB_1_O --block 4096 -qdepth 1 -odirect /mnt/a/file
test_async.py --name A_B_1MB_1_O --block 4096 -qdepth 1 -odirect /mnt/a/file
test_async.py --name A_C_1MB_1_O --block 4096 -qdepth 1 -odirect /mnt/a/file

## 100MB, 1MB blocks, 15 queue, odirect
test_async.py --name A_A_1MB_15_O --block 4096 -qdepth 15 -odirect /mnt/a/file
test_async.py --name A_B_1MB_15_O --block 4096 -qdepth 15 -odirect /mnt/a/file
test_async.py --name A_C_1MB_15_O --block 4096 -qdepth 15 -odirect /mnt/a/file

## 100MB, 1MB blocks, 31 queue, odirect
test_async.py --name A_A_1MB_31_O --block 4096 -qdepth 31 -odirect /mnt/a/file
test_async.py --name A_B_1MB_31_O --block 4096 -qdepth 31 -odirect /mnt/a/file
test_async.py --name A_C_1MB_31_O --block 4096 -qdepth 31 -odirect /mnt/a/file

## 100MB, 1MB blocks, 62 queue, odirect
test_async.py --name A_A_1MB_62_O --block 4096 -qdepth 62 -odirect /mnt/a/file
test_async.py --name A_B_1MB_62_O --block 4096 -qdepth 62 -odirect /mnt/a/file
test_async.py --name A_C_1MB_62_O --block 4096 -qdepth 62 -odirect /mnt/a/file

