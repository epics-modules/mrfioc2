/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include "drvemIocsh.h"

#include <cstdio>
#include <cstring>

#include <stdexcept>
#include <sstream>
#include <map>

#include <epicsString.h>
#include <drvSup.h>
#include <iocsh.h>
#include <initHooks.h>
#include <epicsExit.h>
#include <epicsExport.h>

#include <devLibPCI.h>
#include <devcsr.h>
#include <epicsInterrupt.h>
#include <epicsThread.h>
#include "mrmpci.h"
#include "mrfcsr.h"

#include "drvem.h"
#include "evrRegMap.h"
#include "plx9030.h"
#include "plx9056.h"

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

/* Bit mask used to communicate which VME interrupt levels
 * are used.  Bits are set by mrmEvrSetupVME().  Levels are
 * enabled later during iocInit.
 */
static epicsUInt8 vme_level_mask = 0;

static const epicsPCIID mrmevrs[] = {
    DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                               PCI_DEVICE_ID_MRF_PMCEVR_230, PCI_VENDOR_ID_MRF)
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                                PCI_DEVICE_ID_MRF_PXIEVR_230, PCI_VENDOR_ID_MRF)
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9056,    PCI_VENDOR_ID_PLX,
                                PCI_DEVICE_ID_MRF_EVRTG_300, PCI_VENDOR_ID_MRF)
    ,DEVPCI_END
};

static const struct VMECSRID vmeevrs[] = {
    // VME EVR RF 230
    {MRF_VME_IEEE_OUI, MRF_VME_EVR_RF_BID|MRF_SERIES_230, VMECSRANY}
    ,VMECSR_END
};

static
const
struct printreg
{
    const char *label;
    epicsUInt32 offset;
    int rsize;
} printreg[] = {
#define REGINFO(label, name, size) {label, U##size##_##name, size}
REGINFO("Version", FWVersion, 32),
REGINFO("Control", Control, 32),
REGINFO("Status",  Status, 32),
REGINFO("IRQ Flag",IRQFlag, 32),
REGINFO("IRQ Ena", IRQEnable, 32),
REGINFO("IRQPlsmap",IRQPulseMap, 32),
REGINFO("DBufCtrl",DataBufCtrl, 32),
REGINFO("DBufTxCt",DataTxCtrl, 32),
REGINFO("CountPS", CounterPS, 32),
REGINFO("USecDiv", USecDiv, 32),
REGINFO("ClkCtrl", ClkCtrl, 32),
REGINFO("LogSts",  LogStatus, 32),
REGINFO("TSSec",TSSec, 32),
REGINFO("TSEvt",TSEvt, 32),
REGINFO("TSSecLath",TSSecLatch, 32),
REGINFO("TSEvtLath",TSEvtLatch, 32),
REGINFO("FracDiv", FracDiv, 32),
REGINFO("Scaler0",Scaler(0),32),
REGINFO("Pul0Ctrl",PulserCtrl(0),32),
REGINFO("Pul0Scal",PulserScal(0),32),
REGINFO("Pul0Dely",PulserDely(0),32),
REGINFO("Pul0Wdth",PulserWdth(0),32),
REGINFO("FP01MAP",OutputMapFP(0),32),
REGINFO("FPU01MAP",OutputMapFPUniv(0),32),
REGINFO("RB01MAP",OutputMapRB(0),32),
REGINFO("FPIN0",InputMapFP(0),32),
REGINFO("CML4Low",OutputCMLLow(0),32),
REGINFO("CML4Rise",OutputCMLRise(0),32),
REGINFO("CML4High",OutputCMLHigh(0),32),
REGINFO("CML4Fall",OutputCMLFall(0),32),
REGINFO("CML4Ena",OutputCMLEna(0),32),
REGINFO("CML4Cnt",OutputCMLCount(0),32),
REGINFO("CML4Len",OutputCMLPatLength(0),32),
REGINFO("CML4Pat0",OutputCMLPat(0,0),32),
REGINFO("CML4Pat1",OutputCMLPat(0,1),32),
REGINFO("TXBuf0-3",DataTx(0),32),
REGINFO("RXBuf0-3",DataRx(0),32)
#undef REGINFO
};

