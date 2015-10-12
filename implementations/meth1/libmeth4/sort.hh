#ifndef METH4_SORT_HH
#define METH4_SORT_HH

#include <string>

#include "config.h"
#ifdef HAVE_TBB_TASK_GROUP_H
#include "tbb/task_group.h"
#endif

#include "socket.hh"

#include "cluster_map.hh"

class BucketSorter
{
private:
  const ClusterMap & cluster_;
  uint16_t bkt_;
  size_t len_;
  char * buf_;

public:
  BucketSorter( const ClusterMap & cluster, uint16_t bkt );
  BucketSorter( const BucketSorter & ) = delete;
  BucketSorter( BucketSorter && );
  ~BucketSorter( void );

  BucketSorter & operator=( const BucketSorter & ) = delete;
  BucketSorter & operator=( BucketSorter && ) = delete;

  uint16_t id( void ) const noexcept { return bkt_; }

  void loadBucket( void );
  void sortBucket( void );
  void saveBucket( void );
  void sendBucket( TCPSocket & sock, uint64_t records );
};

class Sorter
{
public:
  explicit Sorter( const ClusterMap & cluster, std::string op,
    std::string arg1 );
};

#endif /* METH4_SORT_HH */
