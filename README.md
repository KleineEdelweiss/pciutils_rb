### OVERVIEW ###
``pciutils`` is a Linux wrapper for ``libpci`` and ``libpciaccess``, written in the C-Ruby API, and it provides the ability to access data about PCI devices on the system. The output is uniform and can allow for the data to be used in other programs, such as attaching PCI information to objects with data that is otherwise unrelated and extracted via some other means -- for example combining PCI device information with ``lmsensors`` values of a graphics card or processor.

This interface allows for filtering by types and exports the base-type definitions from the C-side (such as ``PCI_BASE_CLASS_MULTIMEDIA`` -> ``PciUtils::PCI_MEDIA`` [which would include PCI devices, such as audio cards, for example]).

### NOTES ###
The information is made to emulate the Linux command, ``lspci``. Most of the information is able to be acquired from the C-side of the code and, therefore, incurs very little overhead, even with a full, unfiltered device listing. However, driver information is mapped from the devices generated there, to the data in ``/proc/bus/pci/devices``. Still incurring lower overhead than mapping the text to be parsed from a shell call, this can be run as needed.

Because the information does not need to be refreshed for every device, with new data on every call, this library utilizes a Singleton approach and caches the data in ``PciUtils::Cache``, which inherits the abstract class, ``PciCore::AbsPci``, but updates can and should be performed every-so-often, if devices are likely to be added or removed from the system, or else if drivers may be swapped in and out (e.g.: an Open-Source graphics driver for games with a proprietary professional rendering or AI/ML driver).

Generally speaking, the update process is not necessary during a single run, for most use cases, but some hot-swap devices may benefit from it, so looping the update on a Thread or Ractor in the host program, every several seconds, is probably more than enough.

### INSTALLATION ###
This library requires Ruby extensions to be built natively, so you will need to install the package on your system correlating to ``ruby-dev``. It also requires additional headers from ``libpci`` and ``libpciaccess``, which can usually be installed like:

```sh
# Debian, Ubuntu, Mint, etc.
sudo apt install libpci-dev libpciaccess-dev ruby-dev libmruby-dev

# Fedora, RHEL, CentOS, etc.
sudo [yum, dnf] install libpci-devel libpciaccess-devel ruby-devel[-version]

# Arch, etc.
# Ruby libs are included with a Ruby environment,
# and libpci appears to come with pciutils.
sudo yay -Sy libpciaccess pciutils
```

### TO-DO ###
1) 