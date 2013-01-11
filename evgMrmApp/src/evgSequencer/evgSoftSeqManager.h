#ifndef EVG_SEQ_MGR_H
#define EVG_SEQ_MGR_H

#include <map>

#include <epicsTypes.h>

#include "mrfCommon.h"
#include "evgSoftSeq.h"

class evgSoftSeqMgr {
    typedef std::map<epicsUInt32, evgSoftSeq*> m_softSeq_t;
public:
    evgSoftSeqMgr(evgMrm* const);
    evgSoftSeq* getSoftSeq(epicsUInt32);

    template<typename V>
    void visit(V& o)
    {
        m_softSeq_t::iterator it;
        for(it=m_softSeq.begin(); it!=m_softSeq.end(); ++it) {
            SCOPED_LOCK2(it->second->m_lock, guard);
            o(it->second);
        }
    }

private:
    evgMrm* const m_owner;
    m_softSeq_t   m_softSeq;
    epicsMutex    m_lock;
};
#endif //EVG_SEQ_MGR_H
