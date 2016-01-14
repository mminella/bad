#include "method4.hh"

#include <algorithm>
#include <stdexcept>

using namespace std;

// Do we sort buckets lazily as needed? This optimization should improve the
// time for operations that don't touch all buckets, without hurting
// performance for those that do.
constexpr bool kLazy               = true;
constexpr uint64_t kMemReserve     = 500 * kMB;
constexpr uint64_t kBucketN        = 2;

uint64_t BucketSize(Machine m, uint32_t nodes, uint64_t data)
{
  uint64_t nodedata = DataAtNode(m, nodes, data, Method4::kDataMultiplier);
  uint64_t diskdata = ceil(double(nodedata) / m.disks);
  uint64_t bucket   = m.memory - kMemReserve / m.disks / kBucketN;
  bucket = (bucket / kRecSize) * kRecSize;

  return min(bucket, diskdata / kBucketN);
}

uint64_t ShuffleAll(Machine m, uint32_t nodes, uint64_t data)
{
  uint64_t diskr = data / (m.disk_read * m.disks);
  uint64_t net   = ((nodes - 1) * data / (nodes * m.netio));
  uint64_t diskw = data / (m.disk_read * m.disks);

  return max(diskr, max(net, diskw));
}

uint64_t StartupTime(Machine m, uint32_t nodes, uint64_t data)
{
  uint64_t nodedata = DataAtNode(m, nodes, data, Method4::kDataMultiplier);
  return ShuffleAll(m, nodes, nodedata);
}

// Transfer a data to a single machine
uint64_t BucketNetSend(Machine client, Machine backend, uint32_t nodes,
    uint64_t data, uint32_t len)
{
  // bucket sizing and layer / sharding
  uint64_t range   = len * kRecSize;
  uint64_t bucket  = BucketSize(backend, nodes, data);
  uint64_t layer   = nodes * backend.disks;
  uint64_t bktsout = range / bucket;

  // maximum amount of buckets to transfer from any one backend node
  uint64_t layers  = bktsout / layer;
  uint64_t parts   = bktsout == layer ? 0 : 1;
  uint64_t netbyte = (layers + parts) * bucket;
  uint64_t netout  = NetworkOut(backend, netbyte);

  // ingres time to a single client
  uint64_t netin = NetworkIn(client, range);

  return max(netout, netin);
}

Prediction ReadRangeS(Machine client, Machine backend, uint32_t nodes,
    uint64_t data, bool to_client, uint64_t /* offset */, uint64_t len)
{
  // Shuffle + Sort
  uint64_t nodedata = DataAtNode(backend, nodes, data,
                                 Method4::kDataMultiplier);
  uint64_t start    = StartupTime(backend, nodes, data);
  uint64_t diskr    = SequentialRead(backend, nodedata);
  uint64_t diskw    = SequentialWrite(backend, nodedata);

  // Transfer Range to one client
  uint64_t optime = 0;
  if (to_client) {
      uint64_t net = BucketNetSend(client, backend, nodes, data, len);
      optime = diskr + diskw + net;
    } else {
      optime = diskr + diskw;
    }

  uint64_t time = start + optime;
  uint64_t cost = TimeToCost(backend, nodes, time);
  return {time, start, optime, cost};
}

Prediction ReadRangeL(Machine client, Machine backend, uint32_t nodes,
    uint64_t data, bool to_client, uint64_t /* offset */, uint64_t len)
{
  // How many buckets do we need to sort? The implementation round-robbins each
  // bucket over the nodes and their disks to achieve maximum parallel read
  // throughput for a ranged read. So a 'layer' (before we repeat the same
  // machines disk) is (nodes * disks) buckets in size.
  uint64_t range   = len * kRecSize;
  uint64_t bucket  = BucketSize(backend, nodes, data);
  uint64_t layer   = nodes * backend.disks;
  uint64_t bktsout = range / bucket;

  // maximum amount of buckets to transfer from any one backend disk
  uint64_t layers = ceil(double(bktsout) / layer);
  uint64_t sortsz = layers * bucket;

  // Startup + Sort
  uint64_t start = StartupTime(backend, nodes, data);
  uint64_t diskr = SequentialRead(backend, sortsz, 1);
  uint64_t diskw = SequentialWrite(backend, sortsz, 1);

  // Network for phase 2
  uint64_t optime = 0;
  if (to_client) {
    uint64_t net = BucketNetSend(client, backend, nodes, data, len);
    optime = diskr + max(diskw, net);
  } else {
    optime = diskr + diskw;
  }

  uint64_t time = start + optime;
  uint64_t cost = TimeToCost(backend, nodes, time);
  return {time, start, optime, cost};
}

