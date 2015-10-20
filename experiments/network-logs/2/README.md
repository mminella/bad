# Network Test 2

Date: August 3rd, 2015.

## Description

Bandwidth is tested by 'iperf -t 60', and latency is tested by NetPipe. I
launched 66 c4.8xlarge servers in two placement groups with 33 servers each.

The basic conclusion is:
* Bandwidth/latency between intra-rack servers or inter-rack servers is similar.
* Inter-rack 1-to-32 communication can achieve full bandwidth.
* Inter-rack 32-to-32 communication can achieve full bandwidth.

## Files

* `inter_rack.bandwidth/latency`: test for two servers of different racks.
* `intra_rack.bandwidth/latency`: test for two servers of the same rack.
* `machines.profile`: the profile of the used servers.
* `one_to_32_else.bandwidth`: the bandwidth when one server sends to 32 servers
  in the other rack at the same time.
* `inter_32_to_32`: each server in one rack sends to every 32 servers in another rack.
