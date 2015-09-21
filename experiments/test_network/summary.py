import sys
import os.path

if __name__ == '__main__':
    directory = sys.argv[1]
    workers = int(sys.argv[2])
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
                    bandwidth = float(items[-2])
                else:
                    assert items[-1] == 'Mbits/sec'
                    bandwidth = float(items[-2]) / 1024
                bandwidth_array.append(bandwidth)
                f.close()
        print rank, sum(bandwidth_array), 'Gbits/sec'
