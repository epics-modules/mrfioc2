# Rules to build a DKMS RPM file

SRCDIR = $(CURDIR)/..

DRV_NAME = mrf
include $(SRCDIR)/VERSION

# The RPM_VERSION string takes the dotted version name (without extra version
# info) and follows the Red Hat versioning rules: only letters, digits, and
# periods.  See
#   https://docs.fedoraproject.org/en-US/packaging-guidelines/Versioning/
# for information on RPM versioning.  We'll want to obey these rules if we can.
RPM_VERSION = $(DRV_VERSION)


RPM_FILE = $(DRV_NAME)-$(RPM_VERSION)-1dkms.noarch.rpm

driver-rpm: $(RPM_FILE)
.PHONY: driver-rpm

clean:
	rm -f dkms.conf $(DRV_NAME).spec *.rpm
	rm -rf rpmbuild
.PHONY: clean


SED_SUBSTS =
SED_SUBSTS += s/@@DRV_VERSION@@/$(DRV_VERSION)/g;
SED_SUBSTS += s/@@DRV_NAME@@/$(DRV_NAME)/g;
SED_SUBSTS += s/@@RPM_VERSION@@/$(RPM_VERSION)/g

dkms.conf: dkms.conf.in $(SRCDIR)/VERSION
	sed '$(SED_SUBSTS)' $< >$@

$(DRV_NAME).spec: $(DRV_NAME).spec.in $(SRCDIR)/VERSION
	sed '$(SED_SUBSTS)' $< >$@


$(RPM_FILE): dkms.conf $(DRV_NAME).spec $(wildcard $(SRCDIR)/*)
	mkdir -p rpmbuild/RPMS rpmbuild/BUILD
	rpmbuild -bb \
            --define '_curdir $(CURDIR)' \
            --define '_topdir %{_curdir}/rpmbuild' \
            --define '_sourcedir $(SRCDIR)' \
            --define '_tmppath %{_topdir}/BUILD' \
            $(DRV_NAME).spec
	ln -sf rpmbuild/RPMS/noarch/$(RPM_FILE) .
