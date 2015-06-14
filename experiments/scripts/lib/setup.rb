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

  # options = { :interactive, :node, :user, :skey }
  def initialize(options)
    @opts = options
  end

  def get_opts
    return @opts
  end

  def set_opt(opt, value)
    @opts[opt] = value
  end

  def set_node(node)
    @opts[:node] = node
  end

  def interactive!
    if !@opts[:node] || @opts[:node].length == 0
      print "Node? "
      @opts[:node] = $stdin.gets.strip
      if @opts[:node].length == 0
        puts "You must specify a node!"
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

    Net::SSH.start(@opts[:node], @opts[:user], @opts[:skey]) do |ssh|
      # load ssh keys
      for u in USERS
        ssh.exec! "curl -X GET https://api.github.com/users/#{u}/keys \
          | grep key | sed 's/ *\"key\": *\"\\(.*\\)\"/\\1/' \
          >> /home/ubuntu/.ssh/authorized_keys"
      end

      # install mosh
      ssh.root! "DEBIAN_FRONTEND=noninteractive apt-get -yq apt-get \
        install mosh"

      # disable apt-get autoupdate
      ssh.root! 'sed -i -e"s/1/0/g" /etc/apt/apt.conf.d/10periodic'

      # setup instance store
      ssh.root! "umount /mnt"
      ssh.root! "mkfs.ext4 -E nodiscard /dev/xvdb"
      ssh.root! "sudo mount -o discard /dev/xvdb /mnt"
      ssh.root! "chown ubuntu:ubuntu /mnt"
      ssh.root! "chmod 777 /mnt"

      # setup sortgen
      ssh.exec! "wget #{SGEN_LOC} && tar xzf #{SGEN_FILE}"
      ssh.root! "mv 64/* /usr/bin/"
      # /mnt/64/gensort -t4 104857600 /mnt/records-unsorted
    end
  end
  
end
