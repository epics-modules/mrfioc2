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

devLib2 >= 2.6

@url http://epics.sourceforge.net/devlib2/

RTEMS >= 4.9.x, vxWorks >=6.7, or Linux >= 2.6.26.

@section hardware Supported Hardware

Event Generators.  Current only the VME-EVG-230

Event Receivers.  VME-EVR-230RF, PMC-EVR-230, cPCI-EVR-230, cPCI-EVRTG-300, PCI-EVRTG-300e

@note Support for the VME-EVR-230 (non-RF) is present, but has not been tested.
@note Support for the PCI-EVRTG-300e is present, but has not been testedl

@section doc Documentation

User documentation can be found in the form of usage manuals for both the
<a href="evr-usage.pdf">Receiver</a>
and
<a href="evg-usage.pdf">Generator</a>

Those interested in the implementation for the Receiver might wish to start with mrmEvrSetupPCI()
and mrmEvrSetupVME() or the ::EVRMRM class.

For the generator see mrmEvgSetupVME() or the ::evgMrm class.

@section changelog Changelog

@subsection v204 2.0.4 (Jan 2015)

@subsubsection v204not Notices

@li Requires devLib >= 2.6l
@li EVR GTP is currently disabled
@li VME EVG and EVR are loosely tested (IRQs are working OK, hardware inputs/outputs are not yet tested but expected to work O.K.)
@li EVG databases fixed (in ..App/Db/PSI), originals moved to NSLS folder

@subsubsection v204bug

@li Fixed incorrect firmware version warning message
@li Reworked interrupt handling to fully eliminate race in IRQ acknowledge.
    requires new kernel module (linux only).

@subsubsection v204feat Features

@li Added support for PCI-EVRTG-300e (interrupts are not tested)
@li Added sequence masker aSub (bit mask to replace event codes with zero)
@li Created interrupt handler functions for PCI and VME to handle platform-specific interrupt configuration
@li Added start of sequence interrupt device support
@li Changed record naming convention (renamed all records)

@subsection v203 2.0.3 (Aug 2014)

@subsubsection v203bug Bug fixes

- Linux only.  Fix for ISR race condition which can leave interrupts disabled.
   Anyone running mrfioc2 on Linux is encouraged to update.
- fix rounding in CML/GTX WF calculator
- evgSoftSeq.py: fix sequences of length 1
- seqconst: avoid memory bounds violation
- fix EVG Univ inputs (fixed by Jonah Weber of LBNL)

@subsubsection v203feat Features

- linux: emulate parallel port JTAG cable.
   Allows firmware update w/o the Linux GPIO userspace interface (still not built on Debian stock kernels)
- Python helper scripts with example for NSLS2 EVG magic numbers.

@subsection v202 2.0.2 (Aug 2013)

@subsubsection v202not Notices

- I am considering removal of the EVR interface class.  This was originally
  intended to allow "similar" hardware (pre-MRM cards from MRF).  However,
  to my knowledge this has not been done.  If anyone is using this feature
  please contact me (mdavidsaver@bnl.gov) or it will likely be removed
  in the next (2.1) release.

@subsubsection v202bug Bug fixes

- Fixed issues with EVG sequencer which allowed some user inputs at inappropriate times.
     This could cause the sequencer to stop.
     Sequencer controls now validated against internal state.
- wrong width for RVAL causes endianness issue
- re-enable of CML output during setMode not conditional
- Fix EVG driver init w/o hardware.  This was crashing.
- Update locking for EVR.  Take lock for all device support actions.
- Fix locking error causing EVR driver to hang during IOC shutdown.
- evgSoftSeq.py handle readback of zero length sequences (the initial condition)

@subsubsection v202feat Features

- Corrected the number of pulsers (delay generators) in EVRs. This adds 6 for a total of 16.
- Updated recommended firmware version for PCI EVRs to 6.
- Compile in VCS version or release number.  Add a PV which reads this.
- Read SFP EEPROM information (eg. module serial#, temperature, and incoming optical power).
    Requires firmware >=5.  For version 5 must be from 25 May 2012 or later.
- Add mapping record for Prescaler reset action to the example EVR databases.
- Support and documentation for firmware update of PMC-EVR-230 devices on Linux.
- PV with device position (VME slot or PCI BDF ids)
- Add aSub functions to support NSLS2 injector timing sequence constructor
    - Seq Repeat - Repeats a fixed sequence at specific intervals.  Includes
                   a bit mask to mask out certain occurences.
    - Seq Merge  - Merge two or more sorted sequences while maintaining sorting.
- evgSoftSeq.py In addition to the previous form, the sequence editor GUI now
                 accepts a single command line argument with the PV name prefix.
- evgSoftSeq.py Improved connection handling.
- configure/RELEASE Optionally include caputlog module
- Added enable/disable control for individual EVR outputs.
   Disabled output are mapped to Force Low.
- On Linux, allow the EVR driver to provide time to the system NTPD.
   Allows system clock to be synced to EVR with much lower jitter
   then network NTP server.
- EVG sequencer in single shot mode now shows status disabled on completion.
- Add evrsoftgate.db which uses a hardware event to enable/disable an
  output using a software timer.
- Add evgUserEvt.db which gives a "safe" software event send control.


@warning The default mapping for prescaler reset is now disabled.
         The included database files have been updated.
         Anyone who has created a custom database should update their database
         to include a "Reset PS" mapping record!

@subsection v201 2.0.1 (Apr. 2012)

@subsubsection v201bug Bug fixes

@li Fix several vxWorks build issues
@li Correct initial mapping for EVR output channels to Force Low (aka. Off)
@li Fix readback of EVG sequencer run mode.
@li Limit number of soft event send retries
@li More check for EVG and EVR during initialization.
     Should now catch old firmware versions and CSR address mapping problems.
@li Delay enabling VME interrupts for EVG until later during IOC startup.
@li Fix autosave/restore of CML output bit patterns.
@li Remove rear transition module definitions from default EVG db template
@li Fix locking issue in data buffer tx/rx.
    A deadlock would occur when trying to send a buffer with the link mode
    set to dbus only.

@subsubsection v201feat Features

@li Added evralias.db to facilitate creation of PV name aliases for EVR
    delay generator channels.
@li Always reset all EVG multiplexed counters when a divider value is changed.
@li Add counter to track number of times each EVG sequencer is run.
@li Add mrmEvrForward() shell function to configure EVR event forwarding to downstream EVRs.

@subsection ver20 2.0 (Sept. 2011)

@li Initial release.

@author Michael Davidsaver <mdavidsaver@bnl.gov>

@author Jayesh Shah <jshah@bnl.gov>

@author Eric Björklund <bjorklund@lanl.gov>

*/

#endif // MAINPAGE_H
