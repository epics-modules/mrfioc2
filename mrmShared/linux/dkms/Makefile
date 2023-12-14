# Rules to build a DKMS RPM file

SRCDIR = $(CURDIR)/..

DRV_NAME = mrf
include $(SRCDIR)/VERSION
include CONFIG


# The RPM_VERSION string takes the dotted version name (without extra version
# info) and follows the Red Hat versioning rules: only letters, digits, and
# periods.  See
#   https://docs.fedoraproject.org/en-US/packaging-guidelines/Versioning/
# for information on RPM versioning.  We'll want to obey these rules if we can.
RPM_VERSION = $(DRV_VERSION)


RPM_FILE = $(DRV_NAME)-dkms-$(RPM_VERSION)-1.noarch.rpm

driver-rpm: $(RPM_FILE)
.PHONY: driver-rpm

clean:
	rm -f $(SED_FILES) *.rpm
	rm -rf rpmbuild
.PHONY: clean


SED_SUBSTS =
SED_SUBSTS += s/@@DRV_VERSION@@/$(DRV_VERSION)/g;
SED_SUBSTS += s/@@DRV_NAME@@/$(DRV_NAME)/g;
SED_SUBSTS += s/@@RPM_VERSION@@/$(RPM_VERSION)/g;
SED_SUBSTS += s/@@MRF_USER@@/$(MRF_USER)/g;
SED_SUBSTS += s/@@DRV_RELEASE@@/$(DRV_RELEASE)/g;

%: %.in CONFIG $(SRCDIR)/VERSION
	sed '$(SED_SUBSTS)' $< >$@

SED_FILES = dkms.conf $(DRV_NAME).spec 50-mrf.rules


$(RPM_FILE): $(SED_FILES) $(wildcard $(SRCDIR)/*)
	mkdir -p rpmbuild/RPMS rpmbuild/BUILD
	rpmbuild -bb \
            --define '_curdir $(CURDIR)' \
            --define '_topdir %{_curdir}/rpmbuild' \
            --define '_sourcedir $(SRCDIR)' \
            --define '_tmppath %{_topdir}/BUILD' \
            $(DRV_NAME).spec
	ln -sf rpmbuild/RPMS/noarch/$(RPM_FILE) .
