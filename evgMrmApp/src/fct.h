/*************************************************************************\
* Copyright (c) 2018 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#ifndef EVG_FCT_H
#define EVG_FCT_H

#include <vector>

#include "mrfCommon.h"
#include "mrf/object.h"

class evgMrm;
class SFP;

// Fanout/ConcenTrator
class FCT : public mrf::ObjectInst<FCT>
{
    evgMrm *evg;
    volatile epicsUInt8* const base;
    std::vector<SFP*> sfp;
public:
    FCT(evgMrm *evg, const std::string& id, volatile epicsUInt8* const base);
    virtual ~FCT();

    virtual void lock() const OVERRIDE FINAL {}
    virtual void unlock() const OVERRIDE FINAL {}

    epicsUInt16 statusRaw() const;
    double dcUpstream() const;
    double dcFIFO() const;
    double dcInternal() const;
    bool getDcUpMode() const;
    void setDcUpMode(bool ena);
    double getDcUpTarget() const;
    void setDcUpTarget(double target);

    epicsUInt32 topoId() const;

    double dcPortN(unsigned port) const;
    epicsUInt32 dcPortNStatus(unsigned port) const;

    template<int port>
    double dcPort() const {
        return dcPortN(port);
    }

    template<int port>
    epicsUInt32 dcPortStatus() const {
        return dcPortNStatus(port);
    }

    // Read Rx Shutter enable/disable bits
    epicsUInt32 getRxShutter() const;
    // Set Rx Shutter enable/disable bits
    void setRxShutter(epicsUInt32);

};

#endif // EVG_FCT_H
