#include "evgInput.h"

#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>
 
#include "evgRegMap.h"
std::map<std::string, epicsUInt32> InpStrToEnum;

evgInput::evgInput(const epicsUInt32 id, const InputType type,
volatile epicsUInt8* const pInMap):
m_pInMap(pInMap) {
	
	InpStrToEnum["None"] = None_Input;
	InpStrToEnum["FP_Input"] = FP_Input;
	InpStrToEnum["Univ_Input"] = Univ_Input;
	InpStrToEnum["TB_Input"] = TB_Input;

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
evgInput::setInpDbusMap(epicsUInt16 dbus, bool ena) {
	if(dbus > 7)
		throw std::runtime_error("EVG DBUS ID out of range.");

	epicsUInt32	mask = 0x10000 << dbus;

	//Read-Modify-Write
	epicsUInt32 map = nat_ioread32(m_pInMap);

	if(ena)
		map = map | mask;
	else
		map = map & ~mask;

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
evgInput::setInpTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
	if(trigEvt > 7)
		throw std::runtime_error("Trig Event ID out of range.");

	epicsUInt32	mask = 1 << trigEvt;
	//Read-Modify-Write
	epicsUInt32 map = nat_ioread32(m_pInMap);

	if(ena)
		map = map | mask;
	else
		map = map & ~mask;

	nat_iowrite32(m_pInMap, map);
	
	return OK;
}



