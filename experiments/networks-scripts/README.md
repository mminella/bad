# A test script for bandwidth and latency test on EC2

To run the test, we need to:
(1) edit the configuration in `config.py`,
(2) launch a set of servers within a placement group, by another script or
    manualy through web interface,
(3) run `test_network.py` with the given placement group name.

The script will build scripts, copy them to every machine, run them, and
collect results.

## Dependency

Running `test_network.py`
* Python 2
* boto package

Cluster:
* Iperf and rsync needs to be installed on every machine.

## Potential bug

The script assumes that launching script through ssh happens immediately, if it
is not the case, the script might not work well. It is generally fine.

