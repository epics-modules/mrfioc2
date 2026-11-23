#ifndef MAINPAGE_H
#define MAINPAGE_H

// No code.  Just Doxygen documentation

/**

@mainpage mrfioc2 Timing System Driver for MRF products

@section whatisit What is it?

A driver for VME and PCI cards from Micro Research Finland for
event timing systems.

@url http://www.mrf.fi/

@section whereis Source

Releases can be found at @url https://github.com/epics-modules/mrfioc2/releases

This module is versioned with Git and can be viewed at
@url https://github.com/epics-modules/mrfioc2/

Or checked out with

git clone https://github.com/epics-modules/mrfioc2.git

The canonical version of this page is @url http://epics-modules.github.io/mrfioc2

@subsection requires Requires

- epics-base @url https://github.com/epics-base/epics-base >= 3.15.1,
- devLib2 @url https://github.com/epics-modules/devlib2 >= 2.8.
- RTEMS >= 4.9.x, vxWorks >=6.7, or Linux >= 2.6.26.

@section hardware Supported Hardware

- Event Generators
 - VME-EVG-230
 - PXI-EVG-220
 - PXI-EVG-230
 - cPCI-EVG-300
 - mTCA-EVM-300

- Event Receivers
 - VME-EVR-230RF
 - VME-EVR-230 (non-RF)
 - PMC-EVR-230
 - cPCI-EVR-230
 - cPCI-EVRTG-300
 - cPCI-EVR-300
 - PCIe-EVR-300DC
 - mTCA-EVR-300
 - FRIB EVR (not MRF product)

@section doc Documentation

User documentation can be found in the form of usage manuals for both the
<a href="evr-usage.pdf">Receiver</a>
and
<a href="evg-usage.pdf">Generator</a>

The FPGA binaries and documentation releases are provided within the <a href="https://github.com/jpietari/mrf-firmware.git">mrf-firmware</a> repository.

The <a href="https://mrftiming.readthedocs.io/en/latest/index.html">Micro-Research Finland Event System Reference documentation</a> is available on Readthedocs for community contributions.

Those interested in the implementation for the Receiver might wish to start with mrmEvrSetupPCI()
and mrmEvrSetupVME() or the ::EVRMRM class.

For the generator see mrmEvgSetupVME() or the ::evgMrm class.

@section changelog Changelog

@subsection v280 2.8.0 (Nov 2026)

@li fix: Remove warning regarding paranthesis around operator '|'
@li fix: when evr_model is empty it should go to the default value
@li ci: uses ubuntu-24-04 for base 3.15
@li feat: add HwInfo support for EVR PCIe DC and DCS
@li feat: Include records to identify the type of UNIV
@li fix: add iocshSetError fallback for older EPICS Base
@li feat: report flashwrite failures to IOC shell
@li fix: IOC hang when switching away from software timestamps
@li fix C23 compilation
@li fix: Add the missing hexadecimal conversion specifier for the EVR status
@li feat: add Time-I for EVG
@li fix: calibrate the nextSecond
@li feat: allow clockTS to be lower than the RF clk.
@li fix: avoid duplicate soft timestamp event 125
@li fix: pini field of trigevt and dbus src sel record
@li fix call to evrFastEvtRecv.template in substitutions
@li refactor: simplify source selection logic for Dbus and TrigEvt and add MTCA EVM backplane trigger support for SEQ (#219)
@li feat: add mtca-evr-300rf uv db and fix input config for 300rf (#220)
@li Flash : add Winbond chip information (#218)
@li Feat : fast event receive for MTCA EVR (#216)
@li automatically include QSRV if in path
@li refactor: clean up comments
@li refactor: ResetFifo-Cmd
@li fix: add fiforst during the IOC startup
@li fix: add missing evm-fct-shutter.template
@li refactor: change evm-fct-ctrl.template to evm-fct-shutter.template
@li feat: add TxShutter to EVM
@li feat: integrate evm-fct-dis.template
@li feat: add RxShutDbus and RxShutEvt
@li test: RxShutter
@li feat: add RxShutter for EVMs
@li feat: add Rx/Tx Shutter registers
@li Include macros for DRVH on pulsers Width-SP
@li fix: use 32bit number for records field, instead of 0xffffffff
@li refactor: add proper naming so DCStatus
@li feat: add DCPortStatus for each EVM port.
@li fix: Ambiguously signed bit-field member
@li feat: GHA updates
@li perf: better EditorConfig handling
@li feat: add codeql for kmod
@li fix: gha directory references
@li Include ASG for all private and protected PVs
@li Move all ASG=private to ASG=$(ASGPRIVATE=private)
@li Merge branch 'master' into gha
@li fix: remove security warnings
@li refactor: correct indentation style
@li refactor: remove trailing whitespaces
@li feat: add more options to .editorconfig
@li fix: Multiplication result converted to larger type
@li fix: Potentially overrunning write with float to string conversion
@li fix: adjust codeql to listen the build
@li fix: add missing config.yml for codeql
@li refactor: use path istead of path-ignore for module build gha
@li feat: add codeql
@li fix: add missing new lines
@li feat: add "Check EditorConfig" gha
@li fix: gha permissions added
@li feat: Update issue templates
@li fix: add missing configurations for the __COMMIT_HASH macro
@li fix: Increase RX/TX HIGH limit to 1000
@li feat: PV Label-I added for evg.
@li fix: module-build.yml changed to ubuntu-latest as 20.04 is obsolete.
@li fix: Set default DESC macro to empty
@li fix driver error on rhel95

@subsection v272 2.7.2 (Dec 2024)

@li fix: Generally SFPs support -10C to +85C.
@li feat: Reset the EVR fractional synth command.
@li feat: Timestamp absolute extensions.
@li feat: autosave entries added for mrmevrtsbuf.db.
@li bug fix: remove macro s in Seq Trigger Source Selection OUT link
@li Improve deb file to use generic udev rules

@subsection v271 2.7.1 (Nov 2024)

@li ci: Universal naming.
@li refactor: KERNELDIR changed KERNEL_SRC, plus Kbuild tests.
@li fix: A missing gpl v2 license added for the kmod.
@li adding more db and template files to the install directory
@li fix: kmod flags and vars.
@li feat: Adjustment of the kmod build to be more generic.
@li feat: kmod support for Centos Stream 9
@li refactor: More universal handling of KSER and KVER.
@li perf: Faster CentosStream kernel test.
@li refactor: linux-build for Centos Stream 9.
@li feat: Centos Stream 9 support for the kmod.
@li fix: Ignore irrelevant CI runs.
@li feat: Centos Stream 9 linux-build.yml support.
@li feat: evrdump extended with mtca-evr-300.
@li fix: Version update and SwVer-I PV.

@subsection v270 2.7.0 (Nov 2024)

@li add the 3rd option of sequencer source, install the template of the evgSoftSeq
@li update the evr-vme-300.bob for default pulser and output tab
@li fixed a typo in the evr-vme-300 subsitution file
@li Fix the pv name in the dbus subscreen after the refactor of Omsl-Fout
@li feat: Screen files adjusted for 300 EVM and EVR
@li adding screen files for vme-300 series cards
@li docs: Codeowners support update.
@li fix: Cross Dependencies added to the driver workflow.
@li feat: Integration to ci-scripts v3.4.1.
@li feat: Integration to ci-scripts v3.4.1.
@li refactor: ASG private update.
@li refactor: $(s=:) cleanup from the internal records.
@li refactor: Omsl-FOut.
@li Add mbbi record for evg Dbus source sel. Fix the EVR record name for CML
@li feat: Upgrade of the "printregisters" EVG function.
@li fix: Print 16 bit registers as it is.
@li feat: Upgrade of the "printregisters" EVR function.
@li feat: Update of EVR REGINFO.
@li feat: Update of printreg with generic regs.
@li feat: Update of printreg with the IO-s.
@li feat: MTCA EVM expanded with the Backplane and Transition Board outputs
@li refactor: Proposal of the TB change to Rear.
@li feat: Backplane and TB outs added to the evm.subs.
@li feat: Implementation of the backplane inputs for EVM.
@li feat: TB and Backplane outputs added for EVM.
@li feat: Added backplane and TB maping for EVG/EVM outputs.
@li feat: db file update and bug fix
@li update the transition board output name for evr-vme-300
@li correct the EVM input mapping for evm-vme-300 and evm-mtca-300.uv
@li add Univ11-15 as Dbus source, to make it complete
@li fix the FLNK field of the Mxc reset control record
@li fix: The EVR output default is changed to tri-state.
@li feat: CML/GTX supports for EVR-VME300 with Power PC
@li Fixing the CPU word order for CML of EVR-VME-300 card, mtca is not affected
@li feat: Fine delay for rear transition board
@li update substitution files of evr-vme-300 for fine delay interface
@li Merge pull request #2 from jerzyjamroz/rb-delay
@li feat: Usage of mrmevroutdly.db added to the mtca sub.
@li refactor: mrmevroutdly.db added to manage the fine dly outs.
@li obsolete: mrmevrdlymodule.template -> mrmevrout.db.
@li fix: Minor update of the fine delay record support.
@li Letting the FP Univ module have the same interface for fine delay
@li adding record to support the fine delay
@li Add the support of delay tuning of the TTLDLY board on the transition board
@li Add the EVM frac Synth reset function
@li adding virtual univ inputs for EVM-VME-300 for EVRU/D event forwarding
@li feat: Adding more selection options in evm db files
@li Merge pull request #1 from jerzyjamroz/db-vme300-dev
@li fix: mbbo selections to follow others.
@li fix: conf_mtca_evm_300 increased with 16 numRearInp.
@li Adding more selection options for trigEvt source
@li Adding more selection options for DBussource
@li Adding more selection options for Pps source
@li docs: mrf-firmware and Readthedocs references added.
@li fix: lutX changed to lut_ to avoid the collisions.
@li fix: Correct access out of bonds on buffer used by flash functions
@li Adding the hardware Mxc reset function for each input
@li ci: Linux build with kernel 6.x added.
@li fix evr out db desc
@li update .gitignore
@li use vm_flags_set() on kernels 6.3+
@li Adding db substitution files for the vme-300 card
@li Added the feature of setting a delay target for an EVM
@li Add the FLASH IO for the vme300 boards
@li Adding more options of time stamp generator
@li Added more options to the EVR event clock mode
@li removing duplicated DESC fields in evrbase.db
@li Adding AcTrig source select options for external references
@li Fixing the macros AcTrigMap_EvtMASK and AcTrigMap_EvtSHIFT
@li Fixing the macros in evrRegMap.h
@li Adding the PLL Bandwidth adjustments for EVM and EVR, all changes combined
@li Add configurations for VME-EVR300 card
@li Adding configurations for EVM-VME-300 card and make it compilable with vxWorks
@li fix: version fetched from only digital git tags

@subsection v261 2.6.1 (Apr 2024)

@li add evgMrmShared depends also on evrApp because of internval EVRs
@li feat: Expanded timestamp error with the EVR model.
@li fix: Better softclock handling.
@li fix: Use softclock if the registry empty for EVRU/D.
@li fix: fix db name change 1ppsInp to PpsInp change 1hz to Pps of 300 series change PpsInpCONT to PpsInp1 to keep naming convention
@li fix: upgrade GH actions version to remove warning
@li fix: fix mtca 300 db * fix & add more 1pps source * fix FracSyn db limit * fix evrcml.db typo * fix DC status PDVLD bits meaning * change record TimeSrc-Sel (evrSoftEvt.template) to SoftTimeSrc-Sel to avoid duplication of record name * add prescaler to embedded EVR * other small fix
@li fix: fix MTCA-EVR-300u substitutions file
@li docs: CODEOWNERS update.
@li fix: Fix inclusion of evrSupportBase7.dbd
@li fix: The same variable declared twice.
@li fix: linux-build limited to kernel v5.
@li feat: Include actions to release deb and rpm dkms packages
@li fix: Fix mrf.spec.in for RPM package
@li fix: forces deb package to be compatible with older debian versions
@li fix: noarch for the dkms-rpm package.
@li Fixing PCIe EVR OPI
@li correct issues and change directory architecture
@li convert opi to bob with new naming, harmonized with a color.def using standard name
@li feat: Packaging mrf dkms as a deb package
@li feat: Extension of Osml-FOut to handle more inputs.
@li feat: More EVG inputs added for the Dbus control.
@li fix: RPM package license updated to the driver one.
@li refactor: Package DRV_NAME changed to KMOD_NAME to avoid confusions.
@li refactor: mrfioc2-dkms changed to mrf-dkms for the kmod package name.
@li fix: EVM as fanout by default.
@li fix: IRQPoller hardcoded initialization removed.
@li refactor: 4 to 2-space indents applied for .db and .templates.
@li refactor: 'info( ' changed to 'info(' for .db and .templates.
@li refactor: 'field( ' changed to 'field(' for .db and .templates.
@li feat: ASG private added for the private PVs.


@subsection v260 2.6.0 (Feb 2024)

@li feat: Add featuring masks/enable #124
@li feat: add sofware and hardware mask/enable for sequencer
@li fix: PCIe-DC EVR does not have front panel inputs only universal ones
@li refactor: DKMS RPM
@li refactor: Making the build more generic.
@li refactor: MRF_GROUP changed DRV_GROUP==root.
@li refactor: pci_channel_io_perm_failure info to the log.
@li ci: The latest linux kernel build changed.
@li fix: Versioning and build performance.
@li refactor: dkms to dkms-rpm dir adjustment.
@li feat: RPM_NAME variable added to the RPM makefile.
@li refactor: Rpm package naming to module_name-dkms change.
@li refactor: DRV_RELEASE variable contains dkms.
@li refactor: Kernel driver control files updated.
@li fix: Fix README.md to pint to github pages
@li fix: updating pushdoc.sh
@li fix: Remove sourcefourge links from documentation
@li fix: Can't use upstream event - EVRU and EVM #121
@li fix: EVRU/D backward compatibility.
@li fix: mrfioc2 issue #123.
@li add functions set/reset and inputs in universal substitutions files
@li add functions set/reset and inputs in the EVR300U
@li refactor: Introduction of .uv.substitutions.
@li fix: Correct the setFrequency command (#117)
@li feat: Macros controlling the EVRU/D irqpoller.
@li docs: Update of the release versioning.
@li fix: Missing pulsers 16 to 23 added as the setpoints.
@li fix: Src2 maps Src1 following 1:1.
@li feat: SFTSEN=# enables software timestamp.
@li fix: EVRU/D use the irqpoller until the isr is implemented.
@li feat: Pulser trigger with DBus bits and prescalers (#61)
@li Add DBus bit trig for pulsers
@li Add prescaler trigger for pulsers
@li feat: Better handling of GTX within formFactor_mTCA (#110)
@li Minor changes
@li refactor: std::ostringstream removed from drvem.
@li refactor: Existing inline string builder used instead of std::ostringstream.
@li fix: EVRMRM::Config adjusted for PCIe EVR and MTCA IFB.
@li feat: Printing of the full EVR config added.
@li fix: std::to_string is not suported within RTEMs.
@li feat: GTX output offset [FPUniv] control added.
@li fix: Adjustment of drvemSetup to support the GTX refactoring.
@li refactor: Better handling of GTX within formFactor_mTCA.
@li Changes for UTAG compatibility with base 7.0.5 and earlier
@li feat: lspci update for MTCA devices.
@li refactor: pollirq.cpp removed from the compilation.
@li Add support for UNIV and RF mTCA EVR models #63
@li Adopting the new naming convetion for the new substitutions files
@li Mapping MTCA EVR special outputs as per MRF manual
@li fix: mtca_evr_300ifb setup adjusted to its capabilities.
@li fix: Default MTCA EVR and formFactor_mTCA sub mapping.
@li fix: Added $(s=:) macro for the : separator.
@li Add evr-mtca-300u substitution file
@li Add support for mTCA RF EVR
@li Add suport for mTCA UNIV EVR
@li refactor: Reorganizing of code duplicates within devEvrEvent*
@li Changing subrelease variable to unsigned

@subsection v250 2.5.0 (Dec 2023)

@li feat: Timestaming with UTAG from jerzyjamroz/utag-rc
@li refactor: Risk of using a dummy epicsUTag minimilised.
@li perf: process_event removed from devEvrEventUtag.cpp.
@li feat: Warning added which informs if epicsUTag is supported or not.
@li fix: Default VAL=0 for $(EN)-SP added.
@li fix: evreventutag.db included in the module Makefile.
@li fix: $(s=:) was used for the private -ASub_.
@li docs: Copyright added.
@li refactor: EvrEventUtag clean up.
@li refactor: drvem.cpp recovered to its original form.
@li refactor: devEvrEvent.cpp applied to the original form.
@li fix: event added to the EvrEventUtag handling.
@li perf: evreventutag.db handling enhanced.
@li perf: Reduction of UTag operations to devEvrEvent only.
@li test: UTag tests.
@li fix: epicsUTag always 64 bit.
@li docs: File header adjusted accordingly.
@li perf: getTimeStamp optimized to basically one function.
@li fix: UTag value is arbitrary based, not for s&r.
@li fix: Minimal adjustment to follow the master branch.
@li fix: convertTS does not need the template anymore.
@li perf: Removal of the epicsTimeStampUTag.
@li fix: Epics base 7 or higher limitations added.
@li Create release candidate for UTAG developments
@li build: Removal of the header file installation.
@li fix: DB in Makefile follows refactoring now for evr-mtca-300u.
@li refactor: evr-300 changed to evr-mtca-300u.
@li fix: Normalize the Time-I record timestamp to 1s.


@subsection v240 2.4.0 (Nov 2023)

@li feat: ErrLog added for "out of bonds" TS scenario.
@li fix: BASE > 3.14 expressed as ifdef BASE_3_15.
@li fix: BASE_3_14 var used instead of the complex evaluation.
@li fix: TCLKA and TCLKB and EVRU/EVRD clocks.
@li feat: MTCA EVR FPUNIV, backplane CLKA/B handling added.
@li fix: Link speed increased to support faster SFP modules.
@li fix: TX, RX power alarms extended to support higher powers.
@li Display softseq errors on GUI
@li feat: version.h added to support external compilations.
@li feat: CommitHash pv added for EVR.
@li feat: CommitHash pv added for EVG.
@li fix: errlogPrintf messages corrected.
@li Add pulser software set/reset
@li Fix RFDIV macro
@li Add prescaler polarity selection
@li fix: More universal handling of the clock values for widths and delays.
@li fix: Better handling of the clocks.
@li Merge pull request #91 from jerzyjamroz/evgTrigEvt.db.patch
@li refactor: Order correction- Cont(assumes 0),Cont1,Cont2.
@li refactor: TrigSrc$(s=:)Cont changed to TrigSrc1.
@li feat: Univ Sources added up to 15.
@li feat: More outputs up to UNIV15 + inputs up to FPUV19.
@li feat: Univ and RB outputs + TB inputs added.
@li fix: OutFPUV0 does not support TCLKA.
@li feat: Naming agnostic EVR 300 added.
@li feat: evm-mtca-300 - naming agnostic substitutions.
@li Merge pull request #84 from jerzyjamroz/mv-SYSD-PP
@li fix: $(PP=$(SYS){$(D)-)EvtClk$(s=}) introduced.
@li fix: $(SYS){$(D)} changed to $(P) - not used in subs.
@li fix: $(SYS){$(D)} changed to $(PP=$(SYS){$(D)}).
@li Merge pull request #81 from jerzyjamroz/rm-colon
@li refactor: $(S=:) changed to $(s=:) which is less common.
@li fix: Complex prefix changed to $(P) and the special separator : changed to makro $(S=:).
@li fix: Special separator : changed to makro $(S=:).})

@subsection v231 2.3.1 (Sep 2023)

@li Add PCIe error handling to Linux kernel module (Hinko Kocevar)
@li Add DKMS and RPM configs for Linux kernel module source (Michael Abbott)
@li Fix input count on PCIe-EVR-300 and MTCA-EVR-300 (Jerzy Jamroz)
@li Added support for DLY universal modules (Javier Cereijo Garcia)
@li Add buffered timestamp capture
@li Support Pulser enable gating, prescalar phase/offset features of newer EVR firmware (Luka Krmpotic)
@li Warn about unknown INP/OUT link options.

@subsection v230 2.3.0 (Apr 2020)

@li Add support for mTCA-EVM-300
@li Add safety checks to flash firmware update process
@li Allow iocsh redirection to file (eg. of dbior output)
@li Bug fixes and DB enhancements (Luka Krmpotić)
@li Set SHRLIB_VERSION

@subsection v220 2.2.0 (Oct. 2017)

@subsubsection v220_compat Incompatible changes

@li Requires devlib2 >=2.9
@li Some software sequencer control records have changed to accommodate sequencer support in the 300DC series EVRs.  Updated cs-studio/BOY .opi files.
@li Remove support for sequencer "Automatic" (continuous) trigger mode, also Pause and Abort actions.
@li EDM files removed due to lack of maintenance.

@subsubsection v220_add Additions

@li Add support for mTCA-EVR-300 and PCIe-EVR-300DC.  Requires firmware >=207.  Tested with firmware >=207.6.
@li Add support for PXI-EVG-220.  Support for PXI-EVG-230 was added in 2.1.0, but not documented.
@li Add driver for FRIB developed mTCA EVR
@li mrmEvrSetupPCI() accepts "slot=#" in addition to PCI geographic address
@li linux: kernel module exports PCI ID table to enable automatic module loading
@li Firmware upgrade via PCIe flash access for mTCA-EVR-300 and PCIe-EVR-300DC.  See EVR manual for details.
@li EVG add 1 PPS source option "Sys Clk" to simulate external HW clock using system clock.
@li EVR add 1 PPS and timestamp source simulation.

@subsection v210 2.1.0 (Oct. 2016)

@subsubsection v210change Incompatible changes

@li EVG Enable control removed.  Now always enabled.
@li Incompatible changes to data buffer TX/RX APIs

@subsubsection v210add Addtions

@li CS-Studio BOY .opi files for EVG
@li Support for EVG on little endian targets (eg. linux x86)
@li Support for additional devices: cPCI-EVG-300, cPCI-EVR-300, and PCIe-EVR-300
@li Add mrmEvgSoftTime() for software only time stamp distribution.
@li Support for Fine delay for UNIV I/O daughter cards.
@li Expose EVG Start of Sequence IRQ as with End of Sequence w/ a counter record.
@li Add *Label-I charactor waveform to support long form description strings.
@li Support named Events with Base >= 3.15 (SCAN=Event)
@li Add global flag mrmGTIFEnable to allow generalTime current time provider to be disabled.

@subsection v204 2.0.4 (May 2015)

@subsubsection v204not Notices

@li Requires devLib >= 2.6
@li Builds against EPICS Base 3.15.1
@li Workaround to support UIO in Linux kernel >=3.12 (debian kernel >=3.2.0)
@li Default mrmEvrFIFOPeriod doubled to 1ms

@subsubsection v204bug Bug fixes

@li Fix incorrect firmware version warning message
@li Linux only.  Rework interrupt handling to fully eliminate race in IRQ acknowledge.
    Requires new kernel module.

@subsubsection v204feat Features

@li Added sequence masker aSub (bit mask to replace event codes with zero)
@li Add records to show statistics of interrupt and event FIFO processing
@li Soft sequence loading stops at end of sequence event (0x7f).
    Further delays/events are ignored.

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
  please contact me (mdavidsaver@gmail.com) or it will likely be removed
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

@author Michael Davidsaver <mdavidsaver@gmail.com>

@author Jayesh Shah <jshah@bnl.gov>

@author Eric Björklund <bjorklund@lanl.gov>

@author Slejko Tom <tom.slejko@psi.ch>

@author Helge Brands <helge.brands@psi.ch>

@author Saso Skube <saso.skube@psi.ch>

@author Jure Krasna <jure.krasna@cosylab.com>

@author Dirk Zimoch <dirk.zimoch@psi.ch>

@author Anton Derbenev <aderbenev@bnl.gov>

@author Luka Krmpotić <luka.krmpotic@cosylab.com>

@author Jerzy Jamróz <jerzy.jamroz@gmail.com>

@author Alexis Gaget <alexis.gaget@cea.fr>

@author Di Wang <sdcswd@gmail.com>

@author Gabriel Desmarchelier <gabriel.desmarchelier@cea.fr>

@author Gabriel Fedel <gabriel.fedel@ess.eu>

@author Hinko Kocevar <hinko.kocevar@ess.eu>

@author Javier Cereijo <javier.cereijogarcia@ess.eu>

@author João Paulo Martins <joaopaulosm@gmail.com>

@author Minijackson <minijackson@riseup.net>

@author Ran Hong <hongran@uw.edu>

@author Zan Pratnemer <zan.pratnemer@cosylab.com>

@author Žiga Oven <ziga.oven@cosylab.com>

*/

#endif // MAINPAGE_H
