#PSI Evr health monitoring database

this database should be present with every EVR installed on SF as it allows fro proper EVR health monitoring. 

The database can be included in the following way (startup script commands, change macros to correct values): 

		dbLoadRecords "evr-health.template" "SYS=CSL-IFC1,EVR=EVR0"

The database is included in standard PSI EVR startup script snippet. 


##EVR health monitoring panel: 

to start the panel run: 

		caqtdm -macro "SYS=CSL-IFC1,EVR=EVR0" ui/evr-health.ui

