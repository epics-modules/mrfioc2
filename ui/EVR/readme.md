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
- __evr_master.ui__ is the main screen of the EVR
- __evr_base-link-status.ui__ provides information about the link
- __evr_info.ui__ provides basic info about the connected EVR
- __evr_input.ui__ controls an EVR input
- __evr_map-pulser.ui__ controls mapping of pulser functions to an event
- __evr_map-soft.ui__ shows a number of times the event was triggered
- __evr_map-special.ui__ shows the mapping of special function to an event
- __evr_out-details-min.ui__ a compact view of the output with settable output source.
- __evr_out-details.ui__ similar to _evr_out-details-min.ui_, but including user description
- __evr_out-helper.ui__ a helper widget used to display the right output source in _evr_out_*\_ widgets
- __evr_out-helper.ui__ a helper widget used to display the led next to the right output source in _evr_out_*\_ widgets
- __evr_out-small.ui__ only enabled status and selected output source display of the EVR output
- __evr_prescaler.ui__ information and settings for a prescaler
- __evr_pulser.ui__ information and settings for a pulser
- __evr_sfp-min.ui__ basic info about the connected SFP
- __evr_sfp.ui__ extended info about the connected SFP
