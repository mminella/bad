#ifndef METH4_KNOBS_HH
#define METH4_KNOBS_HH

/**
 * We collect all the tuneable parameters for method 4 here.
 */

#include <cstdint>
#include <string>

/*
 * Queue Lengths:
 * A) (QL + 2) x #Disks.
 * B) (QL + 1 ) x #Disks + #NodeBuckets.
 * C) QL + 1 + #ClusterBuckets x #Disks.
 *
 * Buffer Sizes (now):
 * A) 2GB x #Disks.
 * B) 2GB x #Disks + #NodeBuckets x 10MB.
 * C) 4GB + #ClusterBuckets x #Disks x 1MB.
 *
 * Total Size (now):
 * T = A + B + C
 * T = 4GB + #NodeBuckets x 10MB + #Disks x ( 4GB + #ClusterBuckets x 1MB )
 */

namespace Knobs4 {
  /* A. Disk read queue length */
  static constexpr uint64_t DISK_R_QUEUE_LENGTH = 200; // ~ 2000MB

  /* B. Disk block size (writing) [* Rec::SIZE] */
  static constexpr size_t DISK_W_QUEUE_LENGTH = 200; // ~ 2000MB
  static constexpr size_t DISK_W_BLOCK_SIZE = 1024 * 102; // ~ 10MB

  /* C. Network queue length & block transfer size [* Rec::SIZE] */
  static constexpr size_t NET_QUEUE_LENGTH = 2000; // ~ 4000MB
  static constexpr size_t NET_BLOCK_SIZE = 10240; // ~ 1MB

  /* Network send & receive kernel buffer sizes */
  static constexpr size_t NET_SND_BUF = std::size_t( 1024 ) * 1024 * 2;
  static constexpr size_t NET_RCV_BUF = std::size_t( 1024 ) * 1024 * 2;

  /* Use non-blocking IO on the phase-1 receive side? */
  static constexpr bool NET_NON_BLOCKING = true;

  /* Minimum number of buckets to have per disk */
  static constexpr size_t MIN_BUCKETS_PER_DISK = 2;

  /* Memory to leave unused for OS and other misc purposes. */
  static constexpr uint64_t MEM_RESERVE = 1024 * 1024 * uint64_t( 2000 );

  /* Seconds to wait after startup before trying to connect to other nodes */
  static constexpr size_t STARTUP_WAIT = 10;
}

#endif /* METH4_KNOBS_HH */
