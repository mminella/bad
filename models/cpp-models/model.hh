#ifndef BAD_MODELS_MODEL_H_
#define BAD_MODELS_MODEL_H_

#include <algorithm>
#include <cstdint>
#include <stdexcept>

#include "machines.hh"

constexpr uint64_t kMB      = 1024 * 1024;
constexpr uint64_t kRecSize = 100;
constexpr uint64_t kKeySize = 10;
constexpr uint64_t kValSize = 90;

struct Prediction
{
  uint64_t time_total;
  uint64_t time1;      // Meth1: disk, Meth4: shuffle
  uint64_t time2;      // Meth1:  net, Meth4: sort
  uint64_t cost;
};

inline uint64_t DataAtNode(Machine m, uint64_t nodes, uint32_t data,
    double multiplier = 1.0)
{
  uint64_t nodedata = ceil(double(data) / double(nodes));
  if ((nodedata * multiplier) > (m.disk_size * m.disks)) {
    throw std::runtime_error("Not enough disk space for data set");
  }
  return nodedata;
}

inline uint64_t SequentialRead(Machine m, uint64_t data, uint64_t disks = 0)
{
  if (disks == 0) {
    return data / (m.disk_read * m.disks);
  } else {
    return data / (m.disk_write * disks);
  }
}

inline uint64_t SequentialWrite(Machine m, uint64_t data, uint64_t disks = 0)
{
  if (disks == 0) {
    return data / (m.disk_write * m.disks);
  } else {
    return data / (m.disk_write * disks);
  }
}

inline uint64_t RandomRead(Machine m, uint64_t ios, uint64_t data)
{
  uint64_t rand_t = ios / (m.disk_iops_r * m.disks);
  uint64_t seq_t  = data / (m.disk_read * m.disks);
  return std::max(rand_t, seq_t);
}

inline uint64_t NetworkSend(Machine out, uint64_t outn,
                            Machine in, uint64_t inn, uint64_t data)
{
  uint64_t outspeed = out.netio * outn;
  uint64_t inspeed  = in.netio * inn;
  uint64_t netspeed = std::min(outspeed, inspeed);

  return data / netspeed;
}

inline uint64_t NetworkOut(Machine m, uint64_t data)
{
  return data / m.netio;
}

inline uint64_t NetworkIn(Machine m, uint64_t data)
{
  return data / m.netio;
}

inline uint64_t TimeToCost(Machine m, uint32_t nodes, uint64_t time_s)
{
  uint64_t time_m = time_s / 60;
  time_m = std::max(uint64_t(m.bill_mintime), time_m);
  time_m = std::ceil(double(time_m) / double(m.bill_granularity));
  return time_m * m.cost * nodes;
}

#endif // BAD_MODELS_MODEL_H_
