# B.A.D Method 1 Implementation - Zero upfront

The reading node keeps track of the latest key it has read, starting with k=0.
When the user asks for the next 100-byte record, the reading node asks each of
the file servers for their earliest record after k. Each file server does a
linear scan through its local 500 GB for every read. Linear reads are very
slow; random reads are basically impossible.

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

