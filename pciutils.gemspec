# frozen_string_literal: true
require_relative "lib/pciutils/version.rb"

# Spec
Gem::Specification.new do |spec|
  spec.name = "pciutils"
  spec.version = PciUtils::VERSION
  spec.summary = "Linux PCI utility wrapper in the C-Ruby API"
  spec.description = <<~DESC
    Wrapper for ``libpciaccess`` and ``libpci``
  DESC
  spec.authors = ["Edelweiss"]
  
  # Website data
  #spec.homepage = "https://github.com/KleineEdelweiss/"
  spec.licenses = ["LGPL-3.0"]
  spec.metadata = {
    "homepage_uri"        => spec.homepage,
    "source_code_uri"     => "https://github.com/KleineEdelweiss/pciutils_rb",
    #"documentation_uri"   => "",
    "changelog_uri"       => "https://github.com/KleineEdelweiss/pciutils_rb/blob/master/CHANGELOG.md",
    "bug_tracker_uri"     => "https://github.com/KleineEdelweiss/pciutils_rb/issues"
    }
  
  # List of files
  spec.files = Dir.glob("lib/**/*")
  
  # Rdoc options
  spec.extra_rdoc_files = Dir["README.md","CHANGELOG.md", "LICENSE.txt"]
  spec.rdoc_options += [
    "--title", "PCI Utils -- Linux PCI utility wrapper in the C-Ruby API",
    "--main", "README.md",
    
    # Exclude task data from rdoc
    "--exclude", "Makefile",
    "--exclude", "Rakefile",
    "--exclude", "Gemfile",
    "--exclude", "pciutils.gemspec",
    "--exclude", "rdoc.sh",
    
    # Other options
    "--line-numbers",
    "--inline-source",
    "--quiet"
  ]
  
  # Minimum Ruby version
  spec.required_ruby_version = ">= 2.7.0"
  
  # Compiled extensions
  spec.extensions = ['ext/pci_core/extconf.rb']
end # End spec