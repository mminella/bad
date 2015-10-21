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
    percentile['85'] = numpy.percentile(numpy.array(data), 90)
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
                # if line.startswith("w, s, s, 10485760, 1, 0"):
                if line[0] in "rw":
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
        print "No. of readings: ", number_of_readings

        if len(read_logs) != 0:
          print "Max read bandwidth: ", max(read_logs), " MB/s"
          print "Min read bandwidth: ", min(read_logs), " MB/s"
          print "CDF for reads:"
          print_cdf(cdf(read_logs))

        if len(write_logs) != 0:
          print "Max write bandwidth: ", max(write_logs), " MB/s"
          print "Min write bandwidth: ", min(write_logs), " MB/s"
          print "CDF for writes:"
          print_cdf(cdf(write_logs))
