#ifndef BAD_MODELS_METHOD2_H_
#define BAD_MODELS_METHOD2_H_

#include <algorithm>

#include "machines.hh"
#include "model.hh"

/**
 * B.A.D Model for Method 2 -- Shuffle All.
 *
 * ASSUMPTIONS:
 * - Uniform key distribution.
 *
 * CHOICES:
 * - We aren't including the cost of the client machine.
 */
namespace Method2
{
  // 8 byte point + 2 bytes disk / host ID
  constexpr uint64_t kIndexSize = kKeySize + 10;

  inline uint64_t MinNodes(Machine m, uint64_t data)
  {
    uint64_t diskreq = std::ceil(double(data) / (m.disk_size * m.disks));
    uint64_t index   = (data / kRecSize) * kIndexSize;
    uint64_t memreq  = std::ceil(double(index) / m.memory);
    return std::max(diskreq, memreq);
  }

  Prediction ReadFirst(Machine client, Machine backend, uint32_t nodes,
                       uint64_t data, bool to_client, bool start = true);
  Prediction ReadRange(Machine client, Machine backend, uint32_t nodes,
                       uint64_t data, bool to_client, uint64_t offset,
                       uint64_t len, bool start = true);
  Prediction ReadAll(Machine client, Machine backend, uint32_t nodes,
                     uint64_t data, bool to_client, bool start = true);
  Prediction CDF(Machine client, Machine backend, uint32_t nodes,
                 uint64_t data, bool to_client, uint64_t points, bool start = true);
  Prediction ReservoirSample(Machine client, Machine backend, uint32_t nodes,
                             uint64_t data, bool to_client, uint64_t samples,
                             bool start = true);
}

#endif // BAD_MODELS_METHOD2_H_
