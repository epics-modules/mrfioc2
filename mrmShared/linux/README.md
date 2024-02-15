
# Cross compiling

make ARCH=powerpc CROSS_COMPILE=powerpc-linux-gnu- KERNELDIR=/path/to/headers

# udev

udev rules to automatically set permissions for approprieate /dev/uio*

Leaves ownership with the root user, but sets the group permissions to
allow IOC running from accounts in the softioc group R/W access.

cat << EOF > /etc/udev/rules.d/99-mrfioc2.rules
KERNEL=="uio*", ATTR{name}=="mrf-pci", GROUP="softioc", MODE="0660"
EOF

# dkms-rpm

To create an installable dksm package for this kernel module do the following:

1. Edit dkms/CONFIG to specify the group name for the target user
2. Run make in the dkms directory
3. Install the generated .rpm on your target system

# debian

On this folder runs:

dpkg-buildpackage

or

dpkg-buildpackage -b -uc -us # Not signing the package

A .deb package will be gerated one folder above.
