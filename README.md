# B.A.D Project

http://sortbenchmark.org/

My thinking was that it would be cool to some up with a system that can
discover/construct various solutions to a dataflow problem (e.g. a sort or
parallel join), based on a user's preference for upfront computation time vs.
throughput at reading the resulting sorted file (and whether they're going to
read it linearly on a single node, linearly in parallel from multiple nodes,
randomly, etc.) vs. cost.

Here are sort of some straw-man strategies for the 100 TB sort, given 200
i2.8xlarge instances:

1) "Zero computation upfront." The reading node keeps track of the latest key
it has read, starting with k=0. When the user asks for the next 100-byte
record, the reading node asks each of the file servers for their earliest
record after k. Each file server does a linear scan through its local 500 GB
for every read. Linear reads are very slow; random reads are basically
impossible.

2) "Each file server builds a local index." Upfront, the file servers each
scan through their local 500 GB and construct an in-RAM sorted array of each
10-byte key and the byte position of its payload on disk. This can fit in 39
bits, so the whole record can be 16 bytes, so the in-RAM array is a mere 75
GiB -- easy! Then proceed per #1, except we get rid of the linear scans. In
theory the upfront time should be just the time to read all those keys and
sort them in RAM, and the linear read throughput is probably limited to total
cluster IOPS * 100 bytes, which is plenty to saturate a single reading node.
Random reads are more painful but not impossible.

3) "Build a sorted distributed index." Same as #2, except we merge and sort
the indexes so each node gets a contiguous mapping of the keyspace. (Maybe
using some sort of in-memory radix sort in parallel with the keys arriving?)
Now random reads are basically doable.

4) "Actually move the payloads." This may be the only way to get linear read
performance up to the total cluster IOPS * 4096 bytes.

There are probably lots more solutions that a computer could discover,
especially as we start talking about cost. I think it would be fun to map
these tradeoffs, targeting NSDI 2016 and the Sept. 1 sortbenchmark deadline.

Eager to hear your thoughts (we have barely started on this -- one of the
students who is interested is crashing on his own submission deadline due in
about a week) and look forward to speaking again.

Cheers,
Keith
