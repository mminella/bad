#!/usr/bin/env python2
#
# Usage: `python graph_network_logs.py path/to/experiments/network_logs

import datetime
import matplotlib.pyplot as plt
import os
import sys


# constants
YEAR = 2015
FILE_OUT = 'graph.pdf'

# check usage

if len(sys.argv) != 2:
  print "Usage:", sys.argv[0], "<path to network measurement results>"
  exit(1)
base_dir = sys.argv[1]
if not base_dir.endswith('/'):
  base_dir += '/'


def GetData(directory, workers):
    """
    The `-` in network-scripts is stopping it from being
    import a module from where import we could import
    GetData. Copying this as is for now from draw_summary.py
    """
    stats = list()
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


def make_graph(data_set):
    """
    data_set: a dict of experiment setup string vs. list of experiment
    values.
    """
    # group by timestamp
    yv = []
    for timestamp, results in data_set.iteritems():
        yv.append(results)

    # plot graph
    fig, ax = plt.subplots(1)
    bp = plt.boxplot(yv)
    plt.setp(bp['boxes'], color='black')
    plt.setp(bp['whiskers'], color='black', ls='-')
    plt.setp(bp['fliers'], color='red', marker='+')

    # x-axis ticks
    plt.xticks([1, 2, 3, 4], ['July', 'August', 'September', 'October'], rotation=40)

    # labels and legend
    plt.ylabel('Throughput (MBits/sec)')
    plt.xlabel('Date of Measurement')

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
    ts_bandwidths_dict[timestamp] = GetData('{}4/sample_1'.format(base_dir), 32)


if __name__ == '__main__':
    """
    exp_result_list: A dict mapping from the exp setup
    string to a list of the resulting throughputs.
    """
    ts_bandwidths_dict = {}
    # Using timestamps in case we need to do something smart with the results
    # in future.
    get_jul_data(ts_bandwidths_dict, datetime.datetime(YEAR, 7, 1))
    get_aug_data(ts_bandwidths_dict, datetime.datetime(YEAR, 8, 1))
    get_sep_data(ts_bandwidths_dict, datetime.datetime(YEAR, 9, 1))
    get_oct_data(ts_bandwidths_dict, datetime.datetime(YEAR, 10, 1))

    # plot an experiment
    make_graph(ts_bandwidths_dict)
