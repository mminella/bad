#!/usr/bin/env ruby

#
# This script launches a new Amazon EC2 machine of the configuration you
# select.
#

require './lib/ec2.rb'
require 'optparse'

options = {}

optparse = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} hostname [options]"

  options[:interactive] = ARGV.length == 0
  opts.on("-i", "--interactive", "Ask for options interactively") do
    options[:interactive] = true
  end

  options[:zone] = "us-east-1c"
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

  options[:instance_type] = "m2.xlarge"
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
  opts.on("-c", "--count", "Number of instances to launch") do |c|
    options[:count] = c.to_i
  end

  opts.on("-n", "--name", "Name of the new instance") do |name|
    if options[:count] == 1
      options[:name] = [name]
    else
      names = name.split(',')
      if names.length != options[:count]
        $stderr.puts "Invalid number of names! (formant '<name1>,<name2>,...')"
      end
      options[:name] = names
    end
  end
end

optparse.parse!

puts "Launching..."
Launcher.new(options).launch!

