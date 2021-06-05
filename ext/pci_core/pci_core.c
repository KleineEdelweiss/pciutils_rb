// ext/pci_core/pci_core.c
// Ruby module
#include <ruby.h>

// System modules
#include <stdio.h>
#include <stdlib.h>

// Device modules
#include <pci/pci.h>
#include <pciaccess.h>

// Other modules
#include <string.h>

// Initializers
VALUE PciCore = Qnil;
VALUE PClassFilters = Qnil;

/*
 * PCI devices classes are stored under #{architecture}/pci/header.h.
 *   Example: /usr/include/x86_64-linux-gnu/pci/header.h
 * 
 * The #defines include things like (int and hex values listed as of
 *    the time of writing. Doubt they'll change, but don't hard code
 *    against them, in case they do -- use the #define'd versions
 *    I will export these in the header file):
 * -PCI_BASE_CLASS_PROCESSOR (11 / 0x0b)
 * -PCI_BASE_CLASS_MEMORY (5, 0x05)
 * -PCI_BASE_CLASS_MULTIMEDIA (4, 0x04)
 * 
 * header.h is also automatically included by <pci/pci.h>, so it's
 * not explicitly listed above.
 */

// List all of the PCI devices
VALUE method_pci_list(VALUE self) {
  // Init main PCI system
  pci_system_init();
  // Allocate PCI device access list
  struct pci_access *list = pci_alloc();
  pci_init(list); // Init resources
  pci_scan_bus(list); // Scan all devices
  
  // Create the new mapping hash
  //VALUE map = rb_hash_new();
  //rb_hash_aset(map, rb_id2sym(rb_intern("modalias")),
  //  method_pci_get_config(self));
  
  VALUE devices = rb_ary_new(); // Array of devices
  int count = 0; // Device counter
  
  // Matcher used to simply say to get all the devices
  // (for now...)
  const struct pci_id_match matcher  = {
    .vendor_id = PCI_MATCH_ANY,
    .device_id = PCI_MATCH_ANY,
    .subvendor_id = PCI_MATCH_ANY,
    .subdevice_id = PCI_MATCH_ANY,
    
    .device_class = 0,
    .device_class_mask = 0,
    
    .match_data = 0,
  }; // End matcher
  
  // Create the iterator and the base device
  struct pci_device_iterator *itr = pci_id_match_iterator_create(&matcher);
  struct pci_device *dev = NULL;
  
  // Loop until no more devices can be enumerated (.next => NULL)
  while (dev = pci_device_next(itr)) {
    // Allocate device object to store keys in
    VALUE new_dev = rb_hash_new();
    
    // Additional information struct
    struct pci_dev *dev_info = pci_get_dev(list,
      dev->domain_16, dev->bus, dev->dev, dev->func);
    
    // Use all flags possible
    int flags = INT_MAX;
    
    // Fill in the card information, as flagged above
    int e = pci_fill_info(dev_info, flags);
    
    // Core character data
    const char *device = pci_device_get_device_name(dev);
    const char *vendor = pci_device_get_vendor_name(dev);
    const char *subdevice = pci_device_get_subdevice_name(dev);
    const char *subvendor = pci_device_get_subvendor_name(dev);
    
    // Class structure
    int prog_interface = dev->device_class & 0x7F;
    int subclass = (dev->device_class & 0x7F80) >> 8;
    int class = (dev->device_class & 0x7F8000) >> 16;
    
    // Domain for the card
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("domain")), INT2NUM(dev->domain));
    
    // These should usually be formatted as "%02x" for display
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("class")), INT2NUM(class));
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("subclass")), INT2NUM(subclass));
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("prog_interface")),
      INT2NUM(prog_interface));
    
    // Slot string
    VALUE slot = rb_sprintf("%04x:%02x:%02x.%x",
      dev->domain_16, dev->bus, dev->dev, dev->func);
    
    // Add the slot name
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("dev_name")), slot);
    
    // Slot parts are the raw slot data
    VALUE slot_ary = rb_ary_new();
    rb_ary_store(slot_ary, 0, INT2NUM(dev->domain_16));
    rb_ary_store(slot_ary, 1, INT2NUM(dev->bus));
    rb_ary_store(slot_ary, 2, INT2NUM(dev->dev));
    rb_ary_store(slot_ary, 3, INT2NUM(dev->func));
    
    // Add the raw index portion
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("dev_raw")), slot_ary);
    
    // Attach core device info
    if (device) {
      rb_hash_aset(new_dev, rb_id2sym(rb_intern("device")),
        rb_str_new2(device)); }
    if (vendor) {
      rb_hash_aset(new_dev, rb_id2sym(rb_intern("vendor")),
        rb_str_new2(vendor)); }
    if (subdevice) {
      rb_hash_aset(new_dev, rb_id2sym(rb_intern("subdevice")),
        rb_str_new2(subdevice)); }
    if (subvendor) {
      rb_hash_aset(new_dev, rb_id2sym(rb_intern("subvendor")),
        rb_str_new2(subvendor)); }
        
    // Attach IDs
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("vendor_id")),
      INT2NUM(dev_info->vendor_id));
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("device_id")),
      INT2NUM(dev_info->device_id));
    
    // Driver information
    const char *phy_ptr = pci_get_string_property(dev_info, PCI_FILL_PHYS_SLOT);
    VALUE phys = Qnil;
    if (phy_ptr) { phys = rb_str_new2(phy_ptr); } // End physical slot reader
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("phys")), phys);
    
    // Driver information link
    // /proc/bus/pci/devices
    VALUE matches = rb_hash_new();
    rb_hash_aset(matches, rb_id2sym(rb_intern("bus")),
      rb_sprintf("%02x%02x", dev->bus, dev->func));
    rb_hash_aset(matches, rb_id2sym(rb_intern("idents")),
      rb_sprintf("%04x%04x", dev_info->vendor_id, dev_info->device_id));
    // Attach the matcher for /proc to the main device hash
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("matcher")), matches);
    
    // Modalias
    const char *mod_ptr = pci_get_string_property(dev_info, PCI_FILL_MODULE_ALIAS);
    VALUE module = Qnil;
    if (mod_ptr) {
      int len = strlen(mod_ptr);
      char mfix[len];
      memcpy(mfix, mod_ptr, len - 1);
      module = rb_str_new2(mfix);
    } // End module reader
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("modalias")), module);
    
    // BIOS Label
    const char *bios_ptr = pci_get_string_property(dev_info, PCI_FILL_LABEL);
    VALUE label = Qnil;
    if (bios_ptr) { label = rb_str_new2(bios_ptr); } // End label reader
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("label")), label);
    
    // Format these as "0x02"
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("revision")),
      INT2NUM(dev->revision));
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("rom_size")),
      INT2NUM(dev->rom_size));
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("irq")),
      INT2NUM(dev->irq));
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("numa_node")),
      INT2NUM(dev_info->numa_node));
    
    // IOMMU Group
    const char *iommu_ptr = pci_get_string_property(dev_info, PCI_FILL_IOMMU_GROUP);
    VALUE iommu = Qnil;
    if (iommu_ptr) { iommu = rb_str_new2(iommu_ptr); } // End IOMMU group access
    rb_hash_aset(new_dev, rb_id2sym(rb_intern("iommu")), iommu);
    
    // Attach the device to the devices list
    rb_ary_store(devices, count, new_dev);
    pci_free_dev(dev_info); // Clear the information pointer
    count++; // Increment the device counter
  } // End creation of new PCI device
  
  // Attach all the devices
  //rb_hash_aset(map, rb_id2sym(rb_intern("devices")), devices);
  // Clean up the resources
  pci_iterator_destroy(itr); // Delete iterator
  pci_cleanup(list); // Clean up PCI core system
  pci_system_cleanup(); // Clean up the access system
  return devices;
} // End test method

