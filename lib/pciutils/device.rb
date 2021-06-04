# lib/pciutils/device.rb

# :nodoc: Attach the device class to the PciUtils module
module PciUtils
  ##
  # The Device class represents individual instances of
  # a PCI device, and it offers methods for formatting and
  # attaching additional information.
  # 
  # When using instances of this class, you can inject them
  # as members of other objects, enabling monitoring of more
  # complex data on your system.
  # 
  # The information for the device is obtained by PciCore.list.
  # Each hash is a PCI device.
  # 
  # When assigning the instance variables,
  # it should be pointed out that the numeric
  # ones are converted to decimal int, but they should be
  # represented in hex notation, or they won't make sense.
  class Device
    ##
    # Data passed in from the list
    attr_reader :dev_data
    
    ##
    # Procfs device matchers -- these are used to attach
    # the driver to the device, as the driver name
    # is not able to be cleanly accessed from the
    # C-side of the code, due to dependency issues.
    # 
    # The dependencies are not always available and
    # RARELY in the linker path, as they are system-
    # specific kernel header files, so they may not be
    # exposed on binary distributions (tested on Debian
    # and Arch), even with the dev header packages.
    # 
    # As such, without complex hacks, the only easy
    # ways to access this info include:
    # 1) Match the modalias on regex to the modules.alias
    #   file, '/lib/modules/{current architecture}/modules.alias'
    #   (VERY SLOW)
    # 2) Parse every sysfs bus file, hoping a driver file is present.
    #   (Unreliable, error-prone)
    # 3) Parsing '/proc/bus/pci/devices', splitting the whitespace-
    #   separated values, and comparing against the first 2 fields,
    #   then selecting the last field, and rejecting it if it has
    #   no non-digit characters (the last field is normally "0",
    #   if the file does not identify a driver).
    attr_reader :procdev
    alias :matcher :procdev
    
    ##
    # Driver name (attached as explained for :procdev,
    # AFTER instantiation).
    attr_reader :driver
    
    ##
    # Constructor for a Device instance.
    # This should receive a Hash, from the device
    # list in PciCore.
    def initialize(dev)
      # Init raw device data
      @procdev, @dev_data, @driver = {}, {}, []
      # Check that the hash is, in fact, a hash
      if (Hash === dev) then
        @dev_data = dev
        @procdev = dev[:matcher]
      end
    end # End constructor
    
    ##
    # Load in the driver name from data retrieved
    # from procfs.
    def find_driver(list)
      m = matcher # Get the bus and ident matchers
      # Check through the rows
      list.map do |row|
        # Match the first 2 columns. Only these two
        # should be necessary, as it will match against
        # the bus ID, the function of the device, the
        # device ID, and the vendor ID
        bmatch = row[0].match? m[:bus]
        imatch = row[1].match? m[:idents]
        
        # The list has already been filtered to ONLY
        # include rows with a driver in the last column.
        # 
        # If no row matches, it SHOULD mean procfs
        # does not report a driver for that device, not
        # that the device itself is not reported in procfs.
        if imatch then
          @driver << row.last
          break
        end # Exit the loop, if found
      end # End driver check loop
      # If the driver is nil, then it will return a
      # "missing = true" to the wrapper.
      @driver.uniq!
      @driver.empty?
    end # End read from the driver
  end # End Device class
end # End module attachment