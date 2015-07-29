#!/usr/bin/env python
import argparse
from datetime import datetime
import subprocess
import sys
from time import sleep

def DropCache():
    subprocess.call(['bash', 'drop_cache'])

def DDRead(logs, disks, bs, count):
    status_list = list()
    for i in range(0, len(disks)):
        prg = ["sudo"] + ["dd"] + ["if=" + disks[i]] + ["of=/dev/null"] + \
            ["count=" + count] + ["bs=" + bs]
        status_list.append(subprocess.Popen(prg, stderr=logs[i]))
    for status in status_list:
        status.wait()
    sleep(1)

def DDWrite(logs, disks, bs, count):
    status_list = list()
    for i in range(0, len(disks)):
        prg = ["sudo"] + ["dd"] + ["of=" + disks[i]] + ["if=/dev/zero"] + \
            ["count=" + count] + ["bs=" + bs]
        status_list.append(subprocess.Popen(prg, stderr=logs[i]))
    for status in status_list:
        status.wait()
    sleep(1)

def ParseCmdLine():
    parser = argparse.ArgumentParser(
            description='''Run dd to test raw disk performance.''')
    parser.add_argument('disks',  metavar='disk', type=str, nargs='+',
        help='Which disks to test.')
    parser.add_argument('--count', type=int, default=1024, help='Block count.')
    parser.add_argument('--block', type=int, default=1024**2, help='Block size.')
    return parser.parse_args()

if __name__ == '__main__':
    args = ParseCmdLine()
    num_files = len(args.disks)

    log_files = [open('log_{}.txt'.format(index), 'a')
                 for index in range(0, num_files)]

    DDRead(log_files, args.disks, str(args.block), str(args.count))
    DDWrite(log_files, args.disks, str(args.block), str(args.count))

    for log_file in log_files:
        log_file.close()

