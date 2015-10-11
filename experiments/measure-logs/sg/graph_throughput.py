# Usage: `python graph_throughput.py path/to/experiments/measure_logs/sg/
# (with the trailing '/' in the end.

# TODO:
# 1. Draw the median line.
# 2. Fix legend to not overlap with the plot.

import datetime
import matplotlib.pyplot as plt
import numpy
import os
import sys

base_sg_dir = sys.argv[1]
timestamp_file = '{}README.md'.format(base_sg_dir)
log_files = []
YEAR = 2015


for root, dirs, files in os.walk(base_sg_dir):
    if root != base_sg_dir and root[:len(base_sg_dir+'14')] != '{}14'.format(base_sg_dir):
        # We don't have a timestamp for 14.
        for filename in files:
            if filename != 'log_cpu.txt':
                log_files.append('{}/{}'.format(root, filename))


def make_graph(data_set):
    """
    data_set: a dict of experiment setup string vs. list of experiment
    values.
    """
    labels_list = []
    median_values = []
    median_timestamp = []
    colormap = plt.cm.gist_ncar
    plt.gca().set_color_cycle(
        [colormap(i) for i in numpy.linspace(0, 0.9, len(data_set))]
    )
#    fig = plt.figure(1)
#    ax = fig.add_subplot(111)
    for exp_name, results in data_set.iteritems():
        # show each point.
        # TODO: draw a line that is the median of each
        # experiment run.
        x = []
        y = []
        for timestamp, result in results:
            x.append(timestamp)
            y.append(result)
        median_values.append(numpy.median(y))
        plt.plot(x, y, 'o')
        #y_steps = numpy.linspace(0, ((int(y[-1])/100)+1)*100, len(y))
        #plt.ylim([0, y_steps[-1]])
        plt.ylabel('Throughput')
        plt.xlabel('Time')
        labels_list.append(exp_name)
    #plt.plot(median_values)
    #labels.append('Medians of each experiment run')
    plt.legend(labels_list)
#    handles, labels = ax.get_legend_handles_labels()
#    lgd = ax.legend(handles, labels_list, loc='upper center', bbox_to_anchor=(0.5,-0.1))
#    ax.grid('on')
#    fig.savefig('samplefigure', bbox_extra_artists=(lgd,), bbox_inches='tight')
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
                t = t.strip() # some extra whitespace
                # now it is ['26', '8/31', '3am']
                month, day = d.split('/')
                if t[-2:] == 'am':
                    hour = int(t[0])
                elif t[-2:] == 'pm':
                    hour = int(t[0]) + 12
                # TODO provide timezone.
                timestamps[exp] = datetime.datetime(YEAR, int(month), int(day), hour)


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
            """
            XXX: only one file for now, log taken at 7/29, 11pm
            """
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
    make_graph(exp_result_dict)
