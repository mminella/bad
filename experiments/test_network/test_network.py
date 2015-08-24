#!/usr/bin/python
import boto.ec2
import subprocess
import sys
import config
import time

def FindMachines(region_name, placement_group):
    """Return the ip addresses of all the servers in one region.
    """
    connection = boto.ec2.connect_to_region(region_name)
    assert connection, 'Connection failed.'
    addresses = [(instance.ip_address, instance.private_ip_address)
                 for instance in connection.get_only_instances()
                 if instance.state == 'running'
                 and instance.placement_group == placement_group]
    print 'Testing on {} running servers.'.format(len(addresses))
    return addresses

def RunSerialEveryServer(addresses, command):
    for public_address, _ in addresses:
        subprocess.check_call(
                'ssh {user}@{ip} -i {key_location} {options} {}'.format(
                    command,
                    ip=public_address,
                    user=config.user,
                    key_location=config.key_location,
                    options=config.ssh_options),
                shell=True)

def RunParallelEveryServer(addresses, command):
    status_list = [
        subprocess.Popen(
                'ssh {user}@{ip} -i {key_location} {options} {}'.format(
                    command,
                    user=config.user,
                    ip=public_address,
                    key_location=config.key_location,
                    options=config.ssh_options),
                shell=True)
        for public_address, _ in addresses]
    for status in status_list:
        status.wait()

def BuildPingScript(addresses):
    ping_script = open('ping.sh', 'w')
    for index, (_, private_address) in enumerate(addresses):
        ping_script.write(
                'ping -c {} {} > latency_{}.log &\n'.format(
                    config.ping_times, private_address, index))
    ping_script.close()

def DistributePingScript(addresses):
    status_list = [
        subprocess.Popen(
                'rsync -e "ssh -i {key_location} {options}" -arz'
                ' ping.sh {user}@{ip}:~/test_script/'.format(
                    ip=public_address,
                    user=config.user,
                    key_location=config.key_location,
                    options=config.ssh_options),
                shell=True)
        for public_address, _ in addresses]
    for status in status_list:
        status.wait()

def BuildIperfScript(addresses):
    for index, _ in enumerate(addresses):
        iperf_script = open('iperf_{}.sh'.format(index), 'w')
        for dest_index, (_, private_address) in enumerate(addresses):
            if index != dest_index:
                iperf_script.write(
                        'iperf -t {} -c {} > bandwidth_{}.log &\n'
                        .format(config.iperf_time, private_address, dest_index))
        iperf_script.close()

def DistributeIperfScript(addresses):
    status_list = [
        subprocess.Popen(
                'rsync -e "ssh -i {key_location} {options}" -arz'
                ' iperf_{}.sh {user}@{ip}:~/test_script/iperf.sh'.format(
                    index,
                    ip=public_address,
                    user=config.user,
                    key_location=config.key_location,
                    options=config.ssh_options),
                shell=True)
        for index, (public_address, _) in enumerate(addresses)]
    for status in status_list:
        status.wait()

def CollectResults(addresses):
    for index, (public_address, _) in enumerate(addresses):
        subprocess.check_call(
                'rsync -e "ssh -i {key_location} {options}" -arz'
                ' {user}@{ip}:*.log result_{}/'.format(
                    index,
                    ip=public_address,
                    user=config.user,
                    key_location=config.key_location,
                    options=config.ssh_options),
                shell=True)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print 'usage: {} [placement group]'
        quit()
    addresses = FindMachines(config.region_name, sys.argv[1])
    # Test whether ssh can succeed.
    RunSerialEveryServer(addresses, 'date')
    # Build ping script.
    BuildPingScript(addresses)
    print 'Distribute ping script...'
    # Distribute ping script.
    DistributePingScript(addresses)
    # Build iperf script.
    BuildIperfScript(addresses)
    print 'Distribute iperf script...'
    # Distribute iperf script.
    DistributeIperfScript(addresses)
    print 'Run ping test...'
    # Run ping script.
    RunParallelEveryServer(addresses, 'bash test_script/ping.sh')
    time.sleep(config.ping_times * 1. + 10)
    print 'Run iperf test...'
    # Run iperf server.
    RunParallelEveryServer(addresses, 'screen -S iperf_server -d -m iperf -s')
    # Run iperf client.
    RunParallelEveryServer(addresses, 'bash test_script/iperf.sh')
    time.sleep(config.iperf_time + 10)
    print 'Collect results...'
    # Collect results.
    CollectResults(addresses)
