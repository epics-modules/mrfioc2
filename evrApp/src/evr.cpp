/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include "mrf/version.h"

#include "dbCommon.h"
#include <epicsExport.h>

#include "evr/evr.h"
#include "evr/pulser.h"
#include "evr/output.h"
#include "evr/delay.h"
#include "evr/input.h"
#include "evr/prescaler.h"
#include "evr/cml.h"
#include "evr/util.h"

/**@file evr.cpp
 *
 * Contains implimentations of the pure virtual
 * destructors of the interfaces for C++ reasons.
 * These must be present event though they are never
 * called.  If they are absent that linking will
 * fail in some cases.
 */

EVR::~EVR()
{
}

std::string EVR::versionStr() const
{
    return version().str();
}

std::string EVR::versionSw() const
{
    return MRF_VERSION;
}


bus_configuration *EVR::getBusConfiguration(){
    return &busConfiguration;
}

std::string EVR::position() const
{
    std::ostringstream position;

    if(busConfiguration.busType == busType_pci)
        position << std::hex << busConfiguration.pci.bus << ":"
                 << std::hex << busConfiguration.pci.device << "."
                 << std::hex << busConfiguration.pci.function;
    else if(busConfiguration.busType == busType_vme) position << "Slot #" << busConfiguration.vme.slot;
    else position << "Unknown position";

    return position.str();
}

Pulser::~Pulser()
{
}

Output::~Output()
{
}

Input::~Input()
{
}

PreScaler::~PreScaler()
{
}

CML::~CML()
{
}

DelayModuleEvr::~DelayModuleEvr()
{
}

long get_ioint_info_statusChange(int dir,dbCommon* prec,IOSCANPVT* io)
{
    IOStatus* stat=static_cast<IOStatus*>(prec->dpvt);

    if(!stat) return 1;

    *io=stat->statusChange((dir != 0));

    return 0;
}

OBJECT_BEGIN(EVR) {

    OBJECT_PROP1("Model", &EVR::model);

    OBJECT_PROP1("Version", &EVR::versionStr);
    OBJECT_PROP1("Sw Version", &EVR::versionSw);

    OBJECT_PROP1("Position", &EVR::position);

    OBJECT_PROP1("Event Clock TS Div", &EVR::uSecDiv);

    OBJECT_PROP1("Receive Error Count", &EVR::recvErrorCount);
    OBJECT_PROP1("Receive Error Count", &EVR::linkChanged);

    OBJECT_PROP1("FIFO Overflow Count", &EVR::FIFOFullCount);

    OBJECT_PROP1("FIFO Over rate", &EVR::FIFOOverRate);
    OBJECT_PROP1("FIFO Event Count", &EVR::FIFOEvtCount);
    OBJECT_PROP1("FIFO Loop Count", &EVR::FIFOLoopCount);

    OBJECT_PROP1("HB Timeout Count", &EVR::heartbeatTIMOCount);
    OBJECT_PROP1("HB Timeout Count", &EVR::heartbeatTIMOOccured);

    OBJECT_PROP1("Timestamp Prescaler", &EVR::tsDiv);

    OBJECT_PROP2("Timestamp Source", &EVR::SourceTSraw, &EVR::setSourceTSraw);

    OBJECT_PROP2("Clock", &EVR::clock, &EVR::clockSet);

    OBJECT_PROP2("Timestamp Clock", &EVR::clockTS, &EVR::clockTSSet);

    OBJECT_PROP2("Enable", &EVR::enabled, &EVR::enable);

    OBJECT_PROP2("External Inhibit", &EVR::extInhib, &EVR::setExtInhib);

    OBJECT_PROP1("PLL Lock Status", &EVR::pllLocked);

    OBJECT_PROP1("Interrupt Count", &EVR::irqCount);

    OBJECT_PROP1("Link Status", &EVR::linkStatus);
    OBJECT_PROP1("Link Status", &EVR::linkChanged);

    OBJECT_PROP1("Timestamp Valid", &EVR::TimeStampValid);
    OBJECT_PROP1("Timestamp Valid", &EVR::TimeStampValidEvent);

    OBJECT_PROP1("SW Output status", &EVR::mappedOutputState);

} OBJECT_END(EVR)


