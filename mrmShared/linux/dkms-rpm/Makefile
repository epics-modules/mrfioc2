# Rules to build a DKMS RPM file

SRCDIR = $(CURDIR)/..

include $(SRCDIR)/CONFIG
include CONFIG

# Determine the architecture of the machine
ARCH := $(shell uname -m)

# The RPM_VERSION string takes the dotted version name (without extra version
# info) and follows the Red Hat versioning rules: only letters, digits, and
# periods.  See
#   https://docs.fedoraproject.org/en-US/packaging-guidelines/Versioning/
# for information on RPM versioning.  We'll want to obey these rules if we can.
RPM_VERSION = $(DRV_VERSION)

RPM_FILE = $(PKG_NAME)-$(RPM_VERSION)-$(RPM_RELEASE).$(ARCH).rpm

driver-rpm: $(RPM_FILE)
.PHONY: driver-rpm

clean:
	rm -f $(SED_FILES) *.rpm
	rm -rf rpmbuild
.PHONY: clean


SED_SUBSTS =
SED_SUBSTS += s/@@DRV_VERSION@@/$(DRV_VERSION)/g;
SED_SUBSTS += s/@@KMOD_NAME@@/$(KMOD_NAME)/g;
SED_SUBSTS += s/@@RPM_VERSION@@/$(RPM_VERSION)/g;
SED_SUBSTS += s/@@DRV_GROUP@@/$(DRV_GROUP)/g;
SED_SUBSTS += s/@@RPM_RELEASE@@/$(RPM_RELEASE)/g;
SED_SUBSTS += s/@@ARCH@@/$(ARCH)/g;
SED_SUBSTS += s/@@PKG_NAME@@/$(PKG_NAME)/g;

%: %.in CONFIG $(SRCDIR)/CONFIG
	sed '$(SED_SUBSTS)' $< >$@

SED_FILES = dkms.conf mrf.spec 50-mrf.rules


$(RPM_FILE): $(SED_FILES) $(wildcard $(SRCDIR)/*)
	mkdir -p rpmbuild/RPMS rpmbuild/BUILD
	rpmbuild -bb \
            --define '_curdir $(CURDIR)' \
            --define '_topdir %{_curdir}/rpmbuild' \
            --define '_sourcedir $(SRCDIR)' \
            --define '_tmppath %{_topdir}/BUILD' \
            mrf.spec
	ln -sf rpmbuild/RPMS/$(ARCH)/$(RPM_FILE) .