static
void
printregisters(volatile epicsUInt8 *evr,epicsUInt32 len)
{
    size_t reg;

    printf("EVR\n");
    for(reg=0; reg<NELEMENTS(printreg); reg++){

        if(printreg[reg].offset+printreg[reg].rsize/8 > len)
            continue;

        switch(printreg[reg].rsize){
        case 8:
            printf("%9s: %02x\n",
                   printreg[reg].label,
                   ioread8(evr+printreg[reg].offset));
            break;
        case 16:
            printf("%9s: %04x\n",
                   printreg[reg].label,
                   nat_ioread16(evr+printreg[reg].offset));
            break;
        case 32:
            printf("%9s: %08x\n",
                   printreg[reg].label,
                   nat_ioread32(evr+printreg[reg].offset));
            break;
        }
    }
}

static
bool reportCard(mrf::Object* obj, void* raw)
{
    int *level=(int*)raw;
    EVRMRM *evr=dynamic_cast<EVRMRM*>(obj);
    if(!evr)
        return true;

    printf("--- EVR %s ---\n",obj->name().c_str());

    printf("Model: %08x  Version: %08x\n",evr->model(),evr->version());

    printf("Clock: %.6f MHz\n",evr->clock()*1e-6);

    if(*level>=2){
        printregisters(evr->base, evr->baselen);
    }
    if(*level>=1 && evr->sfp.get()){
        evr->sfp->updateNow();
        evr->sfp->report();
    }

    return true;
}

static
long report(int level)
{
    printf("=== Begin MRF EVR support ===\n");
    mrf::Object::visitObjects(&reportCard, (void*)&level);
    printf("=== End MRF EVR support ===\n");
    return 0;
}

static
void checkVersion(volatile epicsUInt8 *base, unsigned int required, unsigned int recommended)
{
    epicsUInt32 v = READ32(base, FWVersion),evr,ver;

    errlogPrintf("FWVersion 0x%08x\n", v);
    if(v&FWVersion_zero_mask)
        throw std::runtime_error("Invalid firmware version (HW or bus error)");

    evr=v&FWVersion_type_mask;
    evr>>=FWVersion_type_shift;

    if(evr!=0x1)
        throw std::runtime_error("Address does not correspond to an EVR");

    ver=v&FWVersion_ver_mask;
    ver>>=FWVersion_ver_shift;

    errlogPrintf("Found version %u\n", ver);

    if(ver<required) {
        errlogPrintf("Firmware version >=%u is required\n", required);
        throw std::runtime_error("Firmware version not supported");

    } else if(ver<recommended) {
        errlogPrintf("Firmware version >=%u is recommended, please consider upgrading\n", recommended);
    }
}

