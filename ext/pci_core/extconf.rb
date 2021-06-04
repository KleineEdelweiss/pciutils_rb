# ext/pci_core/extconf.rb
require 'mkmf'

# Build PCI
$LFLAGS = ['-lpci', '-lpciaccess', '-lkmod']

# Dependency checker
lchecks = []
hchecks = []
libs = ['pci', 'pciaccess', 'kmod']
headers = [
  'pci/pci.h',
  'pciaccess.h',
  'libkmod.h',
  'sys/utsname.h',
  #'proc_fs.h',
]

# Loop through the libraries and headers
libs.each { |l| lchecks << have_library(l) }
headers.each { |h| hchecks << have_header(h) }

# Library error message
libchk = if !lchecks.all? then 
  "HINT:: MISSING ONE OR MORE DEV LIBRARIES (SEE THE TESTS ABOVE)"
end

# Header error message
headchk = if !hchecks.all? then 
  "HINT:: MISSING ONE OR MORE DEV HEADERS (SEE THE TESTS ABOVE)"
end

# Border
EBORDER = "="*15

# Output reporting
if libchk || headchk then
# ERROR MESSAGE
emsg = <<DEPFAIL
  #{EBORDER}
  !!EXTENSION BUILD FAILED!!
  #{EBORDER}
  Please make sure all the dependencies are installed on your system!
  
  If you are unsure how to obtain these, you may need to consult your
  system's packages. These are dev files, and they do not always
  come pre-packaged.
  
  Example: linux-headers-#{`uname -r`.strip}
  Example: libpci-dev
  Example: libkmod-dev
  
  These dependency names will vary from one system / distro to another, so
  please check your docs.
  
  #{EBORDER}
  
  DETAILS:
  Libraries: #{libs}
  Headers: #{headers}
  
  SUMMARY:
  #{"#{libchk}\n  #{headchk}".strip}
  #{EBORDER}
  ::END ERROR REPORT::
  #{EBORDER}
DEPFAIL
# END ERROR MESSAGE
  
  # Abort the installation
  abort emsg
  
# Otherwise, has everything
else
  create_makefile("pci_core/pci_core")
# SUCCESS MESSAGE FOR PciCore
STDERR.puts <<PCISUCC
#{EBORDER}
  ::STATUS (PciCore):: dependencies satisfied; Makefile created
PCISUCC
end # End output report