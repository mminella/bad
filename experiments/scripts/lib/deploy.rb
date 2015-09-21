#!/usr/bin/env ruby

#
# This script deploys a tar file to an EC2 machine.
#

require 'net/ssh'
require 'net/scp'
require './lib/ssh'

class Deploy

  USER = 'ubuntu'
  HOME = "/home/#{USER}"

  # { :hostname, :distfile, :user, :skey }
  def initialize(opts)
    set_opts(opts)
  end

  def set_opts(opts)
    @hostname = opts[:hostname] if !opts[:hostname].nil?
    @tarfile = opts[:distfile] if !opts[:distfile].nil?
    @user = opts[:user] if !opts[:user].nil?
    @skey = opts[:skey] if !opts[:skey].nil?
  end

  def interactive!
    if @hostname.nil?
      print "Hostname? "
      @hostname = $stdin.gets.strip
    end

    print "SSH username to login with? [#{USER}] "
    @user = $stdin.gets.strip
    @user = USER if @user.length == 0

    if @skey.nil?
      print "Private Key to use? [default] "
      pkey = $stdin.gets.strip
      @skey = !pkey.empty? ? { :keys => [pkey]} : {}
    end

    if @tarfile.nil?
      print "Location of 'bad.tar.gz'? [./] "
      @tarfile = $stdin.gets.strip
      @tarfile = "bad.tar.gz" if @tarfile.length == 0
    end
  end

  def deploy!
    if !File.exists? @tarfile
      puts "Specified file doesn't exists: #{@tarfile}\n"
      exit 1
    end

    @ssh_opts = @skey.merge({
      :user_known_hosts_file => '/dev/null',
      :paranoid => false
    })

    Net::SCP.start(@hostname, @user, @ssh_opts).upload!(@tarfile, HOME)

    Net::SSH.start(@hostname, @user, @ssh_opts) do |ssh|
      ssh.root! "tar xvzf #{HOME}/#{@tarfile} -C /"
    end
  end

end

