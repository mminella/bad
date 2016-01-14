#ifndef BAD_MODELS_METHOD4_H_
#define BAD_MODELS_METHOD4_H_

#include <algorithm>

#include "machines.hh"
#include "model.hh"

/**
 * B.A.D Model for Method 4 -- Shuffle All.
 *
 * ASSUMPTIONS:
 * - Uniform key distribution.
 *
 * CHOICES:
 * - We aren't including the cost of the client machine.
 */
namespace Method4
{
  constexpr uint64_t kDataMultiplier = 3;

  inline uint64_t MinNodes(Machine m, uint64_t data)
  {
    return ceil(double(data * kDataMultiplier) / (m.disk_size * m.disks));
  }

  Prediction ReadFirst(Machine client, Machine backend, uint32_t nodes,
                       uint64_t data, bool to_client);
  Prediction ReadRange(Machine client, Machine backend, uint32_t nodes,
                       uint64_t data, bool to_client, uint64_t offset,
                       uint64_t len);
  Prediction ReadAll(Machine client, Machine backend, uint32_t nodes,
                     uint64_t data, bool to_client);
  Prediction CDF(Machine client, Machine backend, uint32_t nodes,
                 uint64_t data, bool to_client, uint64_t points);
  Prediction ReservoirSample(Machine client, Machine backend, uint32_t nodes,
                             uint64_t data, bool to_client, uint64_t samples);
}

#endif // BAD_MODELS_METHOD4_H_
