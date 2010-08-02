#include "evgInput.h"

#include <iostream>
#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>
 
#include "evgRegMap.h"

evgInput::evgInput(const epicsUInt32 id, volatile epicsUInt8* const pReg, const InputType type):
m_id(id),
m_pReg(pReg),
m_type(type) {

	switch(m_type) {
		case(FP_Input):
			if(m_id >= evgNumFpInp)
				throw std::runtime_error("Front panel input ID out of range");	
			break;	

		case(Univ_Input):
			if(m_id >= evgNumUnivInp)
				throw std::runtime_error("EVG Universal input ID out of range");
			break;

		default:
				throw std::runtime_error("EVG Wrong I/O type");
			break;
	}
}

evgInput::~evgInput() {
}

epicsStatus
evgInput::setInpDbusMap(epicsUInt32 dbusMap) {
	epicsStatus ret = OK;
	epicsUInt32 map = 0;
	
	switch(m_type) {
		case(FP_Input):
			//Read-Modify-Write
			map = READ32(m_pReg, FPInMap(m_id));

			map = map & 0x0000ffff;
			map = map | (dbusMap << 16);

			WRITE32(m_pReg, FPInMap(m_id), map);

			ret = OK;
			break;

		case(Univ_Input):
			//Read-Modify-Write
			map = READ32(m_pReg, UnivInMap(m_id));

			map = map & 0x0000ffff;
			map = map | (dbusMap << 16);

			WRITE32(m_pReg, UnivInMap(m_id), map);
			
			ret = OK;
			break;
	}
	
	return ret;
}

epicsStatus
evgInput::setInpTrigEvtMap(epicsUInt32 trigEvtMap) {
	epicsStatus ret = OK;
	epicsUInt32 map = 0;

	switch(m_type) {
		case(FP_Input):
			//Read-Modify-Write
			map = READ32(m_pReg, FPInMap(m_id));

			map = map & 0xffff0000;
			map = map | trigEvtMap;

			WRITE32(m_pReg, FPInMap(m_id), map);

			ret = OK;
			break;

		case(Univ_Input):
			//Read-Modify-Write
			map = READ32(m_pReg, UnivInMap(m_id));

			map = map & 0xffff0000;
			map = map | trigEvtMap;

			WRITE32(m_pReg, UnivInMap(m_id), map);

			ret = OK;
			break;
	}
	
	return ret;
}

epicsStatus
evgInput::enaExtIrq(bool ena) {
	switch(m_type) {
		case(FP_Input):
			if(ena)
				BITSET32(m_pReg, FPInMap(m_id), EVG_EXT_INP_IRQ_ENA);
			else
				BITCLR32(m_pReg, FPInMap(m_id), EVG_EXT_INP_IRQ_ENA);
			break;

		case(Univ_Input):
			if(ena)
				BITSET32(m_pReg, FPInMap(m_id), EVG_EXT_INP_IRQ_ENA);
			else
				BITCLR32(m_pReg, FPInMap(m_id), EVG_EXT_INP_IRQ_ENA);
			break;
	}

	return OK;
}