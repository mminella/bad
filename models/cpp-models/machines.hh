#ifndef BAD_MODELS_MACHINES_H_
#define BAD_MODELS_MACHINES_H_

#include <cstdint>
#include <string>
#include <vector>

/**
 * Machine learned parameters for Amazon AWS EC2 and Google Cloud Compute
 * services.
 *
 * TODO: Psort numbers for GCloud
 */
struct Machine
{
  std::string type;
  double      memory;
  uint64_t    disk_size;
  uint64_t    disks;
  uint64_t    disk_read;
  uint64_t    disk_write;
  uint64_t    disk_iops_r;
  uint64_t    netio;
  double      psort;
  double      rtt;
  double      cost;
  uint64_t    bill_granularity;
  uint64_t    bill_mintime;
};

const std::vector<Machine> machines_aws{
  // type, mem, disk.size, disks, diskio.r, diskio.w, iops.r, netio, psort, rtt, cost, billing.granularity, billing.mintime
  {"i2.1x",  29.96, 800, 1, 450, 450, 60340,   82, 0.134, 0.149, 0.853, 60, 60},
  {"i2.2x",  60.00, 800, 2, 450, 450, 60340,  125, 0.268, 0.149, 1.705, 60, 60},
  {"i2.4x", 120.07, 800, 4, 450, 450, 60340,  250, 0.536, 0.149, 3.410, 60, 60},
  {"i2.8x", 240.23, 800, 8, 450, 450, 60340, 1200, 1.072, 0.149, 6.820, 60, 60},
};

const std::vector<Machine> machines_aws_advertized{
  // type, mem, disk.size, disks, diskio.r, diskio.w, iops.r, netio, psort, rtt, cost, billing.granularity, billing.mintime
  { "i2.1x",  30.5, 800, 1, 137, 137, 35000,   82, 0.134, 0.149, 0.853, 60, 60},
  { "i2.2x",  61.0, 800, 2, 147, 147, 37500,  125, 0.268, 0.149, 1.705, 60, 60},
  { "i2.4x", 122.0, 800, 4, 171, 151, 43750,  250, 0.536, 0.149, 3.410, 60, 60},
  { "i2.8x", 244.0, 800, 8, 178, 154, 45625, 1200, 1.072, 0.149, 6.820, 60, 60},
};


const std::vector<Machine> machines_aws_min{
  // type, mem, disk.size, disks, diskio.r, diskio.w, iops.r, netio, psort, rtt, cost, billing.granularity, billing.mintime
  { "i2.1x",  29.6, 800, 1, 137, 137, 35000,   80, 0.120, 0.2, 0.853, 60, 60},
  { "i2.2x",  60.0, 800, 2, 147, 147, 35000,  120, 0.240, 0.2, 1.705, 60, 60},
  { "i2.4x", 120.0, 800, 4, 171, 151, 35000,  240, 0.480, 0.2, 3.410, 60, 60},
  { "i2.8x", 240.0, 800, 8, 178, 154, 35000, 1000, 0.960, 0.2, 6.820, 60, 60},
};


const std::vector<Machine> machines_google{
  // type, mem, disk.size, disks, diskio.r, diskio.w, iops.r, psort, netio, rtt, cost, billing.granularity, billing.mintime
  { "n1-highmem-2",   13, 375, 1, 524, 367, 160000,  498, 0, 0.5, 0.0039, 1, 10},
  { "n1-highmem-4",   26, 375, 2, 524, 367, 160000,  980, 0, 0.5, 0.0079, 1, 10},
  { "n1-highmem-8",   52, 375, 4, 524, 367, 160000, 1100, 0, 0.5, 0.0159, 1, 10},
  { "n1-highmem-16", 104, 375, 4, 524, 367, 160000, 1125, 0, 0.5, 0.0243, 1, 10},
  { "n1-highmem-32", 208, 375, 4, 524, 367, 160000, 1150, 0, 0.5, 0.0411, 1, 10},
};

const std::vector<Machine> machines_google_advertized{
  // type, mem, disk.size, disks, diskio.r, diskio.w, iops.r, netio, psort, rtt, cost, billing.granularity, billing.mintime
  { "n1-highmem-2",   13, 375, 1, 663, 352, 170000,  500, 0, 0.5, 0.0039, 1, 10},
  { "n1-highmem-4",   26, 375, 2, 663, 352, 170000, 1000, 0, 0.5, 0.0079, 1, 10},
  { "n1-highmem-8",   52, 375, 4, 663, 352, 170000, 1250, 0, 0.5, 0.0159, 1, 10},
  { "n1-highmem-16", 104, 375, 4, 663, 352, 170000, 1250, 0, 0.5, 0.0243, 1, 10},
  { "n1-highmem-32", 208, 375, 4, 663, 352, 170000, 1250, 0, 0.5, 0.0411, 1, 10},
};

const std::vector<Machine> machines_google_min{
  // type, mem, disk.size, disks, diskio.r, diskio.w, iops.r, netio, psort, rtt, cost, billing.granularity, billing.mintime
  { "n1-highmem-2",   13, 375, 1, 374, 312, 90000, 450, 0, 1.0, 0.0039, 1, 10},
  { "n1-highmem-4",   26, 375, 2, 374, 312, 90000, 900, 0, 1.0, 0.0079, 1, 10},
  { "n1-highmem-8",   52, 375, 4, 374, 312, 90000, 900, 0, 1.0, 0.0159, 1, 10},
  { "n1-highmem-16", 104, 375, 4, 374, 312, 90000, 900, 0, 1.0, 0.0243, 1, 10},
  { "n1-highmem-32", 208, 375, 4, 374, 312, 90000, 900, 0, 1.0, 0.0411, 1, 10},
};

#endif // BAD_MODELS_MACHINES_H_
