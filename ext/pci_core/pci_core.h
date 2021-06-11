// ext/pci_core/pci_core.h

// Ruby module
#include <ruby.h>

// Device modules
#include <pci/pci.h>
#include <pciaccess.h>

// Accepted enum types
#define PCI_ENUM_COUNT INT2NUM(1)
#define PCI_ENUM_VERIFY INT2NUM(2)
#define PCI_ENUM_LIST INT2NUM(3)

// Use all flags possible for filling in devices
#define DEV_FLAGS INT_MAX

// Create the enum list
void method_store_enums(VALUE object) {
  object = rb_ary_new();
  rb_ary_push(object, PCI_ENUM_COUNT);
  rb_ary_push(object, PCI_ENUM_VERIFY);
  rb_ary_push(object, PCI_ENUM_LIST);
} // End enum list generator

// Method to attach constants to the PciCore module
VALUE method_attach_constants(VALUE mod) {
  /*
   * Local constants used to refer to the type of enumeration
   * being performed -- the actual numbers are just arbitrary
   * low values, but they expose an identifiable symbol to use.
   */
  // Count the number of PCI devices, only
  rb_define_const(mod, "PCI_ENUM_COUNT", PCI_ENUM_COUNT);
  // Verify the device count and return a list of modaliases
  rb_define_const(mod, "PCI_ENUM_VERIFY", PCI_ENUM_VERIFY);
  // List all of the devices
  rb_define_const(mod, "PCI_ENUM_LIST", PCI_ENUM_LIST);
  
  // Export several of the PCI types defined by the actual library
  rb_define_const(mod, "PCI_UNDEF", INT2NUM(PCI_CLASS_NOT_DEFINED));
  rb_define_const(mod, "PCI_STORAGE", INT2NUM(PCI_BASE_CLASS_STORAGE));
  rb_define_const(mod, "PCI_NETWORK", INT2NUM(PCI_BASE_CLASS_NETWORK));
  rb_define_const(mod, "PCI_DISPLAY", INT2NUM(PCI_BASE_CLASS_DISPLAY));
  rb_define_const(mod, "PCI_MEDIA", INT2NUM(PCI_BASE_CLASS_MULTIMEDIA));
  rb_define_const(mod, "PCI_MEMORY", INT2NUM(PCI_BASE_CLASS_MEMORY));
  rb_define_const(mod, "PCI_BRIDGE", INT2NUM(PCI_BASE_CLASS_BRIDGE));
  rb_define_const(mod, "PCI_COMM", INT2NUM(PCI_BASE_CLASS_COMMUNICATION));
  rb_define_const(mod, "PCI_SYSTEM", INT2NUM(PCI_BASE_CLASS_SYSTEM));
  rb_define_const(mod, "PCI_INPUT", INT2NUM(PCI_BASE_CLASS_INPUT));
  rb_define_const(mod, "PCI_DOCKING", INT2NUM(PCI_BASE_CLASS_DOCKING));
  rb_define_const(mod, "PCI_PROCESSOR", INT2NUM(PCI_BASE_CLASS_PROCESSOR));
  rb_define_const(mod, "PCI_SERIAL", INT2NUM(PCI_BASE_CLASS_SERIAL));
  rb_define_const(mod, "PCI_WIRELESS", INT2NUM(PCI_BASE_CLASS_WIRELESS));
  rb_define_const(mod, "PCI_INTELLIGENT", INT2NUM(PCI_BASE_CLASS_INTELLIGENT));
  rb_define_const(mod, "PCI_SATELLITE", INT2NUM(PCI_BASE_CLASS_SATELLITE));
  rb_define_const(mod, "PCI_CRYPT", INT2NUM(PCI_BASE_CLASS_CRYPT));
  rb_define_const(mod, "PCI_SIGNAL", INT2NUM(PCI_BASE_CLASS_SIGNAL));
  rb_define_const(mod, "PCI_OTHER", INT2NUM(PCI_CLASS_OTHERS));
} // End constants attach