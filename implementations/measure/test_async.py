#!/usr/bin/env python
import argparse
from datetime import datetime
import subprocess
import sys
from time import sleep

def DropCache():
    subprocess.call(['bash', 'drop_cache'])

def Bench(prog, files, args):
    status_list = list()
    for i in range(0, len(files)):
        status_list.append(subprocess.Popen([prog] + [files[i]] + args,
            stdout=log_files[i]))
    for status in status_list:
        status.wait()
    sleep(1)

def ParseCmdLine():
    parser = argparse.ArgumentParser(
            description='''Run a serial of disk performance tests,
            and write results to "log_x.txt". The experiements are in the
            following order: write, random_write, read, random_read, asyc_read,
            async_random_read. The files to be written should be given.
            ''')
    parser.add_argument('files',  metavar='file', type=str, nargs='+',
        help='File names which specifiy each disk location.')
    parser.add_argument('--block', type=int, default=1024**2, help='Block size.')
    parser.add_argument('--qdepth', type=int, default=31,
        help='Request queue for asynchronous implementation.')
    parser.add_argument('--odirect', default=False, action='store_true',
        help='Use O_DIRECT I/O.')
    return parser.parse_args()

if __name__ == '__main__':
    args = ParseCmdLine()
    num_files = len(args.files)
    odirect = []
    if args.odirect:
        odirect = ['True']

    log_files = [open('log_{}.txt'.format(index), 'a')
                 for index in range(0, num_files)]

    # run experiments
    DropCache()
    Bench('measure_async_read', args.files,
        [str(args.block), str(args.qdepth)] + odirect)
    DropCache()
    Bench('measure_async_random_read', args.files,
        [str(args.block), str(args.qdepth)] + odirect)

    for log_file in log_files:
        log_file.close()
