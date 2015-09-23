# Usage: `python count_total_bandwidth.py path/to/logfile1 path/to/logfile2

import sys
import numpy


def cdf(data):
    """
    Returns a dictionary of the 10th, 50th, 90th and 95th percentiles of the
    input data.
    """
    percentile = {}
    percentile['10'] = numpy.percentile(numpy.array(data), 10)
    percentile['50'] = numpy.percentile(numpy.array(data), 50)
    percentile['90'] = numpy.percentile(numpy.array(data), 90)
    percentile['95'] = numpy.percentile(numpy.array(data), 95)

    return percentile


def print_cdf(cdf_dict):
    for k, v in sorted(cdf_dict.iteritems()):
        print "%sth percentile: %f" % (k, v)

if __name__ == '__main__':
    log_files = sys.argv[1:]
    rows = []
    read_logs = []
    write_logs = []
    unknowns = []
    all_readings = []
    number_of_readings = 0
    for logfile in log_files:
        with open(logfile, 'r') as f:
            for line in f:
                if line[0] not in 'wr':
                    # print "Not evaluating: ", line
                    pass
                else:
                    reading = line.split(',')
                    number_of_readings += 1
                    all_readings.append(reading)
                    if reading[0] == 'r':
                        read_logs.append(float(reading[-1]))
                    elif reading[0] == 'w':
                        write_logs.append(float(reading[-1]))
                    else:
                        unknowns.append(reading)

    if unknowns != []:
        print "Could not recognize these readings: ", unknowns
    else:
        max_read_bandwidth = max(read_logs)
        max_write_bandwidth = max(write_logs)
        read_cdf = cdf(read_logs)
        write_cdf = cdf(write_logs)

        print "No. of readings: ", number_of_readings
        print "Max read bandwidth: ", max_read_bandwidth, " MB/s"
        print "Max write bandwidth: ", max_write_bandwidth, " MB/s"
        print "CDF for reads:"
        print_cdf(read_cdf)
        print "CDF for writes:"
        print_cdf(write_cdf)
