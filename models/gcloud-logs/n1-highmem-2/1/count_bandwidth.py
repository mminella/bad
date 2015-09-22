log_files = ['log_0.txt']
rows = []
read_logs = []
write_logs = []
unknowns = []
all_readings = []
number_of_readings = 0
for logfile in log_files:
    f = open(logfile, 'r')
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
    f.close()

if unknowns != []:
    print "Could not recognize these readings: ", unknowns
else:
    max_read_bandwidth = max(read_logs)
    max_write_bandwidth = max(write_logs)

    print "No. of readings: ", number_of_readings
    print "Max read bandwidth: ", max_read_bandwidth, " MB/s"
    print "Max write bandwidth: ", max_write_bandwidth, " MB/s"
