#
# Library for launching an EC2 machine on Amazon.
#

require 'aws-sdk'
require 'optparse'

class Launcher

  # Just a subset of ones available
  INSTANCE_TYPES = %w[
    t2.micro
    m2.small
    m3.medium
    m3.large
    m3.xlarge
    m3.2xlarge
    r3.large
    r3.xlarge
    r3.2xlarge
    r3.4xlarge
    r3.8xlarge
    i2.xlarge
    i2.2xlarge
    i2.4xlarge
    i2.8xlarge]

  # Hash of Ubuntu 14.04.1 AMIs by availability zone.
  # Array is in order 64-bit hvm-ssd, 64-bit ebs, 64-bit instance
  UBUNTU_AMIS = {
    :"ap-northeast-1" =>
      %w[ami-aa7da3aa
         ami-607da360
         ami-c44a94c4
        ],

    :"ap-southeast-1" =>
      %w[ami-fae0daa8
         ami-cae0da98
         ami-c4edd796
        ],

    :"ap-southeast-2" =>
      %w[ami-cf047ff5
         ami-e1047fdb
         ami-37057e0d
        ],

    :"eu-west-1" =>
      %w[ami-85344af2
         ami-97344ae0
         ami-273c4250
        ],

    :"sa-east-1" =>
      %w[ami-952eae88
         ami-652eae78
         ami-392dad24
        ],

    :"us-east-1" =>
      %w[ami-3b6a8050
         ami-5f6a8034
         ami-1305ef78
        ],

    :"us-west-1" =>
      %w[ami-fb05efbf
         ami-e305efa7
         ami-2107ed65
        ],

    :"us-west-2" =>
      %w[ami-ade2da9d
         ami-ade1d99d
         ami-bdedd58d
        ],
  }

  def createEC2Client(region='us-east-1')
    keyID = ENV["BAD_AWS_ACCESS_KEY"]
    secret = ENV["BAD_AWS_SECRET_KEY"]

    if keyID.nil? or secret.nil?
      raise Exception, "AWS Access keys not defined!"
    end

    return AWS::EC2.new(:access_key_id => keyID,
                        :secret_access_key => secret,
                        :region => region)
  end

  def initialize(options)
    @options = options
    @ec2 = createEC2Client()
  end

  def interactive!
    # Choose region to select zone from
    puts "Which region do you want to deploy in?"
    regions = @ec2.regions.map(&:name)
    regions.each_with_index do |r, i|
      puts "[#{i + 1}] #{r}"
    end
    puts ""
    print "[1-#{regions.size}]? "
    region = @ec2.regions[regions[gets.strip.to_i - 1]]

    @ec2 = createEC2Client(region)

    # AZ
    puts ""
    puts "Which availability zone in #{region.name}?"
    azs = region.availability_zones.map(&:name)
    azs.each_with_index do |z, i|
      puts "[#{i + 1}] #{z}"
    end
    puts ""
    print "[1-#{azs.size}]? "
    @options[:zone] = azs[gets.strip.to_i - 1]

    # Security group
    puts ""
    puts "Which security group should the instances belong to?"
    groups = region.security_groups.map(&:name)
    groups.each_with_index do |g, i|
      puts "[#{i + 1}] #{g}"
    end
    puts ""
    print "[1-#{groups.size}]? "
    @options[:group] = groups[gets.strip.to_i - 1]

    # Instance type
    puts ""
    puts "Which instance type would you like to deploy?"
    INSTANCE_TYPES.each_with_index do |t, i|
      puts "[#{i + 1}] #{t}"
    end
    puts ""
    print "[1-#{INSTANCE_TYPES.size}]? "
    @options[:instance_type] = INSTANCE_TYPES[gets.strip.to_i - 1]

    # Storage type
    if !(@options[:instance_type].start_with? "r3" or
         @options[:instance_type].start_with? "i2")
      puts ""
      puts "Which root storage would you like?"
      puts "[1] ssd"
      puts "[2] ebs"
      puts "[3] instance"
      puts ""
      print "[1-3]? "
      @options[:store] = gets.strip.to_i - 1
    else
      # r3 only supports ssd (hvm virtulization type)
      @options[:store] = 0
    end

    # Placement group for r3 nodes
    if @options[:instance_type].start_with? "r3" or
        @options[:instance_type].start_with? "i2" 
      puts ""
      print "Placement group? "
      pg = gets.strip
      if pg.length > 0
        @options[:placement_group] = pg

        pgs = @ec2.client.describe_placement_groups()[:placement_group_set]
        if pgs.nil? || pgs.all? { |i| i[:group_name] != pg }
          puts ""
          print "PG doesn't exist, create placement group [yes, no]? "
          ans = gets.strip
          if ans != "y" and ans != "yes"
            puts "Can't use uncreated placement group!"
            exit 0
          end
        end
      end
    end

    # SSH Public key
    puts ""
    puts "Which security key will you use?"
    puts "[0] Upload new key"
    keys = region.key_pairs.map(&:name)
    keys.each_with_index do |k, i|
      puts "[#{i + 1}] #{k}"
    end
    puts ""
    print "[0-#{keys.size}]? "
    kid = gets.strip.to_i
    if kid == 0
      print "Key name (#{`hostname`.strip})? "
      keyname = gets.strip
      keyname = `hostname`.strip if keyname.empty?
      print "Key location (~/.ssh/id_rsa.pub)? "
      keyloc = gets.strip
      keyloc = "~/.ssh/id_rsa.pub" if keyloc.empty?
      kp = region.key_pairs.import(keyname, File.read(File.expand_path(keyloc)))
      @options[:key_name] = kp.name
    else
      @options[:key_name] = keys[kid - 1]
    end

    # Termination protection
    puts ""
    puts "Enable termination protection? "
    puts "[1] No"
    puts "[2] Yes"
    puts ""
    print "[1-2]? "
    @options[:terminate] = gets.strip.to_i - 1

    # Number of instances
    puts ""
    print "Number of instances to create? [1] "
    n = gets.strip
    if n.length > 0
      n = n.to_i
    else
      n = 1
    end
    @options[:count] = n

    # Instance names
    puts ""
    for i in 1..n do
      loop do
        print "Enter a name for this instance [#{i}]: "
        @options[:name] ||= []
        name = gets.strip
        if name.length > 0
          @options[:name] << name
          break
        end
        puts "ERROR: please enter a name for the instance!"
      end
    end

    # Final confirm
    puts ""
    print "Proceeed with launch [yes, no]? "
    ans = gets.strip
    puts ""
    if ans != "y" && ans != "yes"
      puts "Aborting launch!"
      exit 0
    end
    puts "Launching..."
  end

  def launch!
    if @options[:interactive]
      interactive!
    end
    
    # configure creation
    region = @ec2.regions[@options[:zone][0..-2]]
    ami = UBUNTU_AMIS[region.name.to_sym][@options[:store]]
    @ec2 = createEC2Client(region.name)

    config = {
      :image_id => ami,
      :availability_zone => @options[:zone],
      :security_groups => @options[:group],
      :instance_type => @options[:instance_type],
      :key_name => @options[:key_name],
      :disable_api_termination => @options[:terminate] == 1,
      :count => @options[:count],
      :block_device_mappings => [
      {
        :device_name => '/dev/xvdb',
        :virtual_name => 'ephemeral0'
      },
      {
        :device_name => '/dev/xvdc',
        :virtual_name => 'ephemeral1'
      },
      {
        :device_name => '/dev/xvdd',
        :virtual_name => 'ephemeral2'
      },
      {
        :device_name => '/dev/xvde',
        :virtual_name => 'ephemeral3'
      },
      {
        :device_name => '/dev/xvdf',
        :virtual_name => 'ephemeral4'
      },
      {
        :device_name => '/dev/xvdg',
        :virtual_name => 'ephemeral5'
      },
      {
        :device_name => '/dev/xvdh',
        :virtual_name => 'ephemeral6'
      },
      {
        :device_name => '/dev/xvdi',
        :virtual_name => 'ephemeral7'
      },
      ]
    }

    if !@options[:placement_group].nil?
      pg = @options[:placement_group]
      config[:placement_group] = pg
      pgs = @ec2.client.describe_placement_groups()[:placement_group_set]
      if pgs.nil? or pgs.all? { |i| i[:group_name] != pg }
        @ec2.client.create_placement_group(:group_name => pg,
                                           :strategy => 'cluster')
      end
    end

    # create the instance(s)
    instances = region.instances.create(config)

    # unify return types (array vs single)
    if @options[:count] == 1
      instances = [instances]
    end

    # wait till launched...
    sleep 1 while instances.any? {|i| !i.exists? || i.status == :pending }

    # setup each instance
    for inst in instances do

      # set the human readable name
      if !@options[:name].nil?
        n = @options[:name].shift
        if !n.nil? && n.length > 0
          inst.add_tag("Name", :value => n)
        end
      end

      # set tags
      if !@options[:tags].nil? && @options[:tags].length > 0
        @options[:tags].each do |k,v|
          inst.add_tag("#{k}", :value => "#{v}")
        end
      end

      # run setup routine if given
      if block_given?
        yield(inst)
      else
        puts "New instance at: #{inst.dns_name}"
      end
    end

  end

end