Prediction CDFS(Machine client, Machine backend, uint32_t nodes, uint64_t data,
    bool to_client, uint32_t points)
{
  Prediction p = Method4::ReadAll(client, backend, nodes, data, false);
  if (to_client) {
    uint64_t net = NetworkSend(backend, nodes, client, 1, points * kRecSize);
    p.time_total += net;
    p.time2 += net;
  }
  return p;
}

Prediction CDFL(Machine client, Machine backend, uint32_t nodes, uint64_t data,
    bool to_client, uint64_t points)
{
  // How many buckets on a single disk do we need to touch? Only need to sort
  // buckets that contain needed points
  uint64_t bucket  = BucketSize(backend, nodes, data);
  uint64_t buckets = data / bucket;
  uint64_t layer   = nodes * backend.disks;

  uint64_t touched = min(points, buckets);
  uint64_t perdisk = max(uint64_t(1), touched / layer);
  uint64_t bktdata = perdisk * bucket;

  // Startup + Sort
  uint64_t start = StartupTime(backend, nodes, data);
  uint64_t diskr = SequentialRead(backend, bktdata, 1);
  uint64_t diskw = SequentialWrite(backend, bktdata, 1);
  // ^ XXX: There is a design choice here, we could not write back to disk

  // Network for phase 2
  uint64_t optime = 0;
  if (to_client) {
    uint64_t net = NetworkSend(backend, nodes, client, 1, points * kRecSize);
    optime = diskr + max(diskw, net);
  } else {
    optime = diskr + diskw;
  }

  uint64_t time = start + optime;
  uint64_t cost = TimeToCost(backend, nodes, time);
  return {time, start, optime, cost};
}

namespace Method4
{
  Prediction ReadFirst(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool /* to_client */)
  {
    if (not kLazy) {
      return ReadAll(client, backend, nodes, data, false);
    }

    // Only need to sort one bucket, so cheaper than ReadAll
    uint64_t bktsize = BucketSize(backend, nodes, data);
    uint64_t start   = StartupTime(backend, nodes, data);
    uint64_t diskr   = SequentialRead(backend, bktsize, 1);
    uint64_t diskw   = SequentialWrite(backend, bktsize, 1);
    // ^ XXX: There is a design choice here, we could not write back to disk

    uint64_t optime = diskr + diskw;
    uint64_t time   = start + optime;
    uint64_t cost   = TimeToCost(backend, nodes, time);
    return {time, start, optime, cost};
  }

  Prediction ReadRange(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool to_client,
      uint64_t offset, uint64_t len)
  {
    if (kLazy) {
      return ReadRangeL(client, backend, nodes, data, to_client, offset, len);
    } else {
      return ReadRangeS(client, backend, nodes, data, to_client, offset, len);
    }
  }

  Prediction ReadAll(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool to_client)
  {
    uint64_t nodedata = DataAtNode(backend, nodes, data,
                                   Method4::kDataMultiplier);
    uint64_t start    = StartupTime(backend, nodes, data);
    uint64_t diskr    = SequentialRead(backend, nodedata);
    uint64_t diskw    = SequentialWrite(backend, nodedata);

    uint64_t p2time = 0;
    if (to_client) {
      uint64_t net = NetworkSend(backend, nodes, client, 1, data);
      p2time = diskr + max(diskw, net);
    } else {
      p2time = diskr + diskw;
    }

    uint64_t time = start + p2time;
    uint64_t cost = TimeToCost(backend, nodes, time);
    return {time, start, p2time, cost};
  }

  Prediction CDF(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool to_client, uint64_t points)
  {
    if (kLazy) {
      return CDFL(client, backend, nodes, data, to_client, points);
    } else {
      return CDFS(client, backend, nodes, data, to_client, points);
    }
  }

  Prediction ReservoirSample(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool to_client, uint64_t samples)
  {
    return ReadRange(client, backend, nodes, data, to_client, 0, samples);
  }
}
