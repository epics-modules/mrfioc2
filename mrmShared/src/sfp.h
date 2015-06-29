/*************************************************************************\
* Copyright (c) 2014 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef SFP_H
#define SFP_H

#include <string>
#include <vector>

#include <epicsMutex.h>

class epicsShareClass SFP : public mrf::ObjectInst<SFP> {
    volatile unsigned char* base;
    typedef std::vector<epicsUInt8> buffer_t;
    buffer_t buffer;
    bool valid;
    mutable epicsMutex guard;

    epicsInt16 read16(unsigned int) const;
public:
    SFP(const std::string& n, volatile unsigned char* reg);
    virtual ~SFP();

    virtual void lock() const{guard.lock();};
    virtual void unlock() const{guard.unlock();};

    bool junk() const{return 0;}
    void updateNow(bool=true);

    double linkSpeed() const;
    double temperature() const;
    double powerTX() const;
    double powerRX() const;

    std::string vendorName() const;
    std::string vendorPart() const;
    std::string vendorRev() const;
    std::string serial() const;
    std::string manuDate() const;

    void report() const;

};

#endif // SFP_H
