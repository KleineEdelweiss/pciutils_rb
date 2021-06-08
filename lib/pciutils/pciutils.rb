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
  def self.stat(count_only = nil)
    out = PciUtils::Cache.instance.stat count_only
    if out then # Make sure the cache is loaded
      return out
    else # Otherwise, load it and return that
      self.update count_only
      return PciUtils::Cache.instance.stat
    end
  end # End stat method
  
  ##
  # Update the cache
  def self.update(count_only = nil)
    PciUtils::Cache.instance.update count_only
  end # End update method
  
  ##
  # Get the filters currently set for the PciCore
  def self.get_filters() PciUtils::Cache.instance.get_filters end
  
  ##
  # Clear the PciCore filters through the Cache
  def self.clear_filters() PciUtils::Cache.instance.clear_filters end
  
  ##
  # Set the filters (wrapped through the Cache)
  def self.set_filters(filters)
    PciUtils::Cache.instance.set_filters filters
  end # End filter setter
  
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
  class Cache < PciCore::AbsPci
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
    
    ##
    # Stat the cache, if it is populated. Otherwise,
    # populate it and then stat it.
    def stat(count_only = nil)
      case
      # Return the cache length
      when (@mcache and count_only) then
        @mcache.length
      # Return the cache data
      when @mcache then
        @mcache
      # Update the cache and send back the data
      else
        update count_only
      end
    end # End cache stat method
    
    ##
    # Load the extra data from procfs, so that the drivers
    # can be attached, as most users will not be able to
    # directly compile this from kernel headers.
    def procload
      @pcache = File.read("/proc/bus/pci/devices").split(/\n/)
        .collect { |row| row.split(/\s+/) }
        .select { |row| row.last.match?(/\D/) }
    end # End the procfs data loader
    
    ##
    # Update the cache
    def update(count_only = nil)
      # Load in the PCI devices from C
      if count_only then return pro_list true
      else
        @ccache = pro_list nil
        # Load the procfs data,
        # and then merge the caches.
        procload
        merge_caches
      end
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
      # Print the count of devices that could be merged
      # (debug only)
      puts "#{cnt} devices have procfs-discernible drivers"
      @mcache # Return the merged cache
    end # End the device merge method
    
    ##
    # Set the filters for the underlying abstract class.
    # Although the C-side performs type-checking, it is
    # performed here, as well, to emit an error message.
    # 
    # The filters passed in should be an array of integers,
    # as defined by the PciCore module constants. The reason
    # for the double-wrapping is in case the AbsPci class is
    # wrapped by something else.
    def set_filters(filters)
      #pro_set_filters(filters.collect { |x| Fixnum === x })
      pro_set_filters(filters)
    end # End filter setter
    
    ##
    # Clear the filters for the underlying abstract class
    def clear_filters() pro_clear_filters end
    
    ##
    # Return the filters from the underlying class
    def get_filters() pro_get_filters end
  end # End Cache class
end # End PCIU module

# d = File.read("/proc/bus/pci/devices").split(/\n/).collect { |row| row.split(/\s+/) }
# d.collect do |row| row.last end .select do |val| val.match? /\D/ end