#ifndef EVG_SEQUENCE_H
#define EVG_SEQUENCE_H

#include <vector>
#include <string>

#include <inttypes.h>
#include <epicsTypes.h>
#include <epicsMutex.h>
#include <dbAccess.h>
#include <dbScan.h>
#include <stdint.h>

class evgMrm;
class evgSeqRam;
class evgSeqRamMgr;

#if __STDC_VERSION__ < 199901L
#include <inttypes.h>
typedef int64_t         epicsInt64;
typedef uint64_t        epicsUInt64;
#endif

enum TimestampInpMode {
    EGU = 0,
    TICKS
};

enum SeqRunMode {
    Normal = 0,
    Auto,
    Single
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
    AC = 16,
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

    epicsStatus setTimestampInpMode(TimestampInpMode);
    TimestampInpMode getTimestampInpMode();

    epicsStatus setTimestamp(epicsUInt64*, epicsUInt32);
    epicsStatus setEventCode(epicsUInt8*, epicsUInt32);
    epicsStatus setTrigSrc(SeqTrigSrc);
    epicsStatus setRunMode(SeqRunMode);    

    std::vector<epicsUInt64> getTimestampCt();
    std::vector<epicsUInt8> getEventCodeCt();
    SeqTrigSrc getTrigSrcCt();
    SeqRunMode getRunModeCt();

    epicsStatus setSeqRam(evgSeqRam*);
    evgSeqRam* getSeqRam();

    bool isLoaded();
    bool isEnabled();
    bool isCommited();

    epicsStatus load();
    epicsStatus unload();
    epicsStatus commit();
    epicsStatus enable();
    epicsStatus disable();
    epicsStatus abort(bool);
    epicsStatus pause();
    epicsStatus sync();
    epicsStatus isRunning();
    epicsStatus commitSoftSeq();

    IOSCANPVT                  ioscanpvt;
    IOSCANPVT                  ioScanPvtErr;
    epicsMutex                 m_lock;

private:
    const epicsUInt32          m_id;
    evgMrm*                    m_owner;
    volatile epicsUInt8* const m_pReg;
    std::string                m_desc;
    std::string                m_err;

    TimestampInpMode           m_timestampInpMode;

    std::vector<epicsUInt64>   m_timestamp;
    std::vector<epicsUInt8>    m_eventCode;
    SeqTrigSrc                 m_trigSrc;
    SeqRunMode                 m_runMode;   

    std::vector<epicsUInt64>   m_timestampCt;
    std::vector<epicsUInt8>    m_eventCodeCt;
    SeqTrigSrc                 m_trigSrcCt;
    SeqRunMode                 m_runModeCt;

    evgSeqRam*                 m_seqRam;
    evgSeqRamMgr*              m_seqRamMgr; 

    bool                       m_isEnabled;
    bool                       m_isCommited;
};

#endif //EVG_SEQUENCE_H

