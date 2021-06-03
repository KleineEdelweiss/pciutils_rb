# Rakefile
require "rake/extensiontask"

# Build the PciUtils extension
Rake::ExtensionTask.new "pci_core" do |ext|
  ext.lib_dir = "lib/pci_core"
  ext.source_pattern = "*.{c,h}"
end # End build on PciUtils