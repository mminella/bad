#!/usr/bin/env python2
#
# Usage: `python graph_throughput.py path/to/experiments/measure_logs/sg/

import collections as coll
import datetime
import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import numpy as np
import os
import pylab
import sys

# constants
TIMESTAMP_FILE = '{}README.md'
FILE_OUT = 'graph.pdf'

# check usage
if len(sys.argv) != 2:
  print "Usage:", sys.argv[0], "<path to experiment results>"
  exit(1)
base_dir = sys.argv[1]
if not base_dir.endswith('/'):
  base_dir += '/'

def load_logfiles(root_dir):
    """
    root_dir: the directory we are loading all data from
    """
    logfiles = []
    for root, dirs, files in os.walk(root_dir):
        if root == root_dir:
            continue
        for filename in files:
            if filename != 'log_cpu.txt' and filename[-3:] != 'pdf':
                logfiles.append('{}/{}'.format(root, filename))
    return logfiles

def load_timestamps(root_dir):
    """
    root_dir: the directory we are loading all data from
    """
    timestamps = {}
    ts_file = TIMESTAMP_FILE.format(root_dir)
    with open(ts_file, 'r') as f:
        lines = f.readlines()[2:]
        for line in lines:
            # This is a line containing the timestamp we want.
            # It is of the form "* 26 -- 8/31 -- 3am"
            exp, d, t = line[2:].strip().split(' -- ')
            t = t.strip()
            year, month, day = d.split('/')
            if t[-2:] == 'am':
                hour = int(t[0:len(t)-2]) % 12
            elif t[-2:] == 'pm':
                hour = int(t[0:len(t)-2]) % 12 + 12
            timestamps[exp] = datetime.datetime(
                int(year), int(month), int(day), hour
            )
    return timestamps

def print_summary(data_set):
    results = sorted([z[1] for z in data_set])[0:]
    timestamps = set([z[0] for z in data_set])
    print "Samples:", len(data_set)
    print "Runs:", len(timestamps)
    print "Mean:", np.mean(results)
    print "Median:", np.median(results)
    print "Std:", np.std(results)
    print "Min:", np.min(results)
    print "Max:", np.max(results)

def datetime_to_date(dt):
    return datetime.date(dt.year, dt.month, dt.day)

def make_graph(data_set, exp_name):
    """
    data_set: a dict of experiment setup string vs. list of experiment
    values.
    """
    # group by timestamp
    data_by_timestamp = coll.defaultdict(list)
    for timestamp, result in data_set:
        ts = datetime_to_date(timestamp)
        data_by_timestamp[ts].append(result)

    # produce x, y values for graph
    xv = []
    yv = []
    start = sorted(data_by_timestamp.iteritems())[0][0]
    end = sorted(data_by_timestamp.iteritems())[-1][0]
    days = (end - start).days + 1
    dlist = [start + datetime.timedelta(days=x) for x in range(0, days)]
    for d in dlist:
      xv.append(d)
      if d in data_by_timestamp:
        yv.append(data_by_timestamp[d])
      else:
        # add in missing timestamps
        yv.append([])

    # plot graph
    fig, ax = plt.subplots(1)
    bp = plt.boxplot(yv, whis=[5,95], sym='')
    plt.setp(bp['boxes'], color='black')
    plt.setp(bp['whiskers'], color='black', ls='-')
    plt.setp(bp['fliers'], color='red', marker='+')
    fig.autofmt_xdate()
    ax.xaxis_date()

    # x-axis ticks
    xsubset = range(0, len(xv), 20)
    xlabels = [xv[i] for i in xsubset]
    plt.xticks(xsubset, xlabels, rotation=40)

    # y-axis range
    pylab.ylim([000,500])

    # add Amazon pessimistic line
    plt.text(-26.5, 448, "\"Expected\"", color='g')
    plt.axhline(456, color='r', xmin=-0.2, xmax=0.1)
    plt.text(-30.5, 130, "\"Pessimistic\"", color='g')
    plt.axhline(137, color='r', xmin=-0.2, xmax=0.1)

    # labels and legend
    plt.ylabel('Throughput (MB/s)')
    plt.xlabel('Date of Measurement')
    # plt.title('Sequential Disk IO Measurements')
    # plt.legend([exp_name])

    # Produce output
    # XXX To save a plot, instead of viewing it, replace `plt.show()` with:
    # plt.show()
    plt.savefig(FILE_OUT, bbox_inches='tight', pad_inches=2)

def pick_an_experiment_to_plot(experiments):
    # XXX: switch out to allow dynamic selection
    # for name in experiments.keys():
    #     print name
    # exp_to_plot = raw_input("Please choose an experiment from the above : ")
    # return exp_to_plot
    print "Using experiment: r, a, s, 1048576, 15, 1"
    return 'r, a, s, 1048576, 15, 1'

if __name__ == '__main__':
    """
    exp_result_list: A dict mapping from the exp setup
    string to a list of the resulting throughputs.
    """
    # load all needed data
    logfiles = load_logfiles(base_dir)
    timestamps = load_timestamps(base_dir)
    experiments = coll.defaultdict(list)

    # process data, grouping correctly by experiment
    for logfile in logfiles:
        exp_no = logfile.rsplit('/', 3)[1]
        timestamp = timestamps[exp_no]
        with open(logfile, 'r') as f:
            for line in f:
                if line[0] not in 'wr':
                    # Skip over non-experiment lines
                    continue
                experiment = line.rsplit(',', 1)
                result = [timestamp, float(experiment[1])]
                experiments[experiment[0]].append(result)

    # plot an experiment
    exp_to_plot = pick_an_experiment_to_plot(experiments)
    print_summary(experiments[exp_to_plot])
    make_graph(experiments[exp_to_plot], exp_to_plot)
