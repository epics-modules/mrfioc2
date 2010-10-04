#include "evgInput.h"

#include <iostream>
#include <string>
#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>
 
#include "evgRegMap.h"

evgInput::evgInput(const epicsUInt32 id, const InputType type,
volatile epicsUInt8* const pInMap):
m_pInMap(pInMap) {

	switch(type) {
		case(FP_Input):
			if(id >= evgNumFpInp)
				throw std::runtime_error("Front panel input ID out of range");	
			break;

		case(Univ_Input):
			if(id >= evgNumUnivInp)
				throw std::runtime_error("EVG Universal input ID out of range");
			break;

		case(TB_Input):
			if(id >= evgNumTbInp)
				throw std::runtime_error("EVG TB input ID out of range");
			break;

		default:
 			throw std::runtime_error("Wrong EVG Input type");
	}
}

evgInput::~evgInput() {
}

epicsStatus
evgInput::enaExtIrq(bool ena) {
	if(ena)
		nat_iowrite32(m_pInMap, nat_ioread32(m_pInMap) |
 								(epicsUInt32)EVG_EXT_INP_IRQ_ENA);
	else
		nat_iowrite32(m_pInMap, nat_ioread32(m_pInMap) &
							 	(epicsUInt32)~(EVG_EXT_INP_IRQ_ENA));

	return OK;
}

epicsStatus
evgInput::setInpDbusMap(epicsUInt32 dbusMap) {
	if(dbusMap > 255)
		throw std::runtime_error("Dbus Map out of range.");

	//Read-Modify-Write
	epicsUInt32 map = nat_ioread32(m_pInMap);

	map = map & 0xff00ffff;
	map = map | (dbusMap << 16);

	nat_iowrite32(m_pInMap, map);

	return OK;
}

epicsStatus
evgInput::setInpSeqTrigMap(epicsUInt32 seqTrigMap) {
	if(seqTrigMap > 3)
		throw std::runtime_error("Seq Trig Map out of range.");

	//Read-Modify-Write
	epicsUInt32 map = nat_ioread32(m_pInMap);

	map = map & 0xffff00ff;
	map = map | (seqTrigMap << 8);

	nat_iowrite32(m_pInMap, map);
	
	return OK;
}

epicsStatus
evgInput::setInpTrigEvtMap(epicsUInt32 trigEvtMap) {
	if(trigEvtMap > 255)
		throw std::runtime_error("Trig Evt Map out of range.");

	//Read-Modify-Write
	epicsUInt32 map = nat_ioread32(m_pInMap);

	map = map & 0xffffff00;
	map = map | trigEvtMap;

	nat_iowrite32(m_pInMap, map);
	
	return OK;
}


