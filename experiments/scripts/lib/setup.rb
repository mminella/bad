#!/usr/bin/env ruby

#
# This script setups a newly provisioned EC2 machine to run B.A.D.
#

require 'net/ssh'
require 'net/scp'
require './lib/ssh'

USERS     = ['dterei']
SGEN_LOC  = 'http://www.ordinal.com/try.cgi/gensort-linux-1.5.tar.gz'
SGEN_FILE = SGEN_LOC.split("/").last

class Setup

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

    print "SSH username to login with? [ubuntu] "
    @opts[:user] = $stdin.gets.strip
    @opts[:user] = "ubuntu" if @opts[:user].length == 0

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
          >> /home/ubuntu/.ssh/authorized_keys"
      end

      # install mosh
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get -yq install \
        mosh binutils libtbb2 libtbb-dev libjemalloc-dev libgoogle-perftools-dev"
      ssh.root! "DEBIAN_FRONTEND=noninteractive add-apt-repository -y ppa:ubuntu-toolchain-r/test"
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get update"
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get -yq dist-upgrade"

      # disable apt-get autoupdate
      ssh.root! 'sed -i -e"s/1/0/g" /etc/apt/apt.conf.d/10periodic'

      # setup sortgen
      ssh.exec! "wget #{SGEN_LOC} && tar xzf #{SGEN_FILE}"
      ssh.root! "mv 64/* /usr/bin/"
    end
  end
  
end
