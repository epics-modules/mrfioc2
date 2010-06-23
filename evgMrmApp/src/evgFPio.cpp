#include "evgFPio.h"

#include <iostream>
#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>
 
#include "evgRegMap.h"

evgFPio::evgFPio(const IOtype type, const epicsUInt32 id, volatile epicsUInt8* const pReg ):
m_type(type),
m_id(id),
m_pReg(pReg) {

	switch(m_type) {
    	
		case(FP_Input):
			if(m_id >= evgNumFpInp)
				throw std::runtime_error("Front panel input ID out of range");	
			break;

		case(FP_Output):
			if(m_id >= evgNumFpOut)
				throw std::runtime_error("EVG Front panel output ID out of range");
			break;

		case(Univ_Input):
			if(m_id >= evgNumUnivInp)
				throw std::runtime_error("EVG Universal input ID out of range");
			break;

		case(Univ_Output):
			if(m_id >= evgNumUnivOut)
				throw std::runtime_error("EVG Universal output ID out of range");
			break;

		default:
				throw std::runtime_error("EVG Wrong I/O type");
			break;
	}
}

epicsStatus
evgFPio::setIOMap(epicsUInt32 map) {

	epicsStatus ret = OK;

	switch(m_type) {
    	
		case(FP_Input):
			WRITE32(m_pReg, UnivInMap(m_id), map);
			ret = OK;
			break;

		case(FP_Output):
			if(map < 32 || map > 63 ||  (map < 62 && map > 39) ) {
				errlogPrintf("ERROR: Front panel output %d map out of range\n", map);
				ret = ERROR;
				break;
			}

			WRITE16(m_pReg, FPOutMap(m_id), map);
			ret = OK;
			break;

		case(Univ_Input):
			WRITE32(m_pReg, UnivInMap(m_id), map);
			ret = OK;
			break;

		case(Univ_Output):
			if(map < 32 || map > 63 ||  (map < 62 && map > 39) ) {
				errlogPrintf("ERROR: Universal output %d map out of range\n", map);
				ret = ERROR;
				break;
			}

			WRITE16(m_pReg, UnivOutMap(m_id), map);
			ret = OK;
			break;
	}
	
	return ret;
		
}


