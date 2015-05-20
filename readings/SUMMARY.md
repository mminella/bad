# Summary of Readings

# Project Tungsten

* Project to improve Spark performance, three areas:
  - Memory management & Binary processing -- Explicit (unsafe) memory
    management, work on binary objects, not java objects.
  - Cache-aware computation -- Better exploit memory hierarchy.
  - Code generation -- For SQL and other DSL's on top of Spark.

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

## BASIL: Automated IO Load Balancing Across Storage Devices

* Modelling approaches:
  - Black box vs White: Black box models are oblivious to the internal details
    of the storage device, white box include some of those details.
  - Absolute vs relative: Absolute models try to predict the actual performance
    characteristics of an app placed on a storage device, while relative just
    provide the relative change in performance when an app is moved from
    storage device A to B.

* BASIL: Black-box + relative.

* Related work:
  - Intro: [12, 21]
  - Storage reconfiguration: Hippodrome [10], Minerva [8].
  - Performance prediction: Mesnier (black-box, relative) [15] .
  - Analytical Models: [14, 17, 29, 20]
    - Table-based: [9]
    - ML: [22]

* BASIL Model: Workload metrics -> Storage metrics -> Performance
  - Model (essentially the above function) is simple linear extrapolation from
    the storage metrics to IO latencies.
  - Workload metrics determine a place on the model.

* Performance metrics:
  - IO Latency (used since allows a very simple linear model).
  - Lower is better!

* Workload metrics:
  - Average IO latency along following:
    - Seek distance (randomness)
    - IO size
    - Read-write ratio
    - Outstanding IOs
  - Mostly inherit to app and independent of storage device:
    - However, some co-dependency: e.g., IO size for a database logger may
      decrease as latency decreases.
  - BASIL assumes workload metrics are independent of IO device.
    - Mesnier's model could capture the co-dependency.

* Storage metrics:
  - Measure impact of IO latency across a cross product of the 4 workload
    metrics that gives 750 configurations.

* Load balancing:
  - Use performance model to predict average IO latency for an app when on a
    storage device.
  - Lower average IO latency is assumed to be better.
  - Optimize in some weighted fashion to improve overall performance across
    many applications.

* IO Latency Model:
  - `L = [ (K1 + OIO)*(K2 + IOSize)*(K3 + read%)*(K4 + random%) ] / K5`
  - Take the 750 measurements per storage array and use it to figure out Ki
    values.
  - Linear fit (with minimizing least square error) used for K5.
  - Most model prediction errors from very sequential workloads, so could
    special case them. But for now, just leave.

* Storage Device Model:
  - Collect (OIO, Latency) data pairs for a storage device.
  - Compute a linear fit for the pairs, i.e., a slope parameter (P).
  - Use as a (relative) model for the storage device (instead of needing the
    full Ki model!).
  - When moving a workload with known latency L from a storage device D1 to a
    storage device D2, we estimate it's new latency L' using:
    - `L' = P(D1) / P(D2) * L`
  - Model only works if OIO is a good predictor for IO behavior (i.e.,
    consistent). To improve over some anomalies, made two changes:
    1) Only consider read OIOs and average read latencies.
    2) Ignore "outliers" (e.g., IO size > 32KB, sequentiality > 90%).

* Load Balancing:
  - Workload on a storage device is the sum of `L` (with K5 = 1) for each app
    on that storage device.
  - Capacity of a storage device is simply its slope, P, since slope is a
    relative measure of performance (i.e., the same storage device with twice
    the disk should have a slope 2x larger).

## Modelling the Relative Fitness of Storage

* A relative fitness model predicts performance differences between a pair of
  devices.
  - Allows capturing the workload changes as an app moves from storage A to B.
  - Allows performance and resource utilization to be used in place of workload
    characteristics (i.e., since modeling moving a workload from A to B, so can
    make use of the performance information of the workload on A).

* Challenges in modelling:
  - Workload characterization: How to concisely describe a complex workload
    without losing necessary information. I.e., how do we compress the stream?
  - An absolute model doesn't capture the connection between a workload and the
    storage device on which it executes, assumes open world that `Wi = Wj`.
  - Which workload characteristics are important will vary from storage device,
    so must maintain the superset.
  - Hence must abandon absolute models!

* Use case:
  - Train models before deploying a new storage device.
  - Measure the characteristics, including the performance and resource
    utilization, of a workload on the device it was originally assinged to.
  - Use the model to predict performance of moving the workload to the new
    storage device.

* Related work:
  - Analytical: [17, 22, 24]
  - Statistical: [14]
  - ML: [27]
  - Table: [2]
  - White-box: [25]
  - Black-box: [2, 14, 19, 27]
  * Burstiness: [28]
  - Spatio-temporal locality: [26]

