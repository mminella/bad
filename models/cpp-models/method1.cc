#include "method1.hh"

#include <algorithm>
#include <stdexcept>

using namespace std;

constexpr uint64_t kMemReserve = 0;
constexpr uint64_t kNetBuffer  = 500 * kMB;
constexpr uint64_t kDiskBuffer = 4000 * kMB;
constexpr uint64_t kSortRatio  = 14;
constexpr uint64_t kRecPtrSize = 18;
constexpr uint64_t kRecAligned = 112;

uint64_t ChunkSize(uint64_t mem, uint64_t disks)
{
  mem = mem - kMemReserve;
  mem = mem - kNetBuffer * 2;
  mem = mem - kDiskBuffer * disks;

  uint64_t div1 = 2 * kRecPtrSize + kRecAligned;
  uint64_t div2 = (kRecPtrSize + kRecAligned) / kSortRatio;

  return (mem / (div1 + div2)) * kRecSize;
}

namespace Method1
{
  Prediction ReadFirst(Machine /* client */, Machine backend,
      uint32_t nodes, uint64_t data)
  {
    uint64_t nodedata = DataAtNode(backend, nodes, data);
    uint64_t timedisk = SequentialRead(backend, nodedata);
    uint64_t cost     = TimeToCost(backend, nodes, timedisk);

    return {timedisk, timedisk, 0, cost};
  }

  Prediction ReadRange(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, uint64_t offset, uint64_t len)
  {
    // XXX: Since assuming normal distribution we're assuming for a range read
    // of length `len`, we retrieve `len / nodes` from each node.

    uint64_t rangedata = (offset + len) * kRecSize;
    uint64_t chunk     = ChunkSize(backend.memory, backend.disks);
    uint64_t nodedata  = DataAtNode(backend, nodes, data);
    uint64_t scans     = ceil(double(rangedata) / chunk / nodes);
    scans = max(uint64_t(1), scans);

    uint64_t timenet  = NetworkSend(backend, nodes, client, 1, rangedata);
    uint64_t timedisk = SequentialRead(backend, nodedata) * scans;
    uint64_t time     = timenet + timedisk;
    uint64_t cost     = TimeToCost(backend, nodes, time);

    return {time, timedisk, timenet, cost};
  }

  Prediction ReadAll(Machine client, Machine backend,
      uint32_t nodes, uint64_t data)
  {
    uint64_t chunk     = ChunkSize(backend.memory, backend.disks);
    uint64_t nodedata  = DataAtNode(backend, nodes, data);
    uint64_t scans     = ceil(double(nodedata) / chunk);

    uint64_t timenet  = NetworkSend(backend, nodes, client, 1, data);
    uint64_t timedisk = SequentialRead(backend, nodedata) * scans;
    uint64_t time     = timenet + timedisk;
    uint64_t cost     = TimeToCost(backend, nodes, time);

    return {time, timedisk, timenet, cost};
  }

  Prediction CDF(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, uint64_t /* points */)
  {
    return ReadAll(client, backend, nodes, data);
  }

  Prediction ReservoirSample(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, uint64_t samples)
  {
    return ReadRange(client, backend, nodes, data, 0, samples);
  }
}
