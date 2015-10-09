#!/usr/bin/env ruby

# Launch a new cluster of machines for B.A.D.

require './lib/ec2'
require './lib/setup'
require './lib/deploy'

require 'optparse'
require 'parallel'

SSH_USER = 'ubuntu'
SSH_PKEY = {} # default
TAR_FILE = 'bad.tar.gz'
CLUSTER_CONF = 'cluster.conf'
MAX_PARALLEL = 20
LAUNCH_TIMEOUT = 180

nameArg = nil
options = {}

def parseName(opts, name)
  names = name.split(',')
  if names.length > 1
    if names.length != opts[:count]
      $stderr.puts "Invalid number of names! (formant '<name1>,<name2>,...')"
      exit 0
    end
  else
    names = []
    for i in 1..opts[:count] do
      names += [ sprintf(name, i) ]
    end
  end
  opts[:name] = names
end

optparse = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [options]\nCreate a set of new i2.xlarge instances for B.A.D."

  options[:group] = "default"
  options[:arch] = 0 # 64-bit
  options[:store] = 0
  options[:terminate] = 0

  options[:start_id] = 1
  opts.on("-s", "--start-id NUMBER",
          "Starting ID of machines in cluster file") do |i|
    options[:start_id] = i.to_i
  end

  options[:append_config] = false
  opts.on("-a", "--append-config", "Append to cluster config file") do
    options[:append_config] = true
  end

  options[:zone] = "ap-southeast-1a"
  opts.on("-z", "--zone ZONE", "AWS region to deploy in") do |zone|
    options[:zone] = zone.strip
  end

  options[:key_name] = `hostname`.strip
  opts.on("-k", "--key KEY_NAME", "Security key name") do |key|
    options[:key_name] = key.strip
  end

  options[:instance_type] = "i2.xlarge"
  opts.on("-i", "--instance INST", "Instance type") do |i|
    options[:instance_type] = i
  end

  options[:file] = '.cluster.conf'
  opts.on("-f", "--file FILE", "File to save cluster info to") do |f|
    options[:file] = f
  end

  options[:count] = 1
  opts.on("-c", "--count NUMBER", "Number of instances to launch") do |c|
    options[:count] = c.to_i
    # we do this to make order of `--count` and `--name` irrelevant.
    if !nameArg.nil?
      parseName(options, nameArg)
    end
  end

  opts.on("-n", "--name NAME_PATTERN", "Name of the new instance") do |name|
    nameArg = name
    parseName(options, name)
  end

  options[:placement_group] = nil
  opts.on("-p", "--placement PLACEMENT_GROUP", "Placement group to launch in") do |pg|
    options[:placement_group] = pg
  end

  options[:tar_file] = TAR_FILE
  opts.on("-d", "--dist-file STRING", "Distribution file to deploy") do |df|
    options[:tar_file] = df
  end

end

if ARGV.length == 0
  optparse.parse %w[--help]
end
optparse.parse!

if options[:key_name].nil?
  $stderr.puts "Must set a public key (`-k`) to use!"
  exit 1
end

# make '.' output for wait appear steady
$stdout.sync = true

# move any old cluster info files
if File.exists? options[:file] and !options[:append_config]
  now = Time.now.strftime("%Y%m%d_%H%M")
  `mv -f #{options[:file]} ./.old_clusters/#{now}.cluster.conf`
end

# start new cluster config
mn=options[:start_id] + options[:count] - 1
`echo "# #{Time.now}" >> #{options[:file]}`
`echo "export MREGION=#{options[:zone][0..-2]}" >> #{options[:file]}`
`echo "export MN=#{mn}" >> #{options[:file]}`

# launch instance
i = options[:start_id]
instances = []
Launcher.new(options).launch! do |instance|
  instances += [instance]
  puts "#{i}: New instance: #{instance.dns_name}"

  # store machine
  `echo "export M#{i}=#{instance.dns_name}" >> #{options[:file]}`
  `echo "export M#{i}_ID=#{instance.id}" >> #{options[:file]}`
  `echo "export M#{i}_TYPE=#{options[:instance_type]}" >> #{options[:file]}`
  `echo "alias m#{i}=\\"ssh ubuntu@\\$M#{i}\\"" >> #{options[:file]}`
  i += 1
end

# configure instances
i = options[:start_id]
threads = [instances.size, MAX_PARALLEL].min
Parallel.map(instances, :in_threads => threads) { |inst|
  # copy since running in parallel
  j = i
  i += 1

  begin
    # wait for instance to be ready
    print "#{j}: Waiting for instance to become ready... #{inst.dns_name}\n"
    timeout = LAUNCH_TIMEOUT
    while inst.status == :pending && timeout > 0
      timeout = timeout - 1
      sleep 1
    end
    if timeout <= 0
      $stderr.print "#{j}: Timeout waiting on instance to be ready!\n"
      exit 1
    end

    # wait for SSH to be ready
    print "#{j}: Waiting on SSH to become ready...\n"
    Net::SSH.wait(inst.dns_name, SSH_USER, SSH_PKEY)

    # copy across cluster config
    print "#{j}: Copy across cluster info...\n"
    Net::SCP.start(inst.dns_name, SSH_USER, SSH_PKEY)
      .upload!(options[:file], CLUSTER_CONF)

    # setup instance
    print "#{j}: Setting up instance...\n"
    conf = Setup.new(host: inst.dns_name, user: SSH_USER, skey: SSH_PKEY)
    conf.setup!
    print "#{j}: Instance setup for B.A.D!\n"

    # deploy bad
    print "#{j}: Waiting on SSH to become ready...\n"
    Net::SSH.wait(inst.dns_name, SSH_USER, SSH_PKEY)
    print "#{j}: Deploying B.A.D distribution...\n"
    deployer = Deploy.new(hostname: inst.dns_name, user: SSH_USER,
                          skey: SSH_PKEY, distfile: options[:tar_file])
    deployer.deploy!
    print "#{j}: B.A.D deployed and ready!\n"
  rescue Exception => e
    $stderr.print "#{j}: Instance failed! [#{inst.dns_name}]\n#{e.message}\n"
    raise e
  end
}
