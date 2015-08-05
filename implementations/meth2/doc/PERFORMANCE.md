# Performance Notes

Place any notes here on performance findings.

## std::sort vs std::qsort

std::sort is faster, for 1GB of records times were:
* std::sort -- 2100ms
* std::qsort -- 2800ms

## std::sort vs boost::string_sort

* std::sort -- 2100ms
* boost:stringsort -- 600ms

Note that the more general, boost::sort gives same speed as std::sort.

## std::vector vs c-array

Slight speed up for sorting when using a raw C-array (i.e., Rec[ NRECS ]) vs
using a Vector. Suprising since a vector has a raw array beneath it, perhaps
while sorting there is some bounds-checking or indirection going on that
doesn't happen with a C-array.

For 1GB of records, times were:

* std::vector -- 2100ms
* c-array -- 1950ms

Or a 7% improvement.

## Packed vs. Aligned

Appears to be no slow-down for using a packed Record struct, and we gain a
small amount of memory savings. Believe this relies on recent Intel processors
that have good performance for unaligned reads.

