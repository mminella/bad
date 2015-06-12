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
  # Array is in order 64-bit hvm-ssd, 64-bit ebs, 64-bit instance, 32-bit ebs,
  # 32-bit instance
  UBUNTU_AMIS = {
    :"ap-northeast-1" =>
      %w[ami-93876e93
         ami-85876e85
         ami-e1836ae1
         ami-83876e83
         ami-e98f66e9],

    :"ap-southeast-1" =>
      %w[ami-66546234
         ami-72546220
         ami-8a5660d8
         ami-4c54621e
         ami-60695f32],

    :"ap-southeast-2" =>
      %w[ami-cd4e3ff7
         ami-dd4e3fe7
         ami-754e3f4f
         ami-df4e3fe5
         ami-714f3e4b],

    :"eu-west-1" =>
      %w[ami-d7fd6ea0
         ami-fbfd6e8c
         ami-c1f467b6
         ami-fdfd6e8a
         ami-19f7646e],

    :"sa-east-1" =>
      %w[ami-a357eebe
         ami-ad57eeb0
         ami-f154edec
         ami-b357eeae
         ami-5d54ed40],

    :"us-east-1" =>
      %w[ami-6089d208
         ami-988ad1f0
         ami-fc99c294
         ami-808ad1e8
         ami-32762d5a],

    :"us-west-1" =>
      %w[ami-cf7d998b
         ami-397d997d
         ami-837397c7
         ami-3f7d997b
         ami-3b70947f],

    :"us-west-2" =>
      %w[ami-3b14370b
         ami-cb1536fb
         ami-3b10330b
         ami-c71536f7
         ami-0d0e2d3d],
  }

  def initialize(options)
    keyID = ENV["BAD_AWS_ACCESS_KEY"]
    secret = ENV["BAD_AWS_SECRET_KEY"]

    if keyID.nil? or secret.nil?
      raise Exception, "AWS Access keys not defined!"
    end

    @options = options
    @ec2 = AWS::EC2.new(:access_key_id => keyID,
                        :secret_access_key => secret)
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

    # 32/64 bit?
    puts ""
    puts "Which architecture would you like?"
    puts "[1] 64-bit"
    puts "[2] 32-bit"
    puts ""
    print "[1-2]? "
    @options[:arch] = gets.strip.to_i - 1

    if !(@options[:instance_type].start_with? "r3" or
         @options[:instance_type].start_with? "i2")
      puts ""
      puts "Which root storage would you like?"
      if @options[:arch] == 0
        puts "[1] ssd"
        puts "[2] ebs"
        puts "[3] instance"
        puts ""
        print "[1-3]? "
      else
        puts "[1] ebs"
        puts "[2] instance"
        puts ""
        print "[1-2]? "
      end
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
    ami = UBUNTU_AMIS[region.name.to_sym][@options[:arch] * 3 + @options[:store]]

    config = {
      :image_id => ami,
      :availability_zone => @options[:zone],
      :security_groups => @options[:group],
      :instance_type => @options[:instance_type],
      :key_name => @options[:key_name],
      :disable_api_termination => @options[:terminate] == 1,
      :count => @options[:count]
    }

    if !@options[:placement_group].nil?
      config[:placement_group] = @options[:placement_group]
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