OBJECT_BEGIN(Input) {

    OBJECT_PROP2("Active Level", &Input::levelHigh, &Input::levelHighSet);

    OBJECT_PROP2("Active Edge", &Input::edgeRise, &Input::edgeRiseSet);

    OBJECT_PROP2("External Code", &Input::extEvt, &Input::extEvtSet);

    OBJECT_PROP2("Backwards Code", &Input::backEvt, &Input::backEvtSet);

    OBJECT_PROP2("External Mode", &Input::extModeraw, &Input::extModeSetraw);

    OBJECT_PROP2("Backwards Mode", &Input::backModeraw, &Input::backModeSetraw);

    OBJECT_PROP2("DBus Mask", &Input::dbus, &Input::dbusSet);

} OBJECT_END(Input)


OBJECT_BEGIN(Output) {

    OBJECT_PROP2("Map", &Output::source, &Output::setSource);

    OBJECT_PROP2("Enable", &Output::enabled, &Output::enable);

} OBJECT_END(Output)


OBJECT_BEGIN(Pulser) {

    OBJECT_PROP2("Delay", &Pulser::delay, &Pulser::setDelay);
    OBJECT_PROP2("Delay", &Pulser::delayRaw, &Pulser::setDelayRaw);

    OBJECT_PROP2("Width", &Pulser::width, &Pulser::setWidth);
    OBJECT_PROP2("Width", &Pulser::widthRaw, &Pulser::setWidthRaw);

    OBJECT_PROP2("Enable", &Pulser::enabled, &Pulser::enable);

    OBJECT_PROP2("Polarity", &Pulser::polarityInvert, &Pulser::setPolarityInvert);

    OBJECT_PROP2("Prescaler", &Pulser::prescaler, &Pulser::setPrescaler);

} OBJECT_END(Pulser)


OBJECT_BEGIN(PreScaler) {

    OBJECT_PROP2("Divide", &PreScaler::prescaler, &PreScaler::setPrescaler);

} OBJECT_END(PreScaler)


OBJECT_BEGIN(CML) {

    OBJECT_PROP2("Enable", &CML::enabled, &CML::enable);

    OBJECT_PROP2("Reset", &CML::inReset, &CML::reset);

    OBJECT_PROP2("Power", &CML::powered, &CML::power);

    OBJECT_PROP2("Freq Trig Lvl", &CML::polarityInvert, &CML::setPolarityInvert);

    OBJECT_PROP2("Pat Recycle", &CML::recyclePat, &CML::setRecyclePat);

    OBJECT_PROP2("Counts High", &CML::timeHigh, &CML::setTimeHigh);
    OBJECT_PROP2("Counts High", &CML::countHigh, &CML::setCountHigh);

    OBJECT_PROP2("Counts Low", &CML::timeLow, &CML::setTimeLow);
    OBJECT_PROP2("Counts Low", &CML::countLow, &CML::setCountLow);

    OBJECT_PROP2("Counts Init", &CML::timeInit, &CML::setTimeInit);
    OBJECT_PROP2("Counts Init", &CML::countInit, &CML::setCountInit);

    OBJECT_PROP2("Fine Delay", &CML::fineDelay, &CML::setFineDelay);

    OBJECT_PROP1("Freq Mult", &CML::freqMultiple);

    OBJECT_PROP2("Mode", &CML::modeRaw, &CML::setModRaw);

    OBJECT_PROP2("Waveform", &CML::getPattern<CML::patternWaveform>,
                             &CML::setPattern<CML::patternWaveform>);

    OBJECT_PROP2("Pat Rise", &CML::getPattern<CML::patternRise>,
                             &CML::setPattern<CML::patternRise>);

    OBJECT_PROP2("Pat High", &CML::getPattern<CML::patternHigh>,
                             &CML::setPattern<CML::patternHigh>);

    OBJECT_PROP2("Pat Fall", &CML::getPattern<CML::patternFall>,
                             &CML::setPattern<CML::patternFall>);

    OBJECT_PROP2("Pat Low", &CML::getPattern<CML::patternLow>,
                            &CML::setPattern<CML::patternLow>);

} OBJECT_END(CML)

OBJECT_BEGIN(DelayModuleEvr) {

	OBJECT_PROP2("Enable", &DelayModuleEvr::enabled, &DelayModuleEvr::setState);
	OBJECT_PROP2("Delay0", &DelayModuleEvr::getDelay0, &DelayModuleEvr::setDelay0);
	OBJECT_PROP2("Delay1", &DelayModuleEvr::getDelay1, &DelayModuleEvr::setDelay1);

} OBJECT_END(DelayModuleEvr)
