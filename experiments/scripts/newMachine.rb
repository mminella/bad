#!/usr/bin/env ruby

#
# This script launches a new Amazon EC2 machine of the configuration you
# select. You can either run interactively, or by setting command line flags.
#
# Supported Options:
# - :image_id, :zone, :group, :instance_type, :key_name, :terminate, :arch,
#   :store, :count, :placement_group
#

require './lib/ec2.rb'
require 'optparse'

nameArg = nil
options = {}

def parseName(opts, name)
  if opts[:count] == 1
    opts[:name] = [name]
  else
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
end

optparse = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [options]\nCreate a set of new EC2 instances."

  options[:interactive] = ARGV.length == 0
  opts.on("-i", "--interactive", "Ask for options interactively") do
    options[:interactive] = true
  end

  options[:zone] = "ap-southeast-1a"
  opts.on("-z", "--zone AVAILABILITY_ZONE", "Availability zone to use") do |zone|
    options[:zone] = zone
  end

  opts.on("-k", "--key KEY_NAME", "Security key name") do |key|
    options[:key_name] = key
  end

  options[:group] = "default"
  opts.on("-g", "--group SECURITY_GROUP", "Security group to launch in") do |group|
    options[:group] = group
  end

  options[:instance_type] = "i2.xlarge"
  opts.on("-t", "--type INSTANCE_TYPE", "Instance type (t1.micro, m1.small...)") do |type|
    options[:instance_type] = type
  end

  options[:arch] = 0
  opts.on("-a", "--arch [64|32]", "Architecture") do |arch|
    if arch == "32"
      options[:arch] = 1
    elsif arch == "64"
      options[:arch] = 0
    else
      $stderr.puts "Invalid architecture: #{arch}"
      exit(1)
    end
  end

  options[:store] = 1
  opts.on("-s", "--store [ebs|instance]", "Root storage type") do |store|
    if store == "instance"
      options[:store] = 1
    elsif arch == "ebs"
      options[:store] = 0
    else
      $stderr.puts "Invalid store: #{store}"
      exit(1)
    end
  end

  options[:terminate] = 0
  opts.on("-x", "--terminate", "Enable termination protection for the instance") do
    options[:terminate] = 1
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
end

optparse.parse!

Launcher.new(options).launch!

