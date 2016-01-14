#include "method2.hh"

#include <algorithm>
#include <stdexcept>

using namespace std;

uint64_t StartupTime(Machine backend, uint32_t nodes, uint64_t data)
{
  uint64_t nodedata = DataAtNode(backend, nodes, data);
  uint64_t load_t   = SequentialRead(backend, nodes, nodedata);
  // XXX: Old sort method was slow and nearly linear, Need to plugin new
  // numbers for sort
  uint64_t sort_t   = load_t;
  return load_t + sort_t;
}

namespace Method2
{
  Prediction ReadFirst(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool /* to_client */, bool start)
  {
    uint64_t index_t = start ? StartupTime(backend, nodes, data) : 0;
    uint64_t disk_t  = RandomRead(backend, 1, kRecSize);
    uint64_t net_t   = NetworkSend(backend, nodes, client, 1, kRecSize * nodes);

    uint64_t time    = index_t + disk_t + net_t;
    uint64_t cost    = TimeToCost(backend, nodes, time);
    return {time, index_t, disk_t + net_t, cost};
  }

  Prediction ReadRange(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool to_client, uint64_t /* offset */,
      uint64_t len, bool start)
  {
    uint64_t clients        = to_client ? 1 : nodes;
    uint64_t nodedata       = DataAtNode(backend, nodes, data);
    uint64_t range_per_node = max(uint64_t(1), len / nodes);

    // Search orchestrated by client requires approximately log requests
    // TODO: search_qs appears to be our largest source of error
    double search_qs  = log2(nodedata / kRecSize);
    uint64_t index_t  = start ? StartupTime(backend, nodes, data) : 0;
    uint64_t search_t = (search_qs * client.rtt) / 1000;
    uint64_t disk_t   = RandomRead(backend, range_per_node,
                                   range_per_node * kRecSize);
    uint64_t net_t    = NetworkSend(backend, nodes, client, clients,
                                    kRecSize * nodes);

    uint64_t time  = index_t + search_t + max(disk_t, net_t);
    uint64_t cost  = TimeToCost(backend, nodes, time);
    return {time, index_t, disk_t + net_t, cost};
  }

  Prediction ReadAll(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool to_client, bool start)
  {
    uint64_t clients  = to_client ? 1 : nodes;
    uint64_t nodedata = DataAtNode(backend, nodes, data);

    uint64_t index_t = start ? StartupTime(backend, nodes, data) : 0;
    uint64_t disk_t  = RandomRead(backend, nodedata / kRecSize, nodedata);
    uint64_t net_t   = NetworkSend(backend, nodes, client, clients, data);

    uint64_t time = index_t + max(disk_t, net_t);
    uint64_t cost = TimeToCost(backend, nodes, time);
    return {time, index_t, disk_t + net_t, cost};
  }

  Prediction CDF(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool to_client, uint64_t points,
      bool start)
  {
    uint64_t index_t = start ? StartupTime(backend, nodes, data) : 0;

    uint64_t op_t = 0;
    for (uint64_t i = 0; i < points; i++) {
      uint64_t offset = (double(i) / points) * (data / kRecSize);
      Prediction p = ReadRange(client, backend, nodes, data, to_client, offset,
                               1, false);
      op_t += p.time_total;
    }

    uint64_t time = index_t + op_t;
    uint64_t cost = TimeToCost(backend, nodes, time);
    return {time, index_t, op_t, cost};
  }

  Prediction ReservoirSample(Machine client, Machine backend,
      uint32_t nodes, uint64_t data, bool to_client, uint64_t samples,
      bool /* start */)
  {
    return ReadRange(client, backend, nodes, data, to_client, 0, samples);
  }
}

