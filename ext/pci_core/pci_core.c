// ext/pci_core/pci_core.c

// Ruby module
#include <ruby.h>
#include <ruby/re.h>

// System modules
#include <stdio.h>
#include <stdlib.h>

// Device modules
#include <pci/pci.h>
#include <pciaccess.h>

// Other modules
#include <string.h>

// Local modules
#include "./pci_core.h"

// Initializers
VALUE PciCore = Qnil;
VALUE PciCoreAbs = Qnil;
VALUE PClassFilters = Qnil;
VALUE PciValidEnums = Qnil;

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

// Mimic 'strip' in the background,
// but do not expose the method
VALUE method_hidden_strip(VALUE self, VALUE string) {
  // Check string
  if (!RB_TYPE_P(string, T_STRING)) { return Qnil; }
  VALUE str = rb_str_to_str(string);
  
  // Initialize the regex strippers
  VALUE lstrip = rb_reg_regcomp(rb_str_new2("\\w"));
  VALUE rstrip = rb_reg_regcomp(rb_str_new2("\\s+$"));
  
  // Get the length
  long len = NUM2LONG(rb_str_length(str));
  
  // Return early, if no length
  if (len < 1) { return str; }
  
  // Find the indices
  VALUE start = rb_reg_match(lstrip, str);
  VALUE end = rb_reg_match(rstrip, str);
  
  // Fix the values
  long st = (NIL_P(start) ? 0 : NUM2LONG(start));
  long en = (NIL_P(end) ? len : NUM2LONG(end));
  
  // Return the new string
  return rb_str_substr(str, st, (en - st));
} // End strip

// Get the modalias of the device
VALUE method_get_modalias(VALUE self, struct pci_dev *dev_info) {
  // Modalias
  const char *mod_ptr = pci_get_string_property(dev_info, PCI_FILL_MODULE_ALIAS);
  VALUE module = Qnil;
  if (mod_ptr) {
    module = method_hidden_strip(self, rb_str_new2(mod_ptr));
  } else {
    module = rb_str_new2("");
  } // End module reader
  
} // End modalias method

// Separate method to attach individual devices to the
// device list map.
VALUE method_hidden_attach_dev(
  VALUE self,
  struct pci_access *list,
  struct pci_device *dev,
  struct pci_dev *dev_info,
  int class
) {
  // Class structure
  // This was moved to the top, because it will
  // be used to determine filters
  int subclass = (dev->device_class & 0x7F80) >> 8;
  int prog_interface = dev->device_class & 0x7F;
    
  // Allocate device object to store keys in
  VALUE new_dev = rb_hash_new();
  
  // Core character data
  const char *device = pci_device_get_device_name(dev);
  const char *vendor = pci_device_get_vendor_name(dev);
  const char *subdevice = pci_device_get_subdevice_name(dev);
  const char *subvendor = pci_device_get_subvendor_name(dev);
  
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
  
  // BIOS Label
  const char *bios_ptr = pci_get_string_property(dev_info, PCI_FILL_LABEL);
  VALUE label = Qnil;
  if (bios_ptr) {
    label = method_hidden_strip(self, rb_str_new2(bios_ptr));
  } else {
    label = rb_str_new2("");
  } // End label reader
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
  if (iommu_ptr) {
    iommu = method_hidden_strip(self, rb_str_new2(iommu_ptr));
  } else {
    iommu = rb_str_new2("");
  } // End IOMMU group access
  rb_hash_aset(new_dev, rb_id2sym(rb_intern("iommu")), iommu);
  return new_dev;
} // End attach hidden method

