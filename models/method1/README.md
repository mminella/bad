# Model 1

Operation can be one of: `readall`, `first`, `nth` or `cdf`.

## point.R

Outputs the predicted performance for the three supported operations for a
particular job and cluster configuration.

`./point.R ../machines.csv i2.8x i2.1x 10 3400 0`

## graph_cluster.R

Graphs the predicted performance (as total time and cost) as the number of
nodes in the cluster vary for a specified job and cluster configuration.

`./graph_read_network.R ../machines.csv first i2.8x i2.1x 3400 20`

## graph_disk_network.R

Graphs the predicted performance (as network and disk time spent) as the number
of nodes in the cluster vary for a specified job and cluster configuration.

`./graph_disk_network.R ../machines.csv first i2.8x i2.1x 3400 20`

