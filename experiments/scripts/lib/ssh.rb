# 
# Shared utilities for launcher scripts
#

require 'net/ssh'
require 'net/scp'

class Net::SSH::Connection::Session
  def root(cmd)
    self.exec "sudo #{cmd}"
  end

  def root!(cmd)
    self.exec! "sudo #{cmd}"
  end
end

module Net
  module SSH

    DEF_TIMEOUT = 120

    def self.wait(host, user, options={})
      mytimeout = DEF_TIMEOUT 
      if !options[:timeout] && !options[:timeout].nil?
        mytimeout = options[:timeout]
      end
      options[:timeout] = 5

      st = Time.now
      while true
        begin
          start(host, user, options)
          return
        rescue
          if (Time.now - st) > mytimeout
            raise ConnectionTimeout, "Net::SSH.wait timed out!"
          end
        end
      end
    end
  end
end

