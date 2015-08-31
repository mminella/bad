#!/usr/bin/env ruby

#
# This script setups a newly provisioned EC2 machine to run B.A.D.
#

require 'net/ssh'
require 'net/scp'
require './lib/ssh'

class Setup

  USER          = "ubuntu"
  HOME          = "/home/#{USER}"
  USERS         = ["dterei"]
  SGEN_LOC      = "http://www.ordinal.com/try.cgi/gensort-linux-1.5.tar.gz"
  SGEN_FILE     = SGEN_LOC.split("/").last
  IXGBEVF_LOC   = "http://downloads.sourceforge.net/project/e1000/ixgbevf%20stable/2.16.1/ixgbevf-2.16.1.tar.gz"
  IXGBEVF_FILE  = IXGBEVF_LOC.split("/").last
  IXGBEVF_DIR   = "#{HOME}/ixgbevf-2.16.1/src"
  IXGBEVF_PATCH = "ixgbevf.2.16.1.patch"
  NETTUNE_FILE  = "60-awstune.conf"

  # options = { :interactive, :host, :user, :skey }
  def initialize(options)
    @opts = options
  end

  def get_opts
    return @opts
  end

  def set_opt(opt, value)
    @opts[opt] = value
  end

  def set_host(host)
    @opts[:host] = host
  end

  def interactive!
    if !@opts[:host] || @opts[:host].length == 0
      print "Host? "
      @opts[:host] = $stdin.gets.strip
      if @opts[:host].length == 0
        puts "You must specify a host!"
        exit 1
      end
    end

    print "SSH username to login with? [#{USER}] "
    @opts[:user] = $stdin.gets.strip
    @opts[:user] = USER if @opts[:user].length == 0

    print "Private Key to use? [default] "
    skey = $stdin.gets.strip
    @opts[:skey] = !skey.empty? ? { :keys => [skey]} : {}
  end

  def setup!
    if @opts[:interactive]
      interactive!
    end

    @ssh_opts = @opts[:skey].merge({:user_known_hosts_file => '/dev/null', :paranoid => false})

    Net::SSH.start(@opts[:host], @opts[:user], @ssh_opts) do |ssh|
      # load ssh keys
      for u in USERS
        ssh.exec! "curl -X GET https://api.github.com/users/#{u}/keys \
          | grep key | sed 's/ *\"key\": *\"\\(.*\\)\"/\\1/' \
          >> #{HOME}/.ssh/authorized_keys"
      end

      # update libc
      ssh.root! "DEBIAN_FRONTEND=noninteractive add-apt-repository -y ppa:ubuntu-toolchain-r/test"
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get update"
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get -yq dist-upgrade"

      # install mosh, allocators, performance monitoring tools and schedtool
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get -yq install \
        mosh binutils libtbb2 libtbb-dev libjemalloc-dev libgoogle-perftools-dev \
        dstat sysbench sysstat nicstat nload schedtool"

      # install perf
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get -yq install \
        linux-tools-common linux-tools-generic linux-tools-3.13.0-53-generic"

      # install build tools
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get -yq install \
        build-essential"

      # disable apt-get autoupdate
      ssh.root! 'sed -i -e"s/1/0/g" /etc/apt/apt.conf.d/10periodic'

      # setup sortgen
      ssh.exec! "wget #{SGEN_LOC} && tar xzf #{SGEN_FILE}"
      ssh.root! "mv 64/* /usr/bin/"

      # reboot for libc upgrade
      ssh.root! "reboot"
    end
    Net::SSH.wait(@opts[:host], @opts[:user], @ssh_opts)

    # tune instance -- networking
    Net::SCP.start(@opts[:host], @opts[:user], @ssh_opts)
      .upload!("./lib/#{NETTUNE_FILE}", HOME)
    Net::SSH.start(@opts[:host], @opts[:user], @ssh_opts) do |ssh|
      ssh.root! "mv #{HOME}/#{NETTUNE_FILE} /etc/sysctl.d/"
      ssh.root! "chown root:root /etc/sysctl.d/#{NETTUNE_FILE}"
      ssh.root! "chmod 644 /etc/sysctl.d/#{NETTUNE_FILE}"
    end

    # install ixgbevf latest version
    Net::SCP.start(@opts[:host], @opts[:user], @ssh_opts)
      .upload!("./lib/#{IXGBEVF_PATCH}", HOME)
    Net::SSH.start(@opts[:host], @opts[:user], @ssh_opts) do |ssh|
      ssh.exec! "wget #{IXGBEVF_LOC} && tar xzf #{IXGBEVF_FILE}"
      ssh.exec! "cd #{IXGBEVF_DIR}; patch -p0 < #{HOME}/#{IXGBEVF_PATCH}"
      ssh.exec! "cd #{IXGBEVF_DIR}; make clean; make"
      ssh.exec! "cd #{IXGBEVF_DIR}; sudo make install"
      ssh.root! "update-initramfs -c -k all"
      ssh.root! "reboot"
    end
    Net::SSH.wait(@opts[:host], @opts[:user], @ssh_opts)

    # tune instance
    Net::SSH.start(@opts[:host], @opts[:user], @ssh_opts) do |ssh|
      # switch to TSC for clock, not Xen
      ssh.exec! "echo tsc | sudo tee /sys/devices/system/clocksource/clocksource0/current_clocksource"
      # switch to longer time slices and batch tune values
      ssh.root! "schedtool -B PID"
    end

  end

end
