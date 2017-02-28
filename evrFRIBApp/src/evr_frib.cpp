/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2017.
 *
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include "evr_frib.h"

#include "mrfCommonIO.h"
#include "evrFRIBRegMap.h"

EVRFRIB::EVRFRIB(const std::string& s,
                 bus_configuration &busConfig,
                 volatile unsigned char *base)
    :base_t(s, busConfig)
    ,base(base)
{
}

EVRFRIB::~EVRFRIB() {}

epicsUInt32 EVRFRIB::version() const {
    return READ32(base, FWVersion);
}

bool EVRFRIB::enabled() const {}
void EVRFRIB::enable(bool) {}

double EVRFRIB::clock() const {}
void EVRFRIB::clockSet(double clk) {}


bool EVRFRIB::pllLocked() const {}

double EVRFRIB::clockTS() const {}
void EVRFRIB::clockTSSet(double) {}

bool EVRFRIB::interestedInEvent(epicsUInt32 event,bool set) {}

bool EVRFRIB::TimeStampValid() const {}

bool EVRFRIB::getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event) {}

bool EVRFRIB::getTicks(epicsUInt32 *tks) {}

bool EVRFRIB::linkStatus() const {}

epicsUInt32 EVRFRIB::recvErrorCount() const {}
