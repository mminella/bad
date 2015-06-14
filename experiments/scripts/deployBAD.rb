#!/usr/bin/env ruby

#
# This script deploys B.A.D to an EC2 machine.
#

require 'net/ssh'
require 'net/scp'
require './lib/deploy'

hostname = ARGV[0]

if !ARGV[0]
  puts "This is the B.A.D deploy script."
  puts ""
  print "Hostname? "
  hostname = $stdin.gets.strip
elsif ARGV[0] == "help" || ARGV[0] == "--help" || ARGV[0] == "-h"
  puts "Usage: " + __FILE__ + " [hostname]"
  exit 0
end

deployer = Deploy.new(hostname: hostname)
deployer.interactive!
deployer.deploy!

puts "\nB.A.D deployed!"

