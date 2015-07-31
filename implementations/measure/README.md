# README

Several simple programs are included, used to test basic disk IO performance:

* `read`: read a file in serial.
* `write`: write a file in serial.
* `random_read`: read a file in random.
* `random_write`: write a file in random.
* `async_read`: read a file in serial, but implemented with kernel AIO
  interface.
* `async_read_random`: read a file in random, but implemented with kernel AIO
  interface.

## Using dd

The following commands are used to test raw disk performance:

* Read: `dd if=/dev/xvdb of=/dev/null bs=1M count=10240`
* Write: `dd if=/dev/zero of=/dev/xvdb bs=1M count=10240`

The same (`dd`) commands can also be used against a file to test disk
performance through the filesystem. However, you want an additional option to
ensure writes are synced:

* Write (fs): `dd if=/dev/zero of=/mnt/file bs=1M count=10240 conv=fdatasync`.

You may also want to consider the `nonblock` and `nocache` options for dd.

## Using iperf

The `iperf` tool can be used to measure throughput between two machines.

One one machine, run iperf in server mode:

`$ iperf -s -p 2000`

One another machine, run iperf in client mode:

`$ iperf -t 20 -p 2000 -i 1 -c <server>`

This will run iperf for 20 seconds (`-t`), printing the throughtput at 1
second (`-i`) intervals.

## Using nload

The `nload` tool allows you to monitor network throughput at a server. Just run
`nload` and watch the pretty graphs!

## Creating the file system

Command to format the file system:

```
sudo mkfs.ext4 -E nodiscard /dev/xvdb
```

This disables `discard` support, which is a feature that tries to perform some
block-zeroing / freeing work on all writes to avoid any latency spikes when
free-blocks are required for future writes and non are yet available.

May also want to consider the `-O ^has_journal` option to disable the journal.

## Initial AWS I2 Results

Initial results:
i2.xlarge
Read: 514MB/s
Write: 434MB/s

(Serial)
Write: 443MB/s
Read: 349MB/s
Async read: 473MB/s

(Random)
Write: 441MB/s
Read: 327MB/s
Async read: 472MB/s

i2.2xlarge
Read: 1025MB/s
Write: 863MB/s

i2.4xlarge
Read: 2035MB/s
Write: 1714MB/s

i2.8xlarge
Read: 3857MB/s
Write: 3247MB/s

(Serial)
Write: 3243MB/s 
Read: 3373MB/s
Async read: 3538MB/s

(Random)
Write: 3227MB/s
Read: 2535MB/s
Async read: 3618MB/s