extern "C"
void
mrmEvrSetupPCI(const char* id,int b,int d,int f)
{
try {
    std::ostringstream position;
    if(mrf::Object::getObject(id)){
        printf("ID %s already in use\n",id);
        return;
    }

    const epicsPCIDevice *cur=0;
    if( devPCIFindBDF(mrmevrs,b,d,f,&cur,0) ){
        printf("PCI Device not found\n");
        return;
    }

    printf("Device %s  %u:%u.%u\n",id,cur->bus,cur->device,cur->function);
    position<<cur->bus<<":"<<cur->device<<"."<<cur->function;
    printf("Using IRQ %u\n",cur->irq);

    volatile epicsUInt8 *plx, *evr;

    if( devPCIToLocalAddr(cur,0,(volatile void**)(void *)&plx,DEVLIB_MAP_UIO1TO1) ||
        devPCIToLocalAddr(cur,2,(volatile void**)(void *)&evr,DEVLIB_MAP_UIO1TO1))
    {
        printf("Failed to map BARs 0 and 2\n");
        return;
    }
    if(!plx || !evr){
        printf("BARs mapped to zero? (%08lx,%08lx)\n",
               (unsigned long)plx,(unsigned long)evr);
        return;
    }

    epicsUInt32 evrlen;
    if( devPCIBarLen(cur,2,&evrlen) ) {
        printf("Can't find BAR #2 length\n");
        return;
    }

    // handle various PCI to local bus bridges
    switch(cur->id.device) {
    case PCI_DEVICE_ID_PLX_9030:
        /* Use the PLX device on the EVR to swap access on
         * little endian systems so we don't have no worry about
         * byte order :)
         */
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
        BITSET(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
        BITCLR(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
#endif

        // Disable interrupts on device

        NAT_WRITE32(evr, IRQEnable, 0);

#ifndef __linux__
        /* Enable active high interrupt1 through the PLX to the PCI bus.
         */
        LE_WRITE16(plx, INTCSR, INTCSR_INT1_Enable|
                   INTCSR_INT1_Polarity|
                   INTCSR_PCI_Enable);
#else
        /* ask the kernel module to enable interrupts through the PLX bridge */
        if(devPCIEnableInterrupt(cur)) {
            printf("Failed to enable interrupt\n");
            return;
        }
#endif
        break;

    case PCI_DEVICE_ID_PLX_9056:
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
        BITSET(LE,8, plx, BIGEND9056, BIGEND9056_BIG);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
        BITCLR(LE,8, plx, BIGEND9056, BIGEND9056_BIG);
#endif

        // Disable interrupts on device

        NAT_WRITE32(evr, IRQEnable, 0);

#ifndef __linux__
        BITSET(LE,32,plx, INTCSR9056, INTCSR9056_PCI_Enable|INTCSR9056_LCL_Enable);
#else
        /* ask the kernel module to enable interrupts through the PLX bridge */
        if(devPCIEnableInterrupt(cur)) {
            printf("Failed to enable interrupt\n");
            return;
        }
#endif
        break;

    default:
        printf("Unknown PCI bridge %04x\n", cur->id.device);
        return;
    }

    checkVersion(evr, 3, 6);

    // Acknowledge missed interrupts
    //TODO: This avoids a spurious FIFO Full
    NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

    // Install ISR

    EVRMRM *receiver=new EVRMRM(id,position.str(),evr,evrlen);

    void *arg=receiver;
#ifdef __linux__
    receiver->isrLinuxPvt = (void*)cur;
#endif

    if(devPCIConnectInterrupt(cur, &EVRMRM::isr, arg, 0)){
        printf("Failed to install ISR\n");
        delete receiver;
    }else{
        // Interrupts will be enabled during iocInit()
    }
} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
}
}

static
void
printRamEvt(EVRMRM *evr,int evt,int ram)
{
    if(evt<0 || evt>255)
        return;
    if(ram<0 || ram>1)
        return;

    epicsUInt32 map[4];

    map[0]=NAT_READ32(evr->base, MappingRam(ram,evt,Internal));
    map[1]=NAT_READ32(evr->base, MappingRam(ram,evt,Trigger));
    map[2]=NAT_READ32(evr->base, MappingRam(ram,evt,Set));
    map[3]=NAT_READ32(evr->base, MappingRam(ram,evt,Reset));

    printf("Event 0x%02x %3d ",evt,evt);
    printf("%08x %08x %08x %08x\n",map[0],map[1],map[2],map[3]);
}

static
bool
enableIRQ(mrf::Object* obj, void*)
{
    EVRMRM *mrm=dynamic_cast<EVRMRM*>(obj);
    if(!mrm)
        return true;

    mrm->enableIRQ();

    return true;
}

static
bool
disableIRQ(mrf::Object* obj, void*)
{
    EVRMRM *mrm=dynamic_cast<EVRMRM*>(obj);
    if(!mrm)
        return true;

    WRITE32(mrm->base, IRQEnable, 0);
    return true;
}

static
void
evrShutdown(void*)
{
    mrf::Object::visitObjects(&disableIRQ,0);
}

static
void inithooks(initHookState state)
{
    epicsUInt8 lvl;
    switch(state)
    {
    case initHookAfterInterruptAccept:
        // Register hook to disable interrupts on IOC shutdown
        epicsAtExit(&evrShutdown, NULL);
        // First enable interrupts for each EVR
        mrf::Object::visitObjects(&enableIRQ,0);
        // Then enable all used levels
        for(lvl=1; lvl<=7; ++lvl)
        {
            if (vme_level_mask&(1<<(lvl-1))) {
                if(devEnableInterruptLevelVME(lvl))
                {
                    printf("Failed to enable interrupt level %d\n",lvl);
                    return;
                }
            }

        }

        break;
  default:
        break;
    }
}

static const iocshArg mrmEvrSetupPCIArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrSetupPCIArg1 = { "Bus number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg2 = { "Device number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg3 = { "Function number",iocshArgInt};
static const iocshArg * const mrmEvrSetupPCIArgs[4] =
{&mrmEvrSetupPCIArg0,&mrmEvrSetupPCIArg1,&mrmEvrSetupPCIArg2,&mrmEvrSetupPCIArg3};
static const iocshFuncDef mrmEvrSetupPCIFuncDef =
    {"mrmEvrSetupPCI",4,mrmEvrSetupPCIArgs};
static void mrmEvrSetupPCICallFunc(const iocshArgBuf *args)
{
    mrmEvrSetupPCI(args[0].sval,args[1].ival,args[2].ival,args[3].ival);
}

extern "C"
void
mrmEvrSetupVME(const char* id,int slot,int base,int level, int vector)
{
try {
    std::ostringstream position;
    if(mrf::Object::getObject(id)){
        printf("ID %s already in use\n",id);
        return;
    }

    struct VMECSRID info;

    volatile unsigned char* csr=devCSRTestSlot(vmeevrs,slot,&info);
    if(!csr){
        printf("No EVR in slot %d\n",slot);
        return;
    }

    printf("Setting up EVR in VME Slot %d\n",slot);
    position<<"Slot #"<<slot;

    printf("Found vendor: %08x board: %08x rev.: %08x\n",
           info.vendor, info.board, info.revision);

    // Set base address

  /* Use function 0 for 16-bit addressing (length 0x00800 bytes)
   * and function 1 for 24-bit addressing (length 0x10000 bytes)
   * and function 2 for 32-bit addressing (length 0x40000 bytes)
   *
   * All expose the same registers, but not all registers are
   * visible through functions 0 or 1.
   */

    CSRSetBase(csr, 2, base, VME_AM_EXT_SUP_DATA);
    
    {
        epicsUInt32 temp=CSRRead32((csr) + CSR_FN_ADER(2));

        if(temp != CSRADER((epicsUInt32)base,VME_AM_EXT_SUP_DATA)) {
            printf("Failed to set CSR Base address in ADER2.  Check VME bus and card firmware version.\n");
            return;
        }
    }

    volatile unsigned char* evr;
    char *Description = allocSNPrintf(40, "EVR-%d '%s' slot %d",
                                      info.board & MRF_BID_SERIES_MASK,
                                      id, slot);

    if(devRegisterAddress(Description, atVMEA32, base, EVR_REGMAP_SIZE, (volatile void**)(void *)&evr))
    {
        printf("Failed to map address %08x\n",base);
        return;
    }

    epicsUInt32 junk;
    if(devReadProbe(sizeof(junk), (volatile void*)(evr+U32_FWVersion), (void*)&junk)) {
        printf("Failed to read from MRM registers (but could read CSR registers)\n");
        return;
    }

    checkVersion(evr, 4, 5);

    // Read offset from start of CSR to start of user (card specific) CSR.
    size_t user_offset=CSRRead24(csr+CR_BEG_UCSR);
    // Currently that value read from the UCSR pointer is
    // actually little endian.
    user_offset= (( user_offset & 0x00ff0000 ) >> 16 ) |
                 (( user_offset & 0x0000ff00 )       ) |
                 (( user_offset & 0x000000ff ) << 16 );
    volatile unsigned char* user_csr=csr+user_offset;

    NAT_WRITE32(evr, IRQEnable, 0); // Disable interrupts

    EVRMRM *receiver=new EVRMRM(id,position.str(),evr,EVR_REGMAP_SIZE);

    if(level>0 && vector>=0) {
        CSRWrite8(user_csr+UCSR_IRQ_LEVEL,  level&0x7);
        CSRWrite8(user_csr+UCSR_IRQ_VECTOR, vector&0xff);

        printf("Using IRQ %d:%2d\n",
               CSRRead8(user_csr+UCSR_IRQ_LEVEL),
               CSRRead8(user_csr+UCSR_IRQ_VECTOR)
               );

        // Acknowledge missed interrupts
        //TODO: This avoids a spurious FIFO Full
        NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

        level&=0x7;
        // VME IRQ level will be enabled later during iocInit()
        vme_level_mask|=1<<(level-1);

        if(devConnectInterruptVME(vector&0xff, &EVRMRM::isr, receiver))
        {
            printf("Failed to connection VME IRQ %d\n",vector&0xff);
            delete receiver;
            return;
        }

        // Interrupts will be enabled during iocInit()
    }

} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
}
}

static const iocshArg mrmEvrSetupVMEArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrSetupVMEArg1 = { "Bus number",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg2 = { "A32 base address",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg3 = { "IRQ Level 1-7 (0 - disable)",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg4 = { "IRQ vector 0-255",iocshArgInt};
static const iocshArg * const mrmEvrSetupVMEArgs[5] =
{&mrmEvrSetupVMEArg0,&mrmEvrSetupVMEArg1,&mrmEvrSetupVMEArg2,&mrmEvrSetupVMEArg3,&mrmEvrSetupVMEArg4};
static const iocshFuncDef mrmEvrSetupVMEFuncDef =
    {"mrmEvrSetupVME",5,mrmEvrSetupVMEArgs};
static void mrmEvrSetupVMECallFunc(const iocshArgBuf *args)
{
    mrmEvrSetupVME(args[0].sval,args[1].ival,args[2].ival,args[3].ival,args[4].ival);
}

extern "C"
void
mrmEvrDumpMap(const char* id,int evt,int ram)
{
try {
    mrf::Object *obj=mrf::Object::getObject(id);
    if(!obj)
        throw std::runtime_error("Object not found");
    EVRMRM *card=dynamic_cast<EVRMRM*>(obj);
    if(!card)
        throw std::runtime_error("Not a MRM EVR");

    printf("Print ram #%d\n",ram);
    if(evt>=0){
        // Print a single event
        printRamEvt(card,evt,ram);
        return;
    }
    for(evt=0;evt<=255;evt++){
        printRamEvt(card,evt,ram);
    }
} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
}
}

static const iocshArg mrmEvrDumpMapArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrDumpMapArg1 = { "Event code",iocshArgInt};
static const iocshArg mrmEvrDumpMapArg2 = { "Mapping select 0 or 1",iocshArgInt};
static const iocshArg * const mrmEvrDumpMapArgs[3] =
{&mrmEvrDumpMapArg0,&mrmEvrDumpMapArg1,&mrmEvrDumpMapArg2};
static const iocshFuncDef mrmEvrDumpMapFuncDef =
    {"mrmEvrDumpMap",3,mrmEvrDumpMapArgs};
static void mrmEvrDumpMapCallFunc(const iocshArgBuf *args)
{
    mrmEvrDumpMap(args[0].sval,args[1].ival,args[2].ival);
}

/** @brief Setup Event forwarding to downstream link
 *
 * Control which events will be forwarded to the downstream event link
 * when they are received on the upstream link.
 * Useful when daisy chaining EVRs.
 *
 * When invoked with the second argument as NULL or "" the current
 * forward mapping is printed.
 *
 * The second argument to this function is a comma seperated list of
 * event numbers and/or the special token 'all'.  If a token is prefixed
 * with '-' then the mapping is cleared, otherwise it is set.
 *
 * After a cold boot, no events are forwarded until/unless mrmrEvrForward
 * is called.
 *
 @code
   > mrmEvrForward("EVR1") # Prints current mappings
   Events forwarded: ...
   > mrmEvrForward("EVR1", "-all") # Clear all forward mappings
   # Clear all forward mappings except timestamp transmission
   > mrmEvrForward("EVR1", "-all, 0x70, 0x71, 0x7A, 0x7D")
   # Forward all except 42
   > mrmEvrForward("EVR1", "all, -42")
 @endcode
 *
 @param id EVR identifier
 @param events A string with a comma seperated list of event specifiers
 */
extern "C"
void
mrmEvrForward(const char* id, const char* events_iocsh)
{
    char *events=events_iocsh ? epicsStrDup(events_iocsh) : 0;
try {
    mrf::Object *obj=mrf::Object::getObject(id);
    if(!obj)
        throw std::runtime_error("Object not found");
    EVRMRM *card=dynamic_cast<EVRMRM*>(obj);
    if(!card)
        throw std::runtime_error("Not a MRM EVR");

    if(!events || strlen(events)==0) {
        // Just print current mappings
        printf("Events forwarded: ");
        for(unsigned int i=1; i<256; i++) {
            if(card->specialMapped(i, ActionEvtFwd)) {
                printf("%d ", i);
            }
        }
        printf("\n");
        free(events);
        return;
    }

    // update mappings

    const char sep[]=", ";
    char *save=0;

    for(char *tok=strtok_r(events, sep, &save);
        tok!=NULL;
        tok = strtok_r(0, sep, &save)
        )
    {
        if(strcmp(tok, "-all")==0) {
            for(unsigned int i=1; i<256; i++)
                card->specialSetMap(i, ActionEvtFwd, false);

        } else if(strcmp(tok, "all")==0) {
            for(unsigned int i=1; i<256; i++)
                card->specialSetMap(i, ActionEvtFwd, true);

        } else {
            char *end=0;
            long e=strtol(tok, &end, 0);
            if(*end || e==LONG_MAX || e==LONG_MIN) {
                printf("Unable to parse event spec '%s'\n", tok);
            } else if(e>255 || e<-255 || e==0) {
                printf("Invalid event %ld\n", e);
            } else if(e>0) {
                card->specialSetMap(e, ActionEvtFwd, true);
            } else if(e<0) {
                card->specialSetMap(-e, ActionEvtFwd, false);
            }

        }
    }


    free(events);
} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
    free(events);
}
}

static const iocshArg mrmEvrForwardArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrForwardArg1 = { "Event spec string",iocshArgString};
static const iocshArg * const mrmEvrForwardArgs[2] =
{&mrmEvrForwardArg0,&mrmEvrForwardArg1};
static const iocshFuncDef mrmEvrForwardFuncDef =
    {"mrmEvrForward",2,mrmEvrForwardArgs};
static void mrmEvrForwardCallFunc(const iocshArgBuf *args)
{
    mrmEvrForward(args[0].sval,args[1].sval);
}

static
void mrmsetupreg()
{
    initHookRegister(&inithooks);
    iocshRegister(&mrmEvrSetupPCIFuncDef,mrmEvrSetupPCICallFunc);
    iocshRegister(&mrmEvrSetupVMEFuncDef,mrmEvrSetupVMECallFunc);
    iocshRegister(&mrmEvrDumpMapFuncDef,mrmEvrDumpMapCallFunc);
    iocshRegister(&mrmEvrForwardFuncDef,mrmEvrForwardCallFunc);
}

epicsExportRegistrar(mrmsetupreg);

static
drvet drvEvrMrm = {
    2,
    (DRVSUPFUN)report,
    NULL
};

epicsExportAddress (drvet, drvEvrMrm);
