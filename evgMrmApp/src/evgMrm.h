#ifndef EVGMRM_H
#define EVGMRM_H

#include <vector>
#include <map>
#include <string>

#include  <epicsTypes.h>
  
#include "evgMxc.h"
#include "evgTrigEvt.h"
#include "evgDbus.h"
#include "evgFPio.h"

#define EVG_CLOCK_SRC_INTERNAL  0       // Event clock is generated internally
#define EVG_CLOCK_SRC_RF        1       // Event clock is generated from the RF input port


class evgMrm {

public:
	/** EVG	**/	
	evgMrm(const epicsUInt32 CardNum, const volatile epicsUInt8* pReg);
	~evgMrm();

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

	/**	List Access	**/
	evgMxc* getMuxCounter(epicsUInt32); 
	evgTrigEvt* getTrigEvt(epicsUInt32);
	evgDbus* getDbus(epicsUInt32 dbusBit);
	evgFPio* getFPio(epicsUInt32, std::string);
	
private:
	const epicsUInt32            	id;         // Logical card number for this card
    const volatile epicsUInt8*		pReg;      	// CPU Address for accessing the card's register map

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
};

#endif //EVGMRM_H