# lib/pciutils/pciutils.rb

# Regular module requires
require "singleton"

# Local requires
require_relative "./device"
require_relative "../pci_core/pci_core"

##
# PCI Utils module
module PciUtils
  ##
  # Stat the cache
  def self.stat() PciUtils::Cache.instance.stat end
  
  ##
  # Update the cache
  def self.update() PciUtils::Cache.instance.update end
  
  ##
  # Load in the procfs data
  def self.procload
    File.read("/proc/bus/pci/devices").split(/\n/).collect { |row| row.split(/\s+/) }
      .select { |row| row.last.match?(/\D/) }
  end # End procfs loader
  
  ##
  # Cache is the current PCI device store and
  # accessor for both C and non-C information.
  # 
  # As some of these operations are expensive, they
  # should not generally be performed more than they
  # are strictly needed. As PCI devices are unlikely to
  # change, and it is even unlikely for a driver change,
  # this can usually be done once, at module startup.
  # 
  # In the off-chance live driver changes or PCI hot-plugging
  # is performed, this class can be updated with the :update
  # method. As this can have resource-heavy operations and
  # there is unlikely to be any need for multiple copies, it
  # is being implemented as a singleton.
  class Cache
    include Singleton
    ##
    # Cache of the raw C-code data
    attr_reader :ccache
    
    ##
    # Cache of the raw procfs data
    attr_reader :pcache
    
    ##
    # Cache of the merged data (cache of actual devices)
    attr_reader :mcache
    alias :cache :mcache
    alias :stat :mcache
    
    ##
    # Update the cache
    def update
      # Load in the PCI devices from C
      @ccache = PciCore.list
      
      # Load the procfs data
      @pcache = PciUtils.procload
      
      # Merge the data
      merge_caches
    end # End the update method
    
    ##
    # Merge the data for each device in
    # :ccache with its corresponding :pcache
    # driver info, if available.
    def merge_caches
      @mcache = [] # Initialize the merge cache
      cnt = 0 # Drivers found count
      @ccache.map do |device|
        # Create a new device object and
        # load in the correct driver name (if available).
        d = Device.new(device)
        missing = d.find_driver(@pcache)
        
        # Increment the counter, if the
        # driver was located from the proc data
        if !missing then cnt += 1 end
        
        # Add the new device to the merge cache
        @mcache << d
      end
      # Print the count and return it
      puts "#{cnt} devices have procfs-discernible drivers"
      cnt
    end # End the device merge method
  end # End Cache class
end # End PCIU module

# d = File.read("/proc/bus/pci/devices").split(/\n/).collect { |row| row.split(/\s+/) }
# d.collect do |row| row.last end .select do |val| val.match? /\D/ end