#ifndef EVG_SEQUENCE_H
#define EVG_SEQUENCE_H

#include <vector>
#include <string>

#include <epicsTypes.h>
#include <epicsMutex.h>
#include <dbAccess.h>
#include <dbScan.h>

class evgMrm;
class evgSeqRam;
class evgSeqRamMgr;

enum SeqRunMode {
	Single = 0,
	Auto,
	Normal
};

enum SeqTrigSrc {
	Mxc0 = 0,
	Mxc1 = 1,
	Mxc2 = 2,
	Mxc3 = 3,
	Mxc4 = 4,
	Mxc5 = 5,
	Mxc6 = 6,
	Mxc7 = 7,
	AC	 = 16,
	SW_Ram0 = 17,
	SW_Ram1 = 18
};

#define IrqStop(id) irqStop##id

class evgSoftSeq {
public:
	evgSoftSeq(const epicsUInt32, evgMrm* const);
	~evgSoftSeq();

	const epicsUInt32 getId() const;	

	epicsStatus setDescription(const char*);
	const char* getDescription();

	epicsStatus setErr(std::string);
	std::string getErr();

	epicsStatus setEventCode(epicsUInt8*, epicsUInt32);
	epicsStatus setTimeStampTick(epicsUInt32*, epicsUInt32);
	epicsStatus setTimeStampSec(epicsFloat64*, epicsUInt32);
	epicsStatus setTrigSrc(SeqTrigSrc);
	epicsStatus setRunMode(SeqRunMode);

	void ctEventCode();
	void ctTimeStamp();
	void ctTrigSrc();
	void ctRunMode();

	std::vector<epicsUInt8> getEventCodeCt();
	std::vector<epicsUInt32> getTimeStampCt();
	SeqTrigSrc getTrigSrcCt();
	SeqRunMode getRunModeCt();

	epicsStatus setSeqRam(evgSeqRam*);
	evgSeqRam* getSeqRam();

	bool isLoaded();
	bool isEnabled();
	bool isCommited(); 

	epicsStatus load	();
	epicsStatus unload	();
	epicsStatus commit	();
	epicsStatus enable	();
	epicsStatus disable	();
	epicsStatus halt	(bool);
	epicsStatus sync	();
	epicsStatus isRunning();
	epicsStatus inspectSoftSeq();

	IOSCANPVT 					ioscanpvt;
	IOSCANPVT 					ioScanPvtErr;
	epicsMutex 					m_lock;

private:
	const epicsUInt32 			m_id;
	evgMrm* 					m_owner;
	volatile epicsUInt8* const  m_pReg;
	std::string 				m_desc;
	std::string 				m_err;

	std::vector<epicsUInt8> 	m_eventCode;
	std::vector<epicsUInt32>	m_timeStamp;
	SeqTrigSrc 					m_trigSrc;
	SeqRunMode 					m_runMode;
	
	std::vector<epicsUInt8> 	m_eventCodeCt;
	std::vector<epicsUInt32>	m_timeStampCt;
	SeqTrigSrc					m_trigSrcCt;
	SeqRunMode 					m_runModeCt;
	
	evgSeqRam*  				m_seqRam;
	evgSeqRamMgr*				m_seqRamMgr; 
	epicsUInt32					m_ecSize;
	epicsUInt32					m_tsSize;
	
	bool						m_isEnabled;
	bool						m_isCommited;
};

#endif //EVG_SEQUENCE_H

