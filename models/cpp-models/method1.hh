#ifndef BAD_MODELS_METHOD1_H_
#define BAD_MODELS_METHOD1_H_

#include <algorithm>

#include "machines.hh"
#include "model.hh"

/**
 * B.A.D Model for Method 1 -- Linear Scan.
 *
 * LIMITATIONS:
 * - We assume reads at backends will perfectly overlap. While not too
 * unreasonable with a uniform key distribution, at larger cluster sizes (both
 * due to more machines, and since the network buffer per backend node at a
 * client is by neccessity smaller, this assumption fails).
 *
 * ASSUMPTIONS:
 * - Uniform key distribution.
 *
 * CHOICES:
 * - We aren't including the cost of the client machine.
 */
namespace Method1
{
  inline uint64_t MinNodes(Machine m, uint64_t data)
  {
    return ceil(double(data) / (m.disk_size * m.disks));
  }

  Prediction ReadFirst(Machine client, Machine backend, uint32_t nodes,
                       uint64_t data);
  Prediction ReadRange(Machine client, Machine backend, uint32_t nodes,
                       uint64_t data, uint64_t offset, uint64_t len);
  Prediction ReadAll(Machine client, Machine backend, uint32_t nodes,
                     uint64_t data);
  Prediction CDF(Machine client, Machine backend, uint32_t nodes,
                 uint64_t data, uint64_t points);
  Prediction ReservoirSample(Machine client, Machine backend, uint32_t nodes,
                             uint64_t data, uint64_t samples);
}

#endif // BAD_MODELS_METHOD1_H_
