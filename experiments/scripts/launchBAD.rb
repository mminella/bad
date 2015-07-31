#!/usr/bin/env ruby

# Launch a new cluster of machines for B.A.D.

require './lib/ec2'
require './lib/setup'
require './lib/deploy'

require 'optparse'

SSH_USER = 'ubuntu'
SSH_PKEY = {} # default
TAR_FILE = 'bad.tar.gz'

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

  options[:zone] = "ap-southeast-1a"
  options[:group] = "default"
  options[:instance_type] = "i2.xlarge"
  options[:arch] = 0 # 64-bit
  options[:store] = 0
  options[:terminate] = 0

  opts.on("-k", "--key KEY_NAME", "Security key name") do |key|
    options[:key_name] = key
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
  puts "Must set a public key (`-k`) to use!"
  exit 1
end

# make '.' output for wait appear steady
$stdout.sync = true

# move any old cluster info files
if File.exists? '.cluster.conf'
  now = Time.now.strftime("%Y%m%d_%H%M")
  `mv -f .cluster.conf ./.old_clusters/#{now}.cluster.conf`
end

# start new cluster config
`echo "# #{Time.now}" > .cluster.conf`
`echo "export MN=#{options[:count]}" >> .cluster.conf`

# launch instance
i = 0
instances = []
Launcher.new(options).launch! do |instance|
  i += 1
  instances += [instance]
  puts "New instance (#{i}) at: #{instance.dns_name}"
  
  # store machine
  `echo "export M#{i}=#{instance.dns_name}" >> .cluster.conf`
  `echo "export M#{i}_ID=#{instance.id}" >> .cluster.conf`
end

# configure instances
instances.each do |instance|
  # wait for instance to be ready and SSH up
  puts "Waiting for instances to become ready..."
  timeout = 120
  while instance.status != :running && timeout > 0
    print '.'
    timeout = timeout - 1
    sleep 1
  end
  if timeout <= 0
    puts "Timeout waiting on instance to be ready!"
    exit 1
  end
  puts "Waiting on SSH to become ready..."
  sleep 10
  Net::SSH.wait(instance.dns_name, SSH_USER, SSH_PKEY)

  # setup instance
  puts "Setting up instance (#{i})..."
  conf = Setup.new(host: instance.dns_name, user: SSH_USER, skey: SSH_PKEY)
  conf.setup!
  puts "Instance (#{i}) setup for B.A.D!"

  # deploy bad
  puts "Deploying a B.A.D distribution..."
  deployer = Deploy.new(hostname: instance.dns_name, user: SSH_USER,
                        skey: SSH_PKEY, distfile: options[:tar_file])
  deployer.deploy!
  puts "B.A.D deployed and ready!"
end

