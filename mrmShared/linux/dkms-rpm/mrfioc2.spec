%define kmod_name mrfioc2
%define udev_rules_update \
  /sbin/udevadm control --reload-rules && \
  /sbin/udevadm trigger

Summary: DKMS mrfioc2 kernel module

Name:    mrfioc2-dkms
Version: 3
Release: 1

Vendor: EPICS Community
License: EPICS Open License
Group: System Environment/Kernel
URL: https://github.com/epics-modules/mrfioc2
BuildRequires: autoconf libtool gcc gcc-c++
BuildArch: x86_64
Requires: dkms
Requires: udev
BuildRoot: %{_tmppath}/%{kmod_name}-%{version}-root

# The two target directories
%define dkmsdir /usr/src/%{kmod_name}-%{version}
%define udevdir /etc/udev/rules.d

%description
Installs the kernel driver for interfacing to the MRF timing cards over
PCI express. The driver is installed using dkms.

%clean
rm -rf %{buildroot}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{dkmsdir} %{buildroot}%{udevdir}
install -m 0644 %{_sourcedir}/uio_mrf.c     %{buildroot}%{dkmsdir}
install -m 0644 %{_sourcedir}/jtag_mrf.c    %{buildroot}%{dkmsdir}
install -m 0644 %{_sourcedir}/mrf.h         %{buildroot}%{dkmsdir}
install -m 0644 %{_curdir}/Makefile.dkms    %{buildroot}%{dkmsdir}/Makefile
install -m 0644 %{_sourcedir}/Kbuild        %{buildroot}%{dkmsdir}
install -m 0644 %{_curdir}/dkms.conf        %{buildroot}%{dkmsdir}
install -m 0644 %{_curdir}/50-mrfioc2.rules     %{buildroot}%{udevdir}

%files
%{dkmsdir}/uio_mrf.c
%{dkmsdir}/jtag_mrf.c
%{dkmsdir}/mrf.h
%{dkmsdir}/Makefile
%{dkmsdir}/Kbuild
%{dkmsdir}/dkms.conf
%{udevdir}/50-mrfioc2.rules

%post
# PRP: groupadd -r mrfioc2 2>&1 || :
dkms add     -m %{kmod_name} -v %{version} --rpm_safe_upgrade
dkms build   -m %{kmod_name} -v %{version}
dkms install -m %{kmod_name} -v %{version}
modprobe %{kmod_name}
%udev_rules_update

%preun
modprobe -r %{kmod_name}
dkms remove --all -m %{kmod_name} -v %{version} --rpm_safe_upgrade --all ||:
%udev_rules_update
# PRP: group root groupdel mrfioc2 2>&1 || :

%postun
rmdir /usr/src/%{kmod_name}-%{version}

%changelog

* Wed Jan 24 2024 Jerzy Jamroz <jerzy.jamroz@gmail.com> - 3-1
- Build adjustments exposing the control variables.
- Pkg name changed to mrfioc2-dkms.
- Correction of the driver version handling.
- Group rw access for the pci resource0.
- AER handling functions.
- Device access group added.
- Tested on CentOS7.

* Mon Jan 22 2024 Jerzy Jamroz <jerzy.jamroz@gmail.com> - 2-1
- Version 2 skipped due to usage as version 1.
- Tested on CentOS7.

* Mon Dec 14 2020 Michael Abbott <michael.abbott@diamond.ac.uk> - 1-1
- Initial release.
