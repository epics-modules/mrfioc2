#ifndef EVG_SEQUENCE_H
#define EVG_SEQUENCE_H

#include <vector>
#include <string>

#include <epicsTypes.h>
#include <epicsMutex.h>
#include <dbAccess.h>

class evgMrm;
class evgSeqRam;
class evgSeqRamMgr;

enum SeqRunMode {
	single = 0,
	automatic,
	normal
};

class evgSoftSeq {
public:
	evgSoftSeq(const epicsUInt32, evgMrm* const);
	~evgSoftSeq();

	const epicsUInt32 getId() const;	

	epicsStatus setDescription(const char*);
	const char* getDescription();

	epicsStatus setEventCode(epicsUInt8*, epicsUInt32);
	std::vector<epicsUInt8> getEventCode();
	
	epicsStatus setTimeStampTick(epicsUInt32*, epicsUInt32);
	epicsStatus setTimeStampSec(epicsFloat64*, epicsUInt32);
	std::vector<epicsUInt32> getTimeStamp();

	epicsStatus setTrigSrc(epicsUInt32);
	epicsUInt32 getTrigSrc();

	epicsStatus setRunMode(SeqRunMode);
	SeqRunMode getRunMode();

	epicsStatus setSeqRam(evgSeqRam*);
	evgSeqRam* getSeqRam();

	epicsStatus load	();
	epicsStatus unload	(dbCommon *);
	epicsStatus commit	(dbCommon *);
	epicsStatus enable	();
	epicsStatus disable	();
	epicsStatus halt	();

	epicsMutex* getLock();

private:
	const epicsUInt32 			m_id;
	evgMrm* 					m_owner;
	volatile epicsUInt8* const  m_pReg;
	std::string 				m_desc;
	std::vector<epicsUInt8> 	m_eventCode;
	std::vector<epicsUInt32>	m_timeStamp;
	epicsUInt32 				m_trigSrc;
	SeqRunMode 					m_runMode;
	evgSeqRam*  				m_seqRam;
	evgSeqRamMgr*				m_seqRamMgr; 
	epicsMutex* 				m_lock;
};

#endif //EVG_SEQUENCE_H