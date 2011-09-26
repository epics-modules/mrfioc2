#ifndef MAINPAGE_H
#define MAINPAGE_H

// No code.  Just Doxygen documentation

/**

@mainpage mrfioc2 Timing System Driver for MRF products

@section whatisit What is it?

A driver for VME and PCI cards from Micro Research Finland for implementing a distributed
timing system.

@url http://www.mrf.fi/

@section whereis Source

Releases can be found at @url http://sourceforge.net/projects/epics/files/mrfioc2/

This module is versioned with Mercurial and can be viewed at
@url http://epics.hg.sourceforge.net/hgweb/epics/mrfioc2/

Or checked out with

hg clone http://epics.hg.sourceforge.net:8000/hgroot/epics/mrfioc2

The canonical version of this page is @url http://epics.sourceforge.net/mrfioc2/

@subsection requires Requires

EPICS Base >= 3.14.8.2

@url http://www.aps.anl.gov/epics/

MSI (Macro expension tool)

@url http://www.aps.anl.gov/epics/extensions/msi/index.php

devLib2

@url http://epics.sourceforge.net/devlib2/

@section hardware Supported Hardware

Event Generators.  Current only the VME-EVG-230

Event Receivers.  VME-EVR-230RF, PMC-EVR-230, cPCI-EVR-230, cPCI-EVRTG-300

@note Support for the VME-EVR-230 (non-RF) is present, but has not been tested.

@section doc Documentation

User documentation can be found in the form of usage manuals for both the
<a href="evr-usage.pdf">Receiver</a>
and
<a href="evg-usage.pdf">Generator</a>

Those interested in the implementation for the Receiver might wish to start with mrmEvrSetupPCI()
and mrmEvrSetupVME() or the ::EVRMRM class.

For the generator see mrmEvgSetupVME() or the ::evgMrm class.

@section changelog Changelog

@subsection ver20 2.0 (Sept. 2011)

@li Initial release.

@author Michael Davidsaver <mdavidsaver@bnl.gov>

@author Jayesh Shah <jshah@bnl.gov>

@author Eric Björklund <bjorklund@lanl.gov>

*/

#endif // MAINPAGE_H
