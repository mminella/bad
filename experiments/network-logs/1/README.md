# EC2 Network Measurement

Hang Qu:

"I did a set of experiments to test EC2 networking performance (mainly
bandwidth) scalability.

For different number of machines(2, 4, 8, 16, 32), I launched the corresponding
number of c3.8xlarge instances in the same placement group with SR-IOV enabled.
And then I launched two iperf TCP flows between each two machines
simultaneously `(N*(N-1)` flows for `N` machines), and let it run for 100
seconds.

This setting is the best networking setting. And I am testing how the machine
scale affects the networking performance.

It turns out when I increased the number of machines form 4 to 32, the average
outgoing bandwidth of each machine dropped gradually from ~9800Mbit/second to
~9400Mbit/second (the network link is 10Gbit ethernet). It seems that TCP is
able to achieve a relatively good aggregate bandwidth, although each flow
bandwidth differs as shown in `flow_distribution.png`.

I also attached the raw logs, in which you can find the flow bandwidth between
each two machines."

