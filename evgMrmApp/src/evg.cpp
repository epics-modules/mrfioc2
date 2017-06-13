#define DATABUF_H_INC_LEVEL2

#include <epicsThread.h>
#include <epicsTime.h>
#include <generalTimeSup.h>

#include <epicsExport.h>

#include "evgOutput.h"
#include "evgAcTrig.h"
#include "evgDbus.h"
#include "evgInput.h"
#include "evgTrigEvt.h"
#include "evgMxc.h"
#include "evgEvtClk.h"
#include "evgMrm.h"

OBJECT_BEGIN(evgAcTrig) {
    OBJECT_PROP2("Divider", &evgAcTrig::getDivider, &evgAcTrig::setDivider);
    OBJECT_PROP2("Phase",   &evgAcTrig::getPhase,   &evgAcTrig::setPhase);
    OBJECT_PROP2("Bypass",  &evgAcTrig::getBypass,  &evgAcTrig::setBypass);
    OBJECT_PROP2("SyncSrc", &evgAcTrig::getSyncSrc, &evgAcTrig::setSyncSrc);
} OBJECT_END(evgAcTrig)

OBJECT_BEGIN(evgDbus) {
    OBJECT_PROP2("Source", &evgDbus::getSource, &evgDbus::setSource);
} OBJECT_END(evgDbus)

OBJECT_BEGIN(evgEvtClk) {
    OBJECT_PROP2("Source",      &evgEvtClk::getSource, &evgEvtClk::setSource);
    OBJECT_PROP2("RFFreq",      &evgEvtClk::getRFFreq, &evgEvtClk::setRFFreq);
    OBJECT_PROP2("RFDiv",       &evgEvtClk::getRFDiv,  &evgEvtClk::setRFDiv);
    OBJECT_PROP2("FracSynFreq", &evgEvtClk::getFracSynFreq, &evgEvtClk::setFracSynFreq);
    OBJECT_PROP1("Frequency",   &evgEvtClk::getFrequency);
} OBJECT_END(evgEvtClk)

OBJECT_BEGIN(evgInput) {
    OBJECT_PROP2("IRQ", &evgInput::getExtIrq, &evgInput::setExtIrq);
} OBJECT_END(evgInput)

OBJECT_BEGIN(evgMxc) {
    OBJECT_PROP1("Status",    &evgMxc::getStatus);
    OBJECT_PROP2("Polarity",  &evgMxc::getPolarity,  &evgMxc::setPolarity);
    OBJECT_PROP2("Frequency", &evgMxc::getFrequency, &evgMxc::setFrequency);
    OBJECT_PROP2("Prescaler", &evgMxc::getPrescaler, &evgMxc::setPrescaler);
} OBJECT_END(evgMxc)

OBJECT_BEGIN(evgOutput) {
    OBJECT_PROP2("Source", &evgOutput::getSource, &evgOutput::setSource);
} OBJECT_END(evgOutput)

OBJECT_BEGIN(evgTrigEvt) {
    OBJECT_PROP2("EvtCode", &evgTrigEvt::getEvtCode, &evgTrigEvt::setEvtCode);
} OBJECT_END(evgTrigEvt)

OBJECT_BEGIN(evgMrm) {
    OBJECT_PROP2("Enable",     &evgMrm::enabled,      &evgMrm::enable);
    OBJECT_PROP2("Reset MXC",  &evgMrm::getResetMxc,  &evgMrm::resetMxc);
    OBJECT_PROP2("Sync TS",    &evgMrm::getSyncTsRequest, &evgMrm::syncTsRequest);
    OBJECT_PROP1("DbusStatus", &evgMrm::getDbusStatus);
    OBJECT_PROP1("Version", &evgMrm::getFwVersion);
    OBJECT_PROP1("Sw Version", &evgMrm::getSwVersion);
    OBJECT_PROP1("Time Error", &evgMrm::timeError);
    OBJECT_PROP1("Time Error", &evgMrm::timeErrorScan);
    OBJECT_PROP2("EvtCode", &evgMrm::writeonly, &evgMrm::setEvtCode);
} OBJECT_END(evgMrm)


