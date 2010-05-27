#include "evgFPio.h"

#include <iostream>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>
 
#include "evgRegMap.h"

evgFPio::evgFPio(const IOtype type, const epicsUInt32 id, volatile epicsUInt8* const pReg ):
type(type),
id(id),
pReg(pReg) {
}

epicsStatus
evgFPio::setIOMap(epicsUInt32 map) {

	epicsStatus status;

	switch(type) {
    	
		case(FP_Input):
			if(id < 0 || id >= evgNumFpInp) {
				errlogPrintf("ERROR: Front panel input number out of range.\n");
				status = ERROR;
				break;
			}
	
			WRITE32(pReg, FPInMap(id), map);
			status = OK;
			break;

		case(FP_Output):
			if(id < 0 || id >= evgNumFpOut) {
				errlogPrintf("ERROR: Front panel output number out of range.\n");
				status = ERROR;
				break;
			}

			WRITE16(pReg, FPOutMap(id), map);
			status = OK;
			break;

		case(Univ_Output):
		case(Univ_Input):
		default:
			errlogPrintf("ERROR: Not Configued.\n");
			status = ERROR;
			break;
	}
	
	return status;
		
}


