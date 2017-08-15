/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#include "evgMrm.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <math.h>

#include <errlog.h> 

#include <dbAccess.h>
#include <devSup.h>
#include <recSup.h>
#include <recGbl.h>
#include <epicsInterrupt.h>
#include <epicsTime.h>
#include <generalTimeSup.h>

#include <longoutRecord.h>

#include "mrf/version.h"
#include <mrfCommonIO.h> 
#include <mrfCommon.h> 

#ifdef __rtems__
#include <rtems/bspIo.h>
#endif //__rtems__

#include "evgRegMap.h"

#include <epicsExport.h>

evgMrm::evgMrm(const std::string& id, bus_configuration& busConfig, volatile epicsUInt8* const pReg, const epicsPCIDevice *pciDevice):
    mrf::ObjectInst<evgMrm>(id),
    TimeStampSource(1.0),
    irqExtInp_queued(0),
    m_buftx(id+":BUFTX",pReg+U32_DataBufferControl, pReg+U8_DataBuffer_base),
    m_pciDevice(pciDevice),
    m_id(id),
    m_pReg(pReg),
    busConfiguration(busConfig),
    m_seq(this, pReg),
    m_acTrig(id+":AcTrig", pReg),
    m_evtClk(id+":EvtClk", pReg),
  shadowIrqEnable(READ32(m_pReg, IrqEnable))
{
    epicsUInt32 v, isevr;

    v = READ32(m_pReg, FPGAVersion);
    isevr=v&FPGAVersion_TYPE_MASK;
    isevr>>=FPGAVersion_TYPE_SHIFT;

    if(isevr!=0x2)
        throw std::runtime_error("Address does not correspond to an EVG");

    for(int i = 0; i < evgNumEvtTrig; i++) {
        std::ostringstream name;
        name<<id<<":TrigEvt"<<i;
        m_trigEvt.push_back(new evgTrigEvt(name.str(), i, pReg));
    }

    for(int i = 0; i < evgNumMxc; i++) {
        std::ostringstream name;
        name<<id<<":Mxc"<<i;
        m_muxCounter.push_back(new evgMxc(name.str(), i, this));
    }

    for(int i = 0; i < evgNumDbusBit; i++) {
        std::ostringstream name;
        name<<id<<":Dbus"<<i;
        m_dbus.push_back(new evgDbus(name.str(), i, pReg));
    }

    for(int i = 0; i < evgNumFrontInp; i++) {
        std::ostringstream name;
        name<<id<<":FrontInp"<<i;
        m_input[ std::pair<epicsUInt32, InputType>(i, FrontInp) ] =
                new evgInput(name.str(), i, FrontInp, pReg + U32_FrontInMap(i));
    }

    for(int i = 0; i < evgNumUnivInp; i++) {
        std::ostringstream name;
        name<<id<<":UnivInp"<<i;
        m_input[ std::pair<epicsUInt32, InputType>(i, UnivInp) ] =
                new evgInput(name.str(), i, UnivInp, pReg + U32_UnivInMap(i));
    }

    for(int i = 0; i < evgNumRearInp; i++) {
        std::ostringstream name;
        name<<id<<":RearInp"<<i;
        m_input[ std::pair<epicsUInt32, InputType>(i, RearInp) ] =
                new evgInput(name.str(), i, RearInp, pReg + U32_RearInMap(i));
    }

    for(int i = 0; i < evgNumFrontOut; i++) {
        std::ostringstream name;
        name<<id<<":FrontOut"<<i;
        m_output[std::pair<epicsUInt32, evgOutputType>(i, FrontOut)] =
                new evgOutput(name.str(), i, FrontOut, pReg + U16_FrontOutMap(i));
    }

    for(int i = 0; i < evgNumUnivOut; i++) {
        std::ostringstream name;
        name<<id<<":UnivOut"<<i;
        m_output[std::pair<epicsUInt32, evgOutputType>(i, UnivOut)] =
                new evgOutput(name.str(), i, UnivOut, pReg + U16_UnivOutMap(i));
    }

    init_cb(&irqExtInp_cb, priorityHigh, &evgMrm::process_inp_cb, this);
    
    scanIoInit(&ioScanTimestamp);
}