* Challenges in absolute modelling:
  - `Pi = Fi(WCi)` where `Pi` = performance characteristic, `Fi` = model for a
    particular storage device, `WCi` = workload characteristics.
  - Absolute in that the values for all the parameters above are isolated, they
    aren't relative to some other device.

* Relative fitness models:
  - Learn to predict scaling factors or performance ratios rather than
    performance itself.
  - E.g., device `i` is 30% faster than device `j` for random reads (regardless
    of request size or other characteristics).
  - Want to predict performanance of a workload running on device `i` when we
    have `WCj` for the workload running on device `j`.
  - 0) Absolute model: `Pi = Fi(WCi)`
  - 1) Predict `WCi` from `WCj`. We use a function indexed by `j` and `i`.
  - 2a) Now can apply: `Pi = Fi( G_{j->i}( WCj ) )`
  - 2b) But instead we compose F & G: `Pi = RM_{j->i}( WCj )`, to give a
    relative model RM indexed by `j` and `i`.
  - 3) Now use performance of device `j` (bandwidth, throughput, latency..) and
    utilization (cache utilization, hit/miss ratio, cpu utilization..):
    `Pi = RM_{j->i}( WCj, PERFj, UTILj )`
  - 4) Finally, predict performance ratios, giving a relative fitness model:
    `Pi / Pj = RF_{j->i}( WCj, PERFj, UTILj )`
  - Use (4) to solve for `Pi`: `Pi = (4) * Pj`
  - Unlike aboslute model, need to train a model per device pair.
  - Use of performance and utilization metrics allows relaxed capturing of
    workload characteristics.

* Performance & Utilization:
  - Appear to be the performance numbers (i.e., bandwidth) that the workload
    achieves (in isolation, no other workloads running on these storage
    devices) on storage device `j`.
  - So allows us to ask the question, what bandwidth will this workload have on
    storage device `i` if it has a bandwidth of `x` of storage device `j`?

* Fitness test: Synthetic workload generator.

* ML:
  - Supervised learning: have access to a set of predictor variables (WCj,
    PERFj, UTILj), as well as the desired response (predicted variables, e.g.,
    Pi). Algorithm needs to learn mapping between them.
  - "True values" are from our samples, our fitness test.
  - Classification vs. regression:
    - Classification: discrete-valued response variables.
    - Regression: real-valued response variables.
  - Paper uses a classification and regression tree (CART) model.

* CART:
  - Tree that at each node splits on a predictor variable.
  - CART model-building is to determine which order to split in (i.e., who
    should be root?). "Best" split is the one that minimizes differences (i.e.,
    range) in the leaf nodes (i.e., tightest clusters).
  - Most relevant predictor variables are used first, and less relevants ones
    to refine the predicted value.
  - A maximal tree (using all predictor variables) is first created, then
    pruned to eliminate over-fitting, with multiple prune-depths explored.
  - Optimal pruning depth is determined through cross-validation, where some
    reserved training data is used to test accuracy of pruned trees.
  - Leaf nodes contain predicted value (average of all samples in that leaf).

* Fitness test workload characteristic variance:

  ---------------------------------------------
        WC        |  A  |  B  |  C  | Diference
  ---------------------------------------------
  Write %         |  40 |  39 |  38 |   5.2%
  Write size (KB) |  61 |  61 |  61 |    0%
  Read size (KB)  |  40 |  41 |  41 |   2.5%
  Write seek (MB) | 321 | 250 | 233 |   38%
  Read seek (MB)  | 710 | 711 | 711 |    0%
  Queue depth     |  23 |  22 |  21 |   9.5%
  ---------------------------------------------

  - So big difference in write seek and essential no change in anything else.
  - Win of relative model comes from ability to use performance data, not much
    it seems from transforming the `WCj`.

* Synthetic results:
  - About a 4-10% improvement with the relative fitness model over an absolute.
  - Relative fitness model prunes (so 'generalizes') much better than all
    others (including a relative model). This is likely since a relative
    fitness model gives a scaling factor, so essentially a function as output.
    While the other three approaches have to give discrete values. So as you
    prune the tree, the discrete approach suffers far more than a functional
    approach.
  - Best prediction results for bandwidth, then throughput, then latency.

* App results (just throughput):
  - TPC-C:
    - Absolute model:   50-70% error
    - Relative model:   18-30% error
    - Relative fitness: 20-35% error

  - Postmark:
    - Absolute model:   30-60% error
    - Relative model:   25-50% error
    - Relative fitness: 20-30% error

