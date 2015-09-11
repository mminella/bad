We tested how well multiple disks can be used for the
"first-record" operation. This used 98GB of data to search in total, divided
evenly among the available disks.

i2  | time (s) | GB/disk | MB/s  | MB/s/disk
--------------------------------------------
 1  |   216    |   98    |   453 |   453
 2  |   109    |   49    |   899 |   450
 4  |    55    |   24.50 | 1,782 |   446
 8  |    33    |   12.25 | 2,970 |   371

