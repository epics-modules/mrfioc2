What is Available?
------------------

More infomation on the Micro Research hardware can be found on their
website http://www.mrf.fi/.

Documentation appears at [http://epics.sourceforge.net/mrfioc2](http://epics.sourceforge.net/mrfioc2)

The latest developments can be found in the 'mrfioc2' VCS repository.

[https://github.com/epics-modules/mrfioc2](https://github.com/epics-modules/mrfioc2)

Prerequisites
-------------

The required software is EPICS Base >= 3.14.8.2, devLib2 >= 2.6, and the MSI tool (included in Base >= 3.15.1).

Base: [http://www.aps.anl.gov/epics/base/R3-14/index.php](http://www.aps.anl.gov/epics/base/R3-14/index.php)

devLib2: [https://github.com/epics-modules/devlib2/](https://github.com/epics-modules/devlib2/)

MSI (Base <3.15 only): [http://www.aps.anl.gov/epics/extensions/msi/index.php](http://www.aps.anl.gov/epics/extensions/msi/index.php)

The Source
----------

VCS Checkout

```shell
git clone https://github.com/epics-modules/mrfioc2.git
```

Edit 'configure/CONFIG_SITE' and run "make".

<a href="https://travis-ci.org/epics-modules/mrfioc2"><img src="https://travis-ci.org/epics-modules/mrfioc2.svg">CI Build Status</img></a>
