#include "evgFPio.h"

#include <iostream>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>
 
#include "evgRegMap.h"

evgFPio::evgFPio(const IOtype type, const epicsUInt32 id, volatile epicsUInt8* const pReg ):
m_type(type),
m_id(id),
m_pReg(pReg) {
}

epicsStatus
evgFPio::setIOMap(epicsUInt32 map) {

	epicsStatus status;

	switch(m_type) {
    	
		case(FP_Input):
			if(m_id < 0 || m_id >= evgNumFpInp) {
				errlogPrintf("ERROR: Front panel input number out of range.\n");
				status = ERROR;
				break;
			}
	
			WRITE32(m_pReg, FPInMap(m_id), map);
			status = OK;
			break;

		case(FP_Output):
			if(m_id < 0 || m_id >= evgNumFpOut) {
				errlogPrintf("ERROR: Front panel output number out of range.\n");
				status = ERROR;
				break;
			}

			WRITE16(m_pReg, FPOutMap(m_id), map);
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


