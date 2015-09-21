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

    DEF_TIMEOUT = 600

    def self.wait(host, user, options={})
      # copy so can modify
      myopts = options.clone
      mytimeout = DEF_TIMEOUT 
      if !myopts[:timeout] && !myopts[:timeout].nil?
        mytimeout = myopts[:timeout]
      end
      myopts[:timeout] = 5

      st = Time.now
      while true
        begin
          start(host, user, myopts)
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

