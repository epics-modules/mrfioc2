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

OBJECT_BEGIN(evgInput) {
    OBJECT_PROP2("IRQ", &evgInput::getExtIrq, &evgInput::setExtIrq);    
    OBJECT_PROP2("FPMASK", &evgInput::getHwMask, &evgInput::setHwMask);
    OBJECT_PROP1("FPMASK", &evgInput::stateChange);

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
    OBJECT_PROP1("DbusStatus", &evgMrm::getDbusStatus);
    OBJECT_PROP1("Version", &evgMrm::getFwVersionStr);
    OBJECT_PROP1("Sw Version", &evgMrm::getSwVersion);
    OBJECT_PROP1("CommitHash", &evgMrm::getCommitHash);
    OBJECT_PROP2("EvtCode", &evgMrm::writeonly, &evgMrm::setEvtCode);
    OBJECT_PROP2("TS Generator", &evgMrm::getTSGenerator, &evgMrm::setTSGenerator);
    {
      bool (evgMrm::*getter)() const = &evgMrm::isSoftSeconds;
      void (evgMrm::*setter)(bool) = &evgMrm::softSecondsSrc;
      OBJECT_PROP2("SimTime", getter, setter);
    }
    {
      std::string (evgMrm::*getter)() const = &evgMrm::nextSecond;
      OBJECT_PROP1("NextSecond", getter);
    }
    OBJECT_PROP1("SoftTick", &evgMrm::postSoftSecondsSrc);
    {
      double (evgMrm::*getter)() const = &evgMrm::deltaSeconds;
      OBJECT_PROP1("Time Error", getter);
    }
    OBJECT_PROP1("Time Error", &evgMrm::timeErrorScan);
    OBJECT_PROP1("NextSecond", &evgMrm::timeErrorScan);
    {
      void (evgMrm::*cmd)() = &evgMrm::resyncSecond;
      OBJECT_PROP1("Sync TS", cmd);
    }
    OBJECT_PROP2("Source",      &evgMrm::getSource, &evgMrm::setSource);
    OBJECT_PROP2("RFFreq",      &evgMrm::getRFFreq, &evgMrm::setRFFreq);
    OBJECT_PROP2("RFDiv",       &evgMrm::getRFDiv,  &evgMrm::setRFDiv);
    OBJECT_PROP2("FracSynFreq", &evgMrm::getFracSynFreq, &evgMrm::setFracSynFreq);
    OBJECT_PROP1("Frequency",   &evgMrm::getFrequency);
    OBJECT_PROP1("PLL Lock Status", &evgMrm::pllLocked);
    OBJECT_PROP2("PLL Bandwidth",   &evgMrm::getPLLBandwidth, &evgMrm::setPLLBandwidth);
} OBJECT_END(evgMrm)
