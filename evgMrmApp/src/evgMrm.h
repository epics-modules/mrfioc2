#ifndef EVGMRM_H
#define EVGMRM_H

#include <vector>
#include <map>
#include <string>

#include  <epicsTypes.h>
#include <dbScan.h>
#include <callback.h>
  
#include "evgMxc.h"
#include "evgTrigEvt.h"
#include "evgDbus.h"
#include "evgFPio.h"
#include "evgSeqRamSup.h"

const epicsUInt16 evgClkSrcInternal = 0;       // Event clock is generated internally
const epicsUInt16 evgClkSrcRF 	    = 1;       // Event clock is generated from the RF input port

extern std::vector<dbCommon*>	recList;

class evgMrm {

public:
	/** EVG	**/	
	evgMrm(const epicsUInt32 CardNum, volatile epicsUInt8* const pReg);
	~evgMrm();

	epicsStatus enable(bool ena);

	static void isr(void*);

	/**	Event Clock Speed	**/
	epicsStatus setClockSpeed(epicsFloat64);
	epicsFloat64 getClockSpeed();

	/**	Event Clock Source	**/
	epicsStatus setClockSource(epicsUInt8);
	epicsUInt8 getClockSource();	

	/**	Software Event	**/
	epicsStatus softEvtEnable(bool);
 	bool softEvtEnabled();

	bool softEvtPend();
	
	epicsStatus setSoftEvtCode(epicsUInt8);
	epicsUInt8 getSoftEvtCode();

	/**	Access	function 	**/
	evgMxc* getMuxCounter(epicsUInt32); 
	evgTrigEvt* getTrigEvt(epicsUInt32);
	evgDbus* getDbus(epicsUInt32 dbusBit);
	evgFPio* getFPio(epicsUInt32, std::string);
	evgSeqRamSup* getSeqRamSup();

// 	IOSCANPVT 						irqStop0;
// 	IOSCANPVT 						irqStop1;
// 	IOSCANPVT 						irqStart0;
// 	IOSCANPVT 						irqStart1;
// 	IOSCANPVT 						irqDataBuf;
// 	IOSCANPVT						irqEvtFifoFull;
// 	IOSCANPVT						irqRxVio;

	const epicsUInt32            	id;         // Logical card number for this card
    volatile epicsUInt8* const		pReg;      	// CPU Address for accessing the card's register map

	CALLBACK callback;	

private:
    epicsFloat64          			ClkSpeed;	// In MHz
	epicsUInt32						ClkSrc;
	
	typedef std::vector<evgMxc*> 	MuxCounter_t;
  	MuxCounter_t muxCounter;

	typedef std::vector<evgTrigEvt*> TrigEvt_t;
  	TrigEvt_t trigEvt;

	typedef std::vector<evgDbus*> 	Dbus_t;
  	Dbus_t dbus;

	typedef std::map< std::pair<epicsUInt32, std::string >, evgFPio*> FPio_t;
	FPio_t FPio;

	evgSeqRamSup* 					seqRamSup;
};

#endif //EVGMRM_H