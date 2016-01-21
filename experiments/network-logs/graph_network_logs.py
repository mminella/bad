#!/usr/bin/env python2
#
# Usage: `python graph_network_logs.py path/to/experiments/network_logs

import datetime
import matplotlib.pyplot as plt
import numpy as np
import os
import pylab
import sys
from textwrap import wrap


# constants
FILE_OUT = 'graph.pdf'

# check usage

if len(sys.argv) != 2:
  print "Usage:", sys.argv[0], "<path to network measurement results>"
  exit(1)
base_dir = sys.argv[1]
if not base_dir.endswith('/'):
  base_dir += '/'

def GetData(directory, workers):
    return GetData_(list(), directory, workers)

def GetData_(stats, directory, workers):
    """
    The `-` in network-scripts is stopping it from being
    import a module from where import we could import
    GetData. Copying this as is for now from draw_summary.py
    """
    for rank in range(0, workers):
        dir_name = os.path.join(directory, 'result_{}'.format(rank))
        assert os.path.exists(dir_name)
        bandwidth_array = list()
        for inner_rank in range(0, workers):
            if inner_rank != rank:
                file_name = os.path.join(
                        dir_name, 'bandwidth_{}.log'.format(inner_rank))
                f = open(file_name, 'r')
                line = f.readlines()[-1]
                items = line.split()
                if items[-1] == 'Gbits/sec':
                    bandwidth = float(items[-2])*1024
                else:
                    assert items[-1] == 'Mbits/sec'
                    bandwidth = float(items[-2])
                bandwidth_array.append(bandwidth)
                f.close()
        #stats.extend(bandwidth_array)
        stats.append(sum(bandwidth_array))
    return stats

def print_summary(data_set):
    allPoints = [item for sublist in data_set for item in sublist]
    print "Samples:", len(allPoints)
    print "Mean:", np.mean(allPoints)
    print "Median:", np.median(allPoints)
    print "Std:", np.std(allPoints)
    print "Min:", np.min(allPoints)
    print "Max:", np.max(allPoints)
    print "5th: ", np.percentile(allPoints, 5)
    print "10th: ", np.percentile(allPoints, 10)

def make_graph(data_set):
    """
    data_set: a dict of experiment setup string vs. list of experiment
    values.
    """
    # group by timestamp
    yv = []
    for timestamp in sorted(data_set.keys()):
        yv.append(data_set[timestamp])

    # print summary
    print_summary(yv)

    # plot graph
    fig, ax = plt.subplots(1)
    bp = plt.boxplot(yv, whis=[5,95], sym='')
    plt.setp(bp['boxes'], color='black')
    plt.setp(bp['whiskers'], color='black', ls='-')
    plt.setp(bp['fliers'], color='red', marker='+')

    # add Amazon pessimistic line
    plt.text(-0.2, 9450, "\"Expected\"", color='g')
    plt.axhline(9600, color='r', xmin=-0.2, xmax=0.05)
    plt.text(-0.3, 8302, "\"Pessimistic\"", color='g')
    plt.axhline(8002, color='r', xmin=-0.2, xmax=0.05)

    # y-axis range
    pylab.ylim([0,11000])

    # x-axis ticks
    plt.xticks(
        [1, 2, 3, 4, 5, 6, 7],
        # TODO: Add new measurements here...
        ['2015-07-05', '2015-08-03', '2015-09-12', '2015-10-16', '2015-11-12',
          '2015-12-20', '2016-01-15'],
        rotation=30)

    # labels and legend
    plt.ylabel("\n".join(wrap('Total Outgoing Throughput Per-Node (Mbits/s)', 24)))
    plt.xlabel('Date of Measurement', labelpad=10)

    # Produce output
#    plt.show()
    plt.savefig(FILE_OUT, bbox_inches='tight', pad_inches=2)


def get_jul_data(ts_bandwidths_data, timestamp):
    logfile = '{}1/bandwidth_32.out'.format(base_dir)
    line_no = 1
    with open(logfile, 'r') as f:
        ts_bandwidths_dict[timestamp] = []
        for line in f:
            if line_no > 32:
                break
            line_spl = line.split()
            bw = float(line_spl[-1])
            ts_bandwidths_dict[timestamp].append(bw)
            line_no += 1


def get_aug_data(ts_bandwidths_data, timestamp):
    logfile = '{}2/summary'.format(base_dir)
    with open(logfile, 'r') as f:
        ts_bandwidths_dict[timestamp] = []
        for line in f:
            line_spl = line.split()
            bw = float(line_spl[-2])
            ts_bandwidths_dict[timestamp].append(bw)


def get_sep_data(ts_bandwidths_data, timestamp):
    logfile = '{}3/summary_32.txt'.format(base_dir)
    with open(logfile, 'r') as f:
        ts_bandwidths_dict[timestamp] = []
        for line in f:
            bw = float(line.strip().split()[1])*1000
            ts_bandwidths_dict[timestamp].append(bw)


def get_oct_data(ts_bandwidths_data, timestamp):
    stats = list()
    GetData_(stats, '{}4/sample_1'.format(base_dir), 32)
    GetData_(stats, '{}4/sample_2'.format(base_dir), 32)
    GetData_(stats, '{}4/sample_3'.format(base_dir), 32)
    ts_bandwidths_dict[timestamp] = stats

def get_nov_data(ts_bandwidths_data, timestamp):
    stats = list()
    GetData_(stats, '{}5/sample_1'.format(base_dir), 16)
    GetData_(stats, '{}5/sample_2'.format(base_dir), 16)
    GetData_(stats, '{}5/sample_3'.format(base_dir), 16)
    ts_bandwidths_dict[timestamp] = stats

def get_dec_data(ts_bandwidths_data, timestamp):
    stats = list()
    GetData_(stats, '{}6/sample_1'.format(base_dir), 16)
    GetData_(stats, '{}6/sample_2'.format(base_dir), 16)
    GetData_(stats, '{}6/sample_3'.format(base_dir), 16)
    ts_bandwidths_dict[timestamp] = stats

def get_jan_data(ts_bandwidths_data, timestamp):
    stats = list()
    GetData_(stats, '{}7/sample_1'.format(base_dir), 16)
    GetData_(stats, '{}7/sample_2'.format(base_dir), 16)
    GetData_(stats, '{}7/sample_3'.format(base_dir), 16)
    ts_bandwidths_dict[timestamp] = stats

if __name__ == '__main__':
    """
    exp_result_list: A dict mapping from the exp setup
    string to a list of the resulting throughputs.
    """
    ts_bandwidths_dict = {}
    # Using timestamps in case we need to do something smart with the results
    # in future.

    # TODO: Add new measurements here...
    get_jul_data(ts_bandwidths_dict, datetime.datetime(2015,  7, 1))
    get_aug_data(ts_bandwidths_dict, datetime.datetime(2015,  8, 1))
    get_sep_data(ts_bandwidths_dict, datetime.datetime(2015,  9, 1))
    get_oct_data(ts_bandwidths_dict, datetime.datetime(2015, 10, 1))
    get_nov_data(ts_bandwidths_dict, datetime.datetime(2015, 11, 1))
    get_dec_data(ts_bandwidths_dict, datetime.datetime(2015, 12, 1))
    get_jan_data(ts_bandwidths_dict, datetime.datetime(2016,  1, 1))

    # plot an experiment
    make_graph(ts_bandwidths_dict)
