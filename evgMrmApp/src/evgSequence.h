#ifndef EVG_SEQUENCE_H
#define EVG_SEQUENCE_H

#include <vector>
#include <string>

#include <epicsTypes.h>

enum SeqRunMode {
	single = 0,
	automatic,
	normal
};

class evgSeqRam;

class evgSequence {
public:
	evgSequence(const epicsUInt32);
	~evgSequence();

	const epicsUInt32 getId() const;	

	epicsStatus setDescription(const char*);
	const char* getDescription();

	epicsStatus setEventCode(epicsUInt8*, epicsUInt32);
	std::vector<epicsUInt8> getEventCode();
	
	epicsStatus setTimeStamp(epicsUInt32*, epicsUInt32);
	std::vector<epicsUInt32> getTimeStamp();

	epicsStatus setTrigSrc(epicsUInt32);
	epicsUInt32 getTrigSrc();

	epicsStatus setRunMode(SeqRunMode);
	SeqRunMode getRunMode();

	epicsStatus setSeqRam(evgSeqRam*);
	evgSeqRam* getSeqRam();

private:
	const epicsUInt32 			m_id;
	std::string 				m_desc;
	std::vector<epicsUInt8> 	m_eventCode;
	std::vector<epicsUInt32>	m_timeStamp;
	epicsUInt32 				m_trigSrc;
	SeqRunMode 					m_runMode;
	evgSeqRam*  				m_seqRam;	
};

#endif //EVG_SEQUENCE_H