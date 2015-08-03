import os
dir_name = 'inter_32_to_32'
for ip_dir in os.listdir(dir_name):
    s = 0
    for i in range(1,33):
        f = open(dir_name + '/' + ip_dir + '/t' + str(i) + '.log', 'r')
        lines = f.readlines()
        s += int(lines[-1].split()[-2])
    print 'the total ougoing bandwidth of {} : {} Mbits/s'.format(ip_dir, s)
