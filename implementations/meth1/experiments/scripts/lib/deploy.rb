#!/usr/bin/env ruby

#
# This script deploys a tar file to an EC2 machine.
#

require 'net/ssh'
require 'net/scp'
require './lib/ssh'

class Deploy

  # { :hostname, :distfile, :sshopts, :reboot }
  def initialize(opts)
    set_opts(opts)
  end

  def set_opts(opts)
    @hostname = opts[:hostname] if !opts[:hostname].nil?
    @tarfile = opts[:distfile] if !opts[:distfile].nil?
    @ssh_opts = opts[:sshopts] if !opts[:sshopts].nil?
    @reboot = opts[:reboot] if !opts[:reboot].nil?
  end

  def interactive!
    if @hostname.nil?
      print "Hostname? "
      @hostname = $stdin.gets.strip
    end

    if @ssh_opts.nil?
      print "Private Key to use? [default] "
      pkey = $stdin.gets.strip
      @ssh_opts = !pkey.empty? ? { :keys => [pkey]} : {}
    end

    if @tarfile.nil?
      print "Location of 'deploy.tar.gz'? [./] "
      @tarfile = $stdin.gets.strip
      @tarfile = "deploy.tar.gz" if @tarfile.length == 0
    end

    if @reboot.nil?
      print "Reboot machine at end? [n] "
      @reboot = $stdin.gets.strip.downcase.chars.first
      @reboot = 'n' if @reboot.nil? || @reboot.length == 0
    end
  end

  def deploy!
    if !File.exists?(@tarfile)
      puts "Specified file doesn't exists: #{@tarfile}\n"
      exit 1
    end

    Net::SCP.start(@hostname, 'ubuntu', @ssh_opts).upload!(@tarfile, '/home/ubuntu/')

    Net::SSH.start(@hostname, 'ubuntu', @ssh_opts) do |ssh|
      ssh.root! "tar xvzf /home/ubuntu/#{@tarfile} -C /"
      if @reboot == 'y'
        ssh.root! "reboot"
      end
    end
  end

end

