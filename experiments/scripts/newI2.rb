#!/usr/bin/env ruby

#
# This is a specialization of `newMachine.rb` to B.A.D.
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
end

optparse.parse!

if options[:key_name].nil?
  echo "Must set a public key (`-k`) to use!"
  exit 1
end

Launcher.new(options).launch!


