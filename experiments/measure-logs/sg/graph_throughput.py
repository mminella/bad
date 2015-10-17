# Usage: `python graph_throughput.py path/to/experiments/measure_logs/sg/
# (with the trailing '/' in the end.

import datetime
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy
import os
import sys

base_sg_dir = sys.argv[1]
timestamp_file = '{}README.md'.format(base_sg_dir)
log_files = []
YEAR = 2015


for root, dirs, files in os.walk(base_sg_dir):
    if root != base_sg_dir and \
            root[:len(base_sg_dir+'14')] != '{}14'.format(base_sg_dir):
        # We don't have a timestamp for 14.
        for filename in files:
            if filename != 'log_cpu.txt' and filename[-3:] != 'pdf':
                log_files.append('{}/{}'.format(root, filename))


def make_graph(data_set, exp_name):
    """
    data_set: a dict of experiment setup string vs. list of experiment
    values.
    """
    labels_list = []
    x = []
    y = []
    timestamp_result_dict = {}

    for timestamp, result in data_set:
        # We now group the data as per the timestamp for every experiment run.
        if timestamp_result_dict.get(timestamp):
            timestamp_result_dict[timestamp].append(result)
        else:
            timestamp_result_dict[timestamp] = [result]

    for ts, results_list in sorted(timestamp_result_dict.iteritems()):
        # Make sure the data we are plotting is sorted by timestamp.
        x.append(ts)
        y.append(results_list)

    fig, ax = plt.subplots(1)
    labels_list.append(exp_name)
    fig.autofmt_xdate()
    plt.boxplot(y)
    ax.xaxis_date()
    plt.xticks(range(1, len(x)+1), x, rotation=15)

    # Labels and legend
    plt.ylabel('Throughput (in MB/s)')
    plt.xlabel('Time')
    plt.legend(labels_list)
    plt.show()


def get_timestamp_dict(timestamps):
    with open(timestamp_file, 'r') as f:
        for line in f:
            if line.strip() == "* 27, Mon Aug 31 11:45:24 PDT 2015, ap-southeast-1a, i2.xlarge":
                # hardcode this anomaly
                timestamps['27'] = datetime.datetime(2015, 8, 31, 11, 45, 24)

            elif line[0] == '*':
                # This is a line containing the timestamp we want.
                # It is of the form "* 26 -- 8/31 -- 3am"
                ts = line[2:]
                exp, d, t = ts.strip().split(' -- ')
                t = t.strip()  # some extra whitespace
                month, day = d.split('/')
                if t[-2:] == 'am':
                    hour = int(t[0])
                elif t[-2:] == 'pm':
                    hour = int(t[0]) + 12
                # TODO provide timezone.
                timestamps[exp] = datetime.datetime(
                    YEAR, int(month), int(day), hour
                )


def pick_an_experiment_to_plot(data_set):
    exp_to_plot = ''
    print "Please choose an experiment from the following:"
    for exp_name in data_set.keys():
        print exp_name
    exp_to_plot = raw_input("Please choose an experiment from the above : ")
    return exp_to_plot

if __name__ == '__main__':
    """
    exp_result_list: A dict mapping from the exp setup
    string to a list of the resulting throughputs.
    """
    exp_result_dict = {}
    timestamps = {}
    get_timestamp_dict(timestamps)
    for logfile in log_files:
        exp_no = logfile.rsplit('/', 3)[1]
        timestamp = timestamps[exp_no]
        with open(logfile, 'r') as f:
            for line in f:
                if line[0] not in 'wr':
                    # Assuming the non-setup/log lines are not going to start
                    # with a 'w' or 'r'
                    pass
                else:
                    exp_log = line.rsplit(',', 1)
                    result = [timestamp, float(exp_log[1])]
                    if exp_result_dict.get(exp_log[0]):
                        exp_result_dict[exp_log[0]].append(result)
                    else:
                        exp_result_dict[exp_log[0]] = [result]

    exp_to_plot = pick_an_experiment_to_plot(exp_result_dict)
    make_graph(exp_result_dict[exp_to_plot], exp_to_plot)
