import sys
import os.path
import matplotlib.pyplot as plot

def GetData(directory, workers):
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

if __name__ == '__main__':
    directory = sys.argv[1]
    workers = int(sys.argv[2])
    stats_1 = GetData(directory + '/sample_1', workers)
    stats_2 = GetData(directory + '/sample_2', workers)
    stats_3 = GetData(directory + '/sample_3', workers)
    plot.boxplot([stats_1, stats_2, stats_3], positions=[1,2,3])
    plot.xticks([1,2,3], ['Oct16#1','Oct16#2','Oct16#3'])
    plot.ylabel('Mbits/sec')
    plot.ylim(0, 10240)
    #plot.title('End-to-end bandwidth')
    plot.title('Aggregate outgoing bandwidth')
    plot.show()
