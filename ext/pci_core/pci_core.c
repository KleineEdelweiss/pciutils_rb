// ext/pci_core/pci_core.c
// Ruby module
#include <ruby.h>

// System modules
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>

// Device modules
#include <pci/pci.h>
#include <pciaccess.h>
#include <libkmod.h>

// Other modules
#include <string.h>

// Initializers
VALUE PciCore = Qnil;

/*// Test the device class
VALUE method_pci_types(VALUE self) {
  VALUE map = rb_hash_new();
  rb_hash_aset(map, rb_id2sym(rb_intern("processor")),
    INT2NUM(PCI_BASE_CLASS_PROCESSOR));
  rb_hash_aset(map, rb_id2sym(rb_intern("memory")),
    INT2NUM(PCI_BASE_CLASS_MEMORY));
  
  return map;
} // End device class test
*/

// List all of the PCI devices
VALUE method_pci_list(VALUE self) {
  // Get the machine architecture, for 
  struct utsname sysinfo;
  uname(&sysinfo);
  
  // Generate the modalias path
  VALUE path = rb_sprintf("/lib/modules/%s/modules.alias", sysinfo.release);
  
  // Init main PCI system
  pci_system_init();
  // Allocate PCI device access list
  struct pci_access *list = pci_alloc();
  pci_init(list); // Init resources
  pci_scan_bus(list); // Scan all devices
  
  // Create the new mapping hash
  VALUE map = rb_hash_new();
  rb_hash_aset(map, rb_id2sym(rb_intern("modalias")), path);
  
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
    
    // Driver information
    // /lib/modules/`uname -r`/modules.alias
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
  rb_hash_aset(map, rb_id2sym(rb_intern("devices")), devices);
  // Clean up the resources
  pci_iterator_destroy(itr); // Delete iterator
  pci_cleanup(list); // Clean up PCI core system
  pci_system_cleanup(); // Clean up the access system
  return map;
} // end test method

// Init the PCI Core extension
void Init_pci_core() {
  // Init the module
  PciCore = rb_define_module("PciCore");
  
  // Functions
  rb_define_module_function(PciCore, "list", method_pci_list, 0);
  //rb_define_module_function(PciCore, "types", method_pci_types, 0);
} // End init