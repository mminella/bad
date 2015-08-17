# B.A.D Method 2 Implementation - Local Index

Upfront, the file servers each scan through their local 500 GB and construct an
in-RAM sorted array of each 10-byte key and the byte position of its payload on
disk. This can fit in 39 bits, so the whole record can be 16 bytes, so the
in-RAM array is a mere 75 GiB -- easy! Then proceed per #1, except we get rid
of the linear scans. In theory the upfront time should be just the time to read
all those keys and sort them in RAM, and the linear read throughput is probably
limited to total cluster IOPS * 100 bytes, which is plenty to saturate a single
reading node. Random reads are more painful but not impossible.

## Building

``` sh
./autogen.sh
./configure
make
```

## Authors

This library is written and maintained by David Terei <dterei@cs.stanford.edu>.

## Licensing

This library is BSD-licensed.

