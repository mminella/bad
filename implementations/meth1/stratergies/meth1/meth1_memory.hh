#ifndef METH1_UTIL_HH
#define METH1_UTIL_HH

#include <cstddef>
#include <cstdint>

/* Amount of memory to reserve for OS and other tasks */
#define MEMRESERVE 1024 * 1024 * 1000

/* Get a count of how many disks this machine has */
size_t num_of_disks( void );

/* figure out max chunk size available */
uint64_t calc_record_space( void );

#endif /* METH1_UTIL_HH */
