# ext/pci_core/extconf.rb
require 'mkmf'

# Build PCI
$LFLAGS = ['-lpci', '-lpciaccess', '-lpciutils']
have_library("pci")
have_library("pciaccess")
have_header("pci/pci.h")
have_header("pciaccess.h")
create_makefile("pci_core/pci_core")