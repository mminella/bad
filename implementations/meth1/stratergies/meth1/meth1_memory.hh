#ifndef METH1_UTIL_HH
#define METH1_UTIL_HH

#include <cstddef>
#include <cstdint>

/* Get a count of how many disks this machine has */
uint64_t num_of_disks( void );

/* figure out max chunk size available */
uint64_t calc_record_space( void );

#endif /* METH1_UTIL_HH */
