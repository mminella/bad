#ifndef METH4_SORT_HH
#define METH4_SORT_HH

#include "config.h"
#ifdef HAVE_TBB_TASK_GROUP_H
#include "tbb/task_group.h"
#endif

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

  void loadBucket( void );
  void sortBucket( void );
  void saveBucket( void );
};

class Sorter
{
public:
  Sorter( const ClusterMap & cluster );
};

#endif /* METH4_SORT_HH */