// List all of the PCI devices
VALUE method_pro_pci_list(VALUE self, VALUE count_type) {
  // Init main PCI system
  pci_system_init();
  // Allocate PCI device access list
  struct pci_access *list = pci_alloc();
  pci_init(list); // Init resources
  pci_scan_bus(list); // Scan all devices
  
  // Array of devices or count of devices
  VALUE devices = Qnil;
  
  // If count_type is NOT the count value, create a device ARRAY
  switch (count_type) {
    case PCI_ENUM_LIST:
    case PCI_ENUM_VERIFY:
      // Make the array and continue
      devices = rb_ary_new();
      break;
    case PCI_ENUM_COUNT:
      break; // It will just attach at the end
    default:
      return rb_str_new2("::PciCore ERROR:: Invalid enum selection");
  } // End count checker type
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
  
  // Get the filter length here to avoid extra check overhead
  int f_len = RARRAY_LEN(PClassFilters);
  
  // Loop until no more devices can be enumerated (.next => NULL)
  while (dev = pci_device_next(itr)) {
    // Class structure
    // Class is still kept in this method, because it
    // needs to be calculated to determine whether
    // to further create the device. Subclass and PI
    // will be created in the new method.
    int class = (dev->device_class & 0x7F8000) >> 16;
    
    // Iterate over the filters to see if the
    // device matches. If filters length is 0,
    // then skip.
    if (f_len > 0) {
      if (!rb_ary_includes(PClassFilters, INT2NUM(class))) { continue; }
    } // End filter checker
    
    // Additional information struct
    struct pci_dev *dev_info = pci_get_dev(list,
      dev->domain_16, dev->bus, dev->dev, dev->func);
    
    // Fill in the card information, as flagged above
    int e = pci_fill_info(dev_info, DEV_FLAGS);
    
    // Choose the correct operations, based on
    // the type of count requested.
    VALUE new_dev = Qnil;
    switch (count_type) {
      case PCI_ENUM_COUNT:
        break; // Do nothing, b/c only the count matters
      case PCI_ENUM_LIST:
        // Make the new_dev the hash
        new_dev = method_hidden_attach_dev(self, list, dev, dev_info, class);
      case PCI_ENUM_VERIFY:
        // Add the modalias, then add the
        // device hash to the array.
        { VALUE module = method_get_modalias(self, dev_info);
        if (RB_TYPE_P(new_dev, T_HASH)) {
          rb_hash_aset(new_dev, rb_id2sym(rb_intern("modalias")), module);
        } else { new_dev = module; }
        rb_ary_store(devices, count, new_dev); }
        break;
    } // End switch for listing type selection
    
    pci_free_dev(dev_info); // Clear the information pointer
    count++; // Increment the device counter
  } // End creation of new PCI device
  
  // If only wanted count, attach the count
  if (count_type == PCI_ENUM_COUNT) { devices = INT2NUM(count); }
  
  // Clean up the resources
  pci_iterator_destroy(itr); // Delete iterator
  pci_cleanup(list); // Clean up PCI core system
  pci_system_cleanup(); // Clean up the access system
  return devices;
} // End test method

// Return the filters
VALUE method_pro_pci_filters_get(VALUE self) {
  return PClassFilters;
} // End PCI filter return

// Set the filters
VALUE method_pro_pci_filters_set(VALUE self, VALUE arg_arr) {
  // If it's an array, set the values
  if (RB_TYPE_P(arg_arr, T_ARRAY)) {
    // Get the length
    int len = RARRAY_LEN(arg_arr);
    // Allocate the new array and loop through
    // the arguments.
    VALUE new_filters = rb_ary_new();
    for (int i = 0; i < len; i++) {
      // Pop from the argument array
      // but only add it, if it's numeric.
      VALUE item = rb_ary_pop(arg_arr);
      if (FIXNUM_P(item)) { rb_ary_push(new_filters, item); }
    } // End parameter loop
    
    // Sort the new array, and apply it to the filter list
    PClassFilters = rb_ary_sort(new_filters);
    return method_pro_pci_filters_get(self); // Return the filter array
  } else { // Print an error, if it is not an array
    return rb_str_new2("::PciCore ERROR:: Args must be in the form of an array.");
  }
} // End PCI filter setter

// Clear the filters
VALUE method_pro_pci_filters_clear(VALUE self) {
  VALUE empty = rb_ary_new(); // Array has no args
  /* 
   * Set with empty array.
   * 
   * Will automatically return the array, as it is
   * returned from the setter.
   * 
   * Cannot fail, as it is an empty array.
  */
  return method_pro_pci_filters_set(self, empty);
} // End PCI filter clear

// Init the PCI Core extension
void Init_pci_core() {
  // Init the module
  PciCore = rb_define_module("PciCore");
  PciCoreAbs = rb_define_class_under(PciCore, "AbsPci", rb_cObject);
  PClassFilters = rb_ary_new();
  
  // Store the enums
  method_store_enums(PciValidEnums);
  
  // Attach constants for filtering
  method_attach_constants(PciCore);
  
  // Module filter variables
  rb_define_readonly_variable("$pci_class_filters", &PClassFilters);
  
  // Functions
  rb_define_protected_method(PciCoreAbs, "pro_list", method_pro_pci_list, 1);
  rb_define_protected_method(PciCoreAbs, "pro_get_filters", method_pro_pci_filters_get, 0);
  rb_define_protected_method(PciCoreAbs, "pro_set_filters", method_pro_pci_filters_set, 1);
  rb_define_protected_method(PciCoreAbs, "pro_clear_filters", method_pro_pci_filters_clear, 0);
} // End init