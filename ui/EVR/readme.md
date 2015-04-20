# mrfioc2 EVR GUI
## Quick start
run 

    ./startUI.sh -s <system name>
from this folder.

Usage: $0 [options]

Options:
- -s <system name>     The system/project name
- -r <EVR name>        Event Receiver name (default: EVR0)
- -h                   This help

## Files description
- __G_EVR_VME_master.ui__ is the main screen of the EVR
- __G_EVR_VME_base-link-status.ui__ provides information about the link
- __G_EVR_VME_info.ui__ provides basic info about the connected EVR
- __G_EVR_VME_input.ui__ controls an EVR input
- __G_EVR_VME_map-pulser.ui__ controls mapping of pulser functions to an event
- __G_EVR_VME_map-soft.ui__ shows a number of times the event was triggered
- __G_EVR_VME_map-special.ui__ shows the mapping of special function to an event
- __G_EVR_VME_out-details-min.ui__ a compact view of the output with settable output source.
- __G_EVR_VME_out-details.ui__ similar to _G_EVR_VME_out-details-min.ui_, but including user description
- __G_EVR_VME_out-helper.ui__ a helper widget used to display the right output source in _G_EVR_VME_out_*\_ widgets
- __G_EVR_VME_out-helper.ui__ a helper widget used to display the led next to the right output source in _G_EVR_VME_out_*\_ widgets
- __G_EVR_VME_out-small.ui__ only enabled status and selected output source display of the EVR output
- __G_EVR_VME_prescaler.ui__ information and settings for a prescaler
- __G_EVR_VME_pulser.ui__ information and settings for a pulser
- __G_EVR_VME_sfp-min.ui__ basic info about the connected SFP
- __G_EVR_VME_sfp.ui__ extended info about the connected SFP