evgMrm::~evgMrm() {
    for(int i = 0; i < evgNumEvtTrig; i++)
        delete m_trigEvt[i];

    for(int i = 0; i < evgNumMxc; i++)
        delete m_muxCounter[i];

    for(int i = 0; i < evgNumDbusBit; i++)
        delete m_dbus[i];

    for(int i = 0; i < evgNumFrontInp; i++)
        delete m_input[std::pair<epicsUInt32, InputType>(i, FrontInp)];

    for(int i = 0; i < evgNumUnivInp; i++)
        delete m_input[std::pair<epicsUInt32, InputType>(i, UnivInp)];

    for(int i = 0; i < evgNumRearInp; i++)
        delete m_input[std::pair<epicsUInt32, InputType>(i, RearInp)];

    for(int i = 0; i < evgNumFrontOut; i++)
        delete m_output[std::pair<epicsUInt32, evgOutputType>(i, FrontOut)];

    for(int i = 0; i < evgNumUnivOut; i++)
        delete m_output[std::pair<epicsUInt32, evgOutputType>(i, UnivOut)];
}

void evgMrm::enableIRQ()
{
    shadowIrqEnable |= EVG_IRQ_PCIIE          | //PCIe interrupt enable,
                       EVG_IRQ_ENABLE         |
                       EVG_IRQ_EXT_INP        |
                       EVG_IRQ_STOP_RAM(0)    |
                       EVG_IRQ_STOP_RAM(1)    |
                       EVG_IRQ_START_RAM(0)   |
                       EVG_IRQ_START_RAM(1);

    WRITE32(m_pReg, IrqEnable, shadowIrqEnable);
}

void 
evgMrm::init_cb(CALLBACK *ptr, int priority, void(*fn)(CALLBACK*), void* valptr) { 
    callbackSetPriority(priority, ptr); 
    callbackSetCallback(fn, ptr);     
    callbackSetUser(valptr, ptr);     
    (ptr)->timer=NULL;                            
}

const std::string
evgMrm::getId() const {
    return m_id;
}

volatile epicsUInt8*
evgMrm::getRegAddr() const {
    return m_pReg;
}

epicsUInt32 
evgMrm::getFwVersion() const {
    return READ32(m_pReg, FPGAVersion);
}

epicsUInt32
evgMrm::getFwVersionID(){
    epicsUInt32 ver = getFwVersion();

    ver &= FPGAVersion_VER_MASK;

    return ver;
}

formFactor
evgMrm::getFormFactor(){
    epicsUInt32 form = getFwVersion();

    form &= FPGAVersion_FORM_MASK;
    form >>= FPGAVersion_FORM_SHIFT;

    if(form <= formFactor_PCIe) return (formFactor)form;
    else return formFactor_unknown;
}

std::string
evgMrm::getFormFactorStr(){
    std::string text;

    switch(getFormFactor()){
    case formFactor_CPCI:
        text = "CompactPCI 3U";
        break;

    case formFactor_CPCIFULL:
        text = "CompactPCI 6U";
        break;

    case formFactor_CRIO:
        text = "CompactRIO";
        break;

    case formFactor_PCIe:
        text = "PCIe";
        break;

    case formFactor_PXIe:
        text = "PXIe";
        break;

    case formFactor_PMC:
        text = "PMC";
        break;

    case formFactor_VME64:
        text = "VME 64";
        break;

    default:
        text = "Unknown form factor";
    }

    return text;
}

std::string
evgMrm::getSwVersion() const {
    return MRF_VERSION;
}

epicsUInt32
evgMrm::getDbusStatus() const {
    return READ32(m_pReg, Status)>>16;
}

void
evgMrm::enable(bool ena) {
    SCOPED_LOCK(m_lock);
    if(ena)
        BITSET32(m_pReg, Control, EVG_MASTER_ENA);
    else
        BITCLR32(m_pReg, Control, EVG_MASTER_ENA);

    BITSET32(m_pReg, Control, EVG_DIS_EVT_REC);
    BITSET32(m_pReg, Control, EVG_REV_PWD_DOWN);
    BITSET32(m_pReg, Control, EVG_MXC_RESET);
}

bool
evgMrm::enabled() const {
    return (READ32(m_pReg, Control) & EVG_MASTER_ENA) != 0;
}

void
evgMrm::resetMxc(bool reset) {
    if(reset) {
        SCOPED_LOCK(m_lock);
        BITSET32(m_pReg, Control, EVG_MXC_RESET);
    }
}

