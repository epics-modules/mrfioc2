/*************************************************************************\
* Copyright (c) 2020 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef DRVEMTSBUFFER_H
#define DRVEMTSBUFFER_H

#include <vector>
#include <utility> // std::pair

#include <dbScan.h>

#include "mrf/object.h"

class EVRMRM;

struct EVRMRMTSBuffer : public mrf::ObjectInst<EVRMRMTSBuffer>
{
    typedef mrf::ObjectInst<EVRMRMTSBuffer> base_t;
    OBJECT_DECL(EVRMRMTSBuffer);

    explicit EVRMRMTSBuffer(const std::string& n, EVRMRM* evr);
    virtual ~EVRMRMTSBuffer();

    virtual void lock() const OVERRIDE FINAL;
    virtual void unlock() const OVERRIDE FINAL;

    epicsUInt32 dropCount() const { return dropped; }

    epicsUInt16 timeEvent() const { return timeEvt; }
    void flushTimeSet(epicsUInt16 v);

    epicsUInt16 flushEvent() const { return flushEvt; }
    void flushEventSet(epicsUInt16 v);

    void flushNow();
    void doFlush();

    epicsUInt32 getTimesRelFirst(epicsInt32 *arr, epicsUInt32 count) const;
    epicsUInt32 getTimesRelFlush(epicsInt32 *arr, epicsUInt32 count) const;
    epicsUInt32 getTimesRelPrevFlush(epicsInt32 *arr, epicsUInt32 count) const;

    IOSCANPVT flushed() const { return scan; }

    EVRMRM* const evr;

    epicsUInt32 dropped;

    IOSCANPVT scan;

    epicsUInt8 timeEvt;
    epicsUInt8 flushEvt;

    struct ebuf_t {
        size_t pos;
        std::vector<epicsTimeStamp> buf;
        epicsTimeStamp flushtime, prevflushtime;
        bool ok, prevok;
        bool drop;
        ebuf_t() :pos(0u), ok(false), prevok(false), drop(false) {
            flushtime.secPastEpoch = 0u;
            flushtime.nsec = 0u;
            prevflushtime = flushtime;
        }
    private:
        ebuf_t(const ebuf_t&);
    } ebufs[2];
    // active buffer being filled.  Other is for readout.
    // must lock both evr and this mutex to change.
    unsigned char active; // either 0 or 1
};

#endif // DRVEMTSBUFFER_H
