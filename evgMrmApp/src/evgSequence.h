#ifndef EVG_SEQUENCE_H
#define EVG_SEQUENCE_H

#include <vector>
#include <string>

#include <epicsTypes.h>

enum RunMode {
	single = 0,
	recycle,
	recycleOnTrigger
};

enum TrigSrc {
	mxc0 = 0,
	mxc1 = 1,
	mxc2 = 2,
	mxc3 = 3,
	mxc4 = 4,
	mxc5 = 5,
	mxc6 = 6,
	mxc7 = 7,
	AC   = 16,
	ram0 = 17,
	ram1 = 18
};

class evgSequence {
public:
	evgSequence(const epicsUInt32);
	~evgSequence();

	const epicsUInt32 getId() const;	

	//epicsStatus setDescription(const epicsUInt8*);
	//const epicsUInt8* getDescription();

	epicsStatus setDescription(const char*);
	const char* getDescription();

	epicsStatus setEventCode(epicsUInt8*, epicsUInt32);
	epicsUInt8* getEventCodeA();
	std::vector<epicsUInt8> getEventCodeV();
	
	epicsStatus setTimeStamp(epicsUInt32*, epicsUInt32);
	epicsUInt32* getTimeStampA();
	std::vector<epicsUInt32> getTimeStampV();

	epicsStatus setTrigSrc(TrigSrc);
	TrigSrc getTrigSrc();

	epicsStatus setRunMode(RunMode);
	RunMode getRunMode();

private:
	const epicsUInt32 			m_id;
	std::string 				m_desc;
	std::vector<epicsUInt8> 	m_eventCodeV;
	std::vector<epicsUInt32>	m_timeStampV;
	epicsUInt8* 				m_eventCodeA;
	epicsUInt32* 				m_timeStampA;
	TrigSrc 					m_trigSrc;
	RunMode 					m_runMode;	
};

#endif //EVG_SEQUENCE_H