void
evgMrm::isr_pci(void* arg) {
    evgMrm *evg = static_cast<evgMrm*>(arg);

    // Call to the generic implementation
    evg->isr(evg, true);

#if defined(__linux__) || defined(_WIN32)
    /**
     * On PCI variant of EVG the interrupts get disabled in kernel (by uio_mrf module) since IRQ task is completed here (in userspace).
     * Interrupts must therefore be re-enabled here.
     */
    if(devPCIEnableInterrupt(evg->m_pciDevice)) {
        printf("PCI: Failed to enable interrupt\n");
        return;
    }
#endif
}

void
evgMrm::isr_vme(void* arg) {
    evgMrm *evg = static_cast<evgMrm*>(arg);

    // Call to the generic implementation
    evg->isr(evg, false);
}

void
evgMrm::isr(evgMrm *evg, bool pci) {
try{
    epicsUInt32 flags = READ32(evg->m_pReg, IrqFlag);
    epicsUInt32 active = flags & evg->shadowIrqEnable;

#if defined(vxWorks) || defined(__rtems__)
    if(!active) {
#  ifdef __rtems__
        if(!pci)
            printk("evgMrm::isr with no active VME IRQ 0x%08x 0x%08x\n", flags, evg->shadowIrqEnable);
#else
        (void)pci;
#  endif
        // this is a shared interrupt
        return;
    }
    // Note that VME devices do not normally shared interrupts
#else
    // for Linux, shared interrupts are detected by the kernel module
    // so any notifications to userspace are real interrupts by this device
    (void)pci;
#endif

    if(active & EVG_IRQ_START_RAM(0)) {
        evg->m_seq.doStartOfSequence(0);
    }

    if(active & EVG_IRQ_START_RAM(1)) {
        evg->m_seq.doStartOfSequence(1);
    }

    if(active & EVG_IRQ_STOP_RAM(0)) {
        evg->m_seq.doEndOfSequence(0);
    }

    if(active & EVG_IRQ_STOP_RAM(1)) {
        evg->m_seq.doEndOfSequence(1);
    }

    if(active & EVG_IRQ_EXT_INP) {
        if(evg->irqExtInp_queued==0) {
            callbackRequest(&evg->irqExtInp_cb);
            evg->irqExtInp_queued=1;
        } else if(evg->irqExtInp_queued==1) {
            evg->shadowIrqEnable &= ~EVG_IRQ_EXT_INP;
            evg->irqExtInp_queued=2;
        }
    }

    WRITE32(evg->getRegAddr(), IrqEnable, evg->shadowIrqEnable);
    WRITE32(evg->m_pReg, IrqFlag, flags);  // Clear the interrupt causes
    READ32(evg->m_pReg, IrqFlag);          // Make sure the clear completes before returning

}catch(...){
    epicsInterruptContextMessage("c++ Exception in ISR!!!\n");
}
}

void
evgMrm::process_inp_cb(CALLBACK *pCallback) {
    void* pVoid;
    callbackGetUser(pVoid, pCallback);
    evgMrm* evg = static_cast<evgMrm*>(pVoid);

    {
        interruptLock ig;
        if(evg->irqExtInp_queued==2) {
            evg->shadowIrqEnable |= EVG_IRQ_EXT_INP;
            WRITE32(evg->getRegAddr(), IrqEnable, evg->shadowIrqEnable);
        }
        evg->irqExtInp_queued=0;
    }
     
    evg->tickSecond();
    scanIoRequest(evg->ioScanTimestamp);
}

void
evgMrm::postSoftSecondsSrc()
{
    tickSecond();
    scanIoRequest(ioScanTimestamp);
}

void
evgMrm::setEvtCode(epicsUInt32 evtCode) {
    if(evtCode > 255)
        throw std::runtime_error("Event Code out of range. Valid range: 0 - 255.");

    SCOPED_LOCK(m_lock);

    while(READ32(m_pReg, SwEvent) & SwEvent_Pend) {}

    WRITE32(m_pReg, SwEvent,
            (evtCode<<SwEvent_Code_SHIFT)
            |SwEvent_Ena);
}

/**    Access    functions     **/

evgEvtClk*
evgMrm::getEvtClk() {
    return &m_evtClk;
}

evgInput*
evgMrm::getInput(epicsUInt32 inpNum, InputType type) {
    evgInput* inp = m_input[ std::pair<epicsUInt32, InputType>(inpNum, type) ];
    if(!inp)
        throw std::runtime_error("Input not initialized");

    return inp;
}

epicsEvent* 
evgMrm::getTimerEvent() {
    return &m_timerEvent;
}

const bus_configuration *evgMrm::getBusConfiguration()
{
    return &busConfiguration;
}

void evgMrm::show(int lvl)
{
}