// Return the filters
VALUE method_pci_filters_get(VALUE self) {
  return PClassFilters;
} // End PCI filter return

// Set the filters
VALUE method_pci_filters_set(VALUE self, VALUE arg_arr) {
  // If it's an array, set the values
  if (RB_TYPE_P(arg_arr, T_ARRAY)) {
    int len = RARRAY_LEN(arg_arr);
    PClassFilters = arg_arr; // Apply the values
    return method_pci_filters_get(self); // Return the filter array
  } else { // Print an error, if it is not an array
    return rb_str_new2("::PciCore ERROR:: Args must be in the form of an array.");
  }
} // End PCI filter setter

// Clear the filters
VALUE method_pci_filters_clear(VALUE self) {
  VALUE empty = rb_ary_new(); // Array has no args
  /* 
   * Set with empty array.
   * 
   * Will automatically return the array, as it is
   * returned from the setter.
   * 
   * Cannot fail, as it is an empty array.
  */
  return method_pci_filters_set(self, empty);
} // End PCI filter clear

// Init the PCI Core extension
void Init_pci_core() {
  // Init the module
  PciCore = rb_define_module("PciCore");
  PClassFilters = rb_ary_new();
  
  // Module filter variables
  rb_define_readonly_variable("$pci_class_filters", &PClassFilters);
  
  // Functions
  rb_define_module_function(PciCore, "list", method_pci_list, 0);
  rb_define_module_function(PciCore, "get_filters", method_pci_filters_get, 0);
  rb_define_module_function(PciCore, "set_filters", method_pci_filters_set, 1);
  rb_define_module_function(PciCore, "clear_filters", method_pci_filters_clear, 0);
} // End init