# Summary of Readings

# Project Tungsten

* Project to improve Spark performance, three areas:
  - Memory management & Binary processing -- Explicit (unsafe) memory
    management, work on binary objects, not java objects.
  - Cache-aware computation -- Better exploit memory hierarchy.
  - Code generation -- For SQL and other DSL's ontop of Spark.

## Ousterhout, 2015, NSDI

* Measurement of performance-bottlenecks in Spark, where is our time going?
* Instrument Spark (to record blocking) on 2 benchmarks and 1 'production'
  workload. Production workload not well specified, a DataBricks customer.
* Results:
  - Disk I/0 responsible for at most a 19% slowdown in execution.
  - Network responsible for only 2-3% slowdown in execution.
  - Stragglers responsible for at most 10% slowdown in execution.
  - CPU bound!
* Stragglers (75% of) easy to identify: mostly GC or HDFS reads.
* Why CPU?
  - Java is slow -- C++ 2x faster.
  - Lots of time (up to 60%) spent decompressing and deserializing data.

## Conley, 2015

* Tools from paper: http://github.com/TritonNetworking/themis

* Problem:
  - Public cloud has lots of options, how do you choose a setup?
  - Scalability of performance is not simple, can't use one VM to model your
    cluster. How to measure and predict performance?

* Contributions:
  - Synthetic methodology for measuring the IO capabilities of
    high-performance VMs.
  - Measurement of current AWS VMs.
  - Evaluation of cost-efficient GraySort using above measurements.
  - New world records in sorting speed and cost-efficiency.

* Assumption:
  - IO bound (network or disk), not CPU for sorting (well, later measured in
    full GraySort evaulation).

* Important take-away here is that the performance of an individual VM can't be
  simply multiplied by your cluster size to get the cluster performance, the
  underlying hardware / virtualization has a scalability coefficient < 1.

* Themis -- own MR frame-work:
  - Unlike traditional MR, after `map()`, intermediate data is shuffled and
    sent to reducers, rather than being pushed to the local disk of the map
    node for FT. The reducer node writes shuffled data to local disk (before
    the reduce phase starts).
  - Reducers then read from local disk and sort shuffled data into key-order
    before applying `reduce()` function.
  - Output from reducer is written to local disk.
  - Pushing intermediate data to disk at reducer, rather than mapper, improves
    performance but degrades performance more when faults occur.

* External sort performance model:
  - 2-IO: Each record is read and written to storage devices exactly twice.

* Themis performance model: 2-IO (no replicas).
  - `B_storage = min (B_read, B_write)`
  - `T1 = max( D / B_storage, D / B_network )`
  - `T2 = D / B_storage`
  - `T = T1 + T2`

* Themis performance model: App-level replication.
  - `T2 = max( D / B_read, D / B_network, 2D / B_write )`
  - Assumption that same number of disks? So a single disk stores 2x partitions
    of output data?

* Themis performance model: App-level replication.
  - `T1 = max( D / B_readEBS, D / B_network, D / B_write )`
  - `T2 = max ( D / B_read, D / B_writeEBS )`

* Scaling model:
  - Measure per-VM local storage bandwidth.
  - Measure network-IO of each instance type (assumes perfect scalability in
    network performance).
  - Measure network-IO of a large cluster.

* Scalability measurements:
  - Used custom benchmarking tools: DiskBench, NetBench.
  - DiskBench: measures simultaneous read-write performance.
  - NetBench: doesn't use placement groups, single TCP connection, measure
    all-to-all network bandwidth as observed by slowest node.
  - Network performance from idea to cluster can drop by 20-50%.

* EBS measurements:
  - DiskBench used in read-only, or write-only mode, not simultaneous.
  - EBS Types: magnetic, SSD, provisioned IOPS SSD
  - Bandwidth of 250MB/s -- slow compared to local (up to 900MB/s both read and
    write).
  - Takes 2+ volumes to saturate read for SSD, and 3+ for magnetic.
  - Takes 3+ volumes to saturate write for SSD, and 8+ for magnetic.

* Eval 2-IO GraySort
  - Variance of VM's: Slowest storage bandwidth was 87% of previously measured,
    slowest network was 81% of previously measured.
  - 178 i2.8xlarge VM's in a placement group.
  - Estimated cost of $325.
  - Sort took 888 seconds, cost of $299.45.

