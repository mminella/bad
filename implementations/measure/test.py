#!/usr/bin/env python
import argparse
import subprocess
import sys

def DropCache():
    subprocess.call(['bash', 'drop_cache.sh'])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description='''Run a serial of disk performance tests,
            and write results to "log_x.txt".
            The experiements are in the following order:
            write, random_write, read, random_read, asyc_read,
            async_random_read.
            The files to be written should be given.
            ''')
    parser.add_argument('files',  metavar='file', type=str, nargs='+',
                        help='File names which specifiy each disk location.')
    parser.add_argument('--size', type=int, default=1024**3, help='File size.')
    parser.add_argument('--block', type=int, default=1024**2, help='Block size.')
    parser.add_argument('--fly', type=int, default=64,
                        help='Request queue for asynchronous implementation.')
    args = parser.parse_args()
    num_files = len(args.files)

    log_files = [open('log_{}.txt'.format(index), 'w')
                 for index in range(0, num_files)]

    status_list = list()
    for index in range(0, num_files):
        status_list.append(subprocess.Popen(
            ['./write', args.files[index], str(args.size), str(args.block)],
            stdout=log_files[index]))
    for status in status_list:
        status.wait()

    status_list = list()
    for index in range(0, num_files):
        status_list.append(subprocess.Popen(
            ['./random_write', args.files[index],
             str(args.size), str(args.block)],
            stdout=log_files[index]))
    for status in status_list:
        status.wait()

    DropCache()
    status_list = list()
    for index in range(0, num_files):
        status_list.append(subprocess.Popen(
            ['./read', args.files[index], str(args.block)],
            stdout=log_files[index]))
    for status in status_list:
        status.wait()

    DropCache()
    status_list = list()
    for index in range(0, num_files):
        status_list.append(subprocess.Popen(
            ['./random_read', args.files[index], str(args.block)],
            stdout=log_files[index]))
    for status in status_list:
        status.wait()


    DropCache()
    status_list = list()
    for index in range(0, num_files):
        status_list.append(subprocess.Popen(
            ['./async_read', args.files[index], str(args.block), str(args.fly)],
            stdout=log_files[index]))
    for status in status_list:
        status.wait()

    DropCache()
    status_list = list()
    for index in range(0, num_files):
        status_list.append(subprocess.Popen(
            ['./async_random_read', args.files[index],
             str(args.block), str(args.fly)],
            stdout=log_files[index]))
    for status in status_list:
        status.wait()

    for log_file in log_files:
        log_file.close()
