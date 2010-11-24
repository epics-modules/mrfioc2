
#include "drvemIocsh.h"

#include <cstdio>
#include <cstring>

#include <stdexcept>
#include <map>

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

#include <evrmap.h>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

extern "C" {
int evrmrmVerb=1;
epicsExportAddress(int,evrmrmVerb);
}

/* Bit mask used to communicate which VME interrupt levels
 * are used.  Bits are set by mrmEvrSetupEVR().  Levels are
 * enabled later during iocInit.
 */
static epicsUInt8 vme_level_mask = 0;

#define DBG evrmrmVerb

static const epicsPCIID mrmevrs[] = {
   DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                              PCI_DEVICE_ID_MRF_PMCEVR_230, PCI_VENDOR_ID_MRF)
  ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                              PCI_DEVICE_ID_MRF_PXIEVR_230, PCI_VENDOR_ID_MRF)
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
  char label[10];
  epicsUInt32 offset;
  int rsize;
} printreg[] = {
#define REGINFO(label, name, size) {label, U##size##_##name, size}
REGINFO("Version", FWVersion, 32),
REGINFO("Control", Control, 32),
REGINFO("Status",  Status, 32),
REGINFO("IRQ Flag",IRQFlag, 32),
REGINFO("IRQ Ena", IRQEnable, 32),
REGINFO("IRQPlsmap",IRQPulseMap, 16),
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
REGINFO("FP0MAP",OutputMapFP(0),16),
REGINFO("FPU0MAP",OutputMapFPUniv(0),16),
REGINFO("RB0MAP",OutputMapRB(0),16),
REGINFO("FPIN0CFG",InputMapFPCfg(0),8),
REGINFO("FPIN0DBs",InputMapFPDBus(0),8),
REGINFO("FPIN0Bck",InputMapFPBEvt(0),8),
REGINFO("FPIN0Ext",InputMapFPEEvt(0),8),
REGINFO("CML4Low",OutputCMLLow(0),32),
REGINFO("CML4Rise",OutputCMLRise(0),32),
REGINFO("CML4High",OutputCMLHigh(0),32),
REGINFO("CML4Fall",OutputCMLFall(0),32),
REGINFO("CML4Ena",OutputCMLEna(0),32),
REGINFO("CML4High",OutputCMLCountHigh(0),16),
REGINFO("CML4Low",OutputCMLCountLow(0),16),
REGINFO("CML4Len",OutputCMLPatLength(0),32),
REGINFO("CML4Pat0",OutputCMLPat(0,0),32),
REGINFO("CML4Pat1",OutputCMLPat(0,1),32),
REGINFO("TXBuf0",DataTx(0),8)
#undef REGINFO
};

static
void
printregisters(volatile epicsUInt8 *evr)
{
    size_t reg;

    printf("EVR\n");
    for(reg=0; reg<NELEMENTS(printreg); reg++){

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
bool reportCard(int level,short id,EVRMRM& evr)
{
  printf("--- EVR %d ---\n",id);

  printf("Model: %08x  Version: %08x\n",evr.model(),evr.version());

  printf("Clock: %.6f MHz\n",evr.clock()*1e-6);

  if(level>=2){
    printregisters(evr.base);
  }

  return true;
}

static
long report(int level)
{
  printf("=== Begin MRF EVR support ===\n");
  evrmap.visit(level,&reportCard);
  printf("=== End MRF EVR support ===\n");
  return 0;
}

#ifdef linux
struct plxholder {
    volatile void* plx;
    EVRMRM *evr;
    plxholder(EVRMRM* a,volatile void* b) : plx(b), evr(a) {}
};

static
void PLXISR(void *raw)
{
    plxholder *card=static_cast<plxholder*>(raw);

    EVRMRM::isr(static_cast<void*>(card->evr));

    LE_WRITE16(card->plx, INTCSR, INTCSR_INT1_Enable|
                                  INTCSR_INT1_Polarity|
                                  INTCSR_PCI_Enable);
}

#endif


extern "C"
void
mrmEvrSetupPCI(int id,int b,int d,int f)
{
try {
  if(cardIdInUse(id)){
    printf("ID %d already in use\n",id);
    return;
  }

  const epicsPCIDevice *cur=0;
  if( devPCIFindBDF(mrmevrs,b,d,f,&cur,0) ){
    printf("PCI Device not found\n");
    return;
  }

  printf("Device %u  %u:%u.%u\n",id,cur->bus,cur->device,cur->function);
  printf("Using IRQ %u\n",cur->irq);

  volatile epicsUInt8 *plx, *evr;

  if( devPCIToLocalAddr(cur,0,(volatile void**)(void *)&plx,0) ||
      devPCIToLocalAddr(cur,2,(volatile void**)(void *)&evr,0))
  {
      printf("Failed to map BARs 0 and 2\n");
      return;
  }
  if(!plx || !evr){
      printf("BARs mapped to zero? (%08lx,%08lx)\n",
              (unsigned long)plx,(unsigned long)evr);
      return;
  }

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

  /* Enable active high interrupt1 through the PLX to the PCI bus.
   */
  LE_WRITE16(plx, INTCSR, INTCSR_INT1_Enable|
                            INTCSR_INT1_Polarity|
                            INTCSR_PCI_Enable);

  // Acknowledge missed interrupts
  //TODO: This avoids a spurious FIFO Full
  NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

  // Install ISR

  EVRMRM *receiver=new EVRMRM(id,evr);

  void (*pciisr)(void *);
  void *arg=receiver;
#ifdef linux
  pciisr=&PLXISR;
  arg=new plxholder(receiver, plx);
#else
  pciisr=&EVRMRM::isr;
#endif

  if(devPCIConnectInterrupt(cur,pciisr,arg,0)){
      printf("Failed to install ISR\n");
      delete receiver;
  }else{
      // Interrupts will be enabled during iocInit()

      evrmap.store(id,*receiver);
      datatxmap.append(id,receiver->buftx);
      datarxmap.append(id,receiver->bufrx);
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
enableIRQ(int,short,EVRMRM& mrm)
{

  WRITE32(mrm.base, IRQEnable,
       IRQ_Enable
      |IRQ_RXErr    |IRQ_BufFull
      |IRQ_Heartbeat|IRQ_HWMapped
      |IRQ_Event    |IRQ_FIFOFull
  );

  return true;
}

static
bool
disableIRQ(int,short,EVRMRM& mrm)
{
    WRITE32(mrm.base, IRQEnable, 0);
    return true;
}

static
void
evrShutdown(void*)
{
    evrmap.visit(0, &disableIRQ);
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
    evrmap.visit(0, &enableIRQ);
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

static const iocshArg mrmEvrSetupPCIArg0 = { "ID number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg1 = { "Bus number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg2 = { "Device number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg3 = { "Function number",iocshArgInt};
static const iocshArg * const mrmEvrSetupPCIArgs[4] =
{&mrmEvrSetupPCIArg0,&mrmEvrSetupPCIArg1,&mrmEvrSetupPCIArg2,&mrmEvrSetupPCIArg3};
static const iocshFuncDef mrmEvrSetupPCIFuncDef =
    {"mrmEvrSetupPCI",4,mrmEvrSetupPCIArgs};
static void mrmEvrSetupPCICallFunc(const iocshArgBuf *args)
{
    mrmEvrSetupPCI(args[0].ival,args[1].ival,args[2].ival,args[3].ival);
}

extern "C"
void
mrmEvrSetupVME(int id,int slot,int base,int level, int vector)
{
try {
  if(cardIdInUse(id)){
    printf("ID %d already in use\n",id);
    return;
  }

  struct VMECSRID info;

  volatile unsigned char* csr=devCSRTestSlot(vmeevrs,slot,&info);
  if(!csr){
    printf("No EVR in slot %d\n",slot);
    return;
  }

  printf("Setting up EVR in VME Slot %d\n",slot);

  printf("Found vendor: %08x board: %08x rev.: %08x\n",
    info.vendor, info.board, info.revision);

  // Set base address

  /* Use function 0 for 16-bit addressing (length 0x00800 bytes)
   * and function 1 for 24-bit addressing (length 0x10000 bytes)
   * and function 2 for 32-bit addressing (length 0x20000 bytes)
   *
   * All expose the same registers, but not all registers are
   * visible through functions 0 or 1.
   */

  CSRSetBase(csr, 2, base, VME_AM_EXT_SUP_DATA);

  volatile unsigned char* evr;

  if(devRegisterAddress("MRF EVR", atVMEA32, base, 0x20000, (volatile void**)(void *)&evr))
  {
    printf("Failed to map address %08x\n",base);
    return;
  }

  // Read offset from start of CSR to start of user (card specific) CSR.
  size_t user_offset=CSRRead24(csr+CR_BEG_UCSR);
  // Currently that value read from the UCSR pointer is
  // actually little endian.
  user_offset= (( user_offset & 0x00ff0000 ) >> 16 ) |
               (( user_offset & 0x0000ff00 )       ) |
               (( user_offset & 0x000000ff ) << 16 );
  volatile unsigned char* user_csr=csr+user_offset;

  NAT_WRITE32(evr, IRQEnable, 0); // Disable interrupts

  EVRMRM *receiver=new EVRMRM(id,evr);

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
    // Level with be enabled later during iocInit()
    vme_level_mask|=1<<(level-1);

    if(devConnectInterruptVME(vector&0xff, &EVRMRM::isr, receiver))
    {
      printf("Failed to connection VME IRQ %d\n",vector&0xff);
      delete receiver;
      return;
    }

    // Interrupts will be enabled during iocInit()
  }

  evrmap.store(id,*receiver);
  datatxmap.append(id,receiver->buftx);
  datarxmap.append(id,receiver->bufrx);

} catch(std::exception& e) {
  printf("Error: %s\n",e.what());
}
}

static const iocshArg mrmEvrSetupVMEArg0 = { "ID number",iocshArgInt};
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
    mrmEvrSetupVME(args[0].ival,args[1].ival,args[2].ival,args[3].ival,args[4].ival);
}

extern "C"
void
mrmEvrDumpMap(int id,int evt,int ram)
{
try {
  EVRMRM *card=&evrmap.get<EVRMRM>(id);
  /* CardMap::get() throws if 'id' is invalid */
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

static const iocshArg mrmEvrDumpMapArg0 = { "ID number",iocshArgInt};
static const iocshArg mrmEvrDumpMapArg1 = { "Event code",iocshArgInt};
static const iocshArg mrmEvrDumpMapArg2 = { "Mapping select 0 or 1",iocshArgInt};
static const iocshArg * const mrmEvrDumpMapArgs[3] =
{&mrmEvrDumpMapArg0,&mrmEvrDumpMapArg1,&mrmEvrDumpMapArg2};
static const iocshFuncDef mrmEvrDumpMapFuncDef =
    {"mrmEvrDumpMap",3,mrmEvrDumpMapArgs};
static void mrmEvrDumpMapCallFunc(const iocshArgBuf *args)
{
    mrmEvrDumpMap(args[0].ival,args[1].ival,args[2].ival);
}

extern "C"
void mrmEvrSendBuf(int id, int size, int val)
{
  size&=DataTxCtrl_len_mask;
  if(size<=0) {
    printf("Size too small %d\n",size);
    return;
  }

  EVRMRM *card=&evrmap.get<EVRMRM>(id);
  if(!card){
    printf("Invalid card\n");
    return;
  }

  EVRMRM* mrm=dynamic_cast<EVRMRM*>(card);
  if(!mrm){
    printf("Card not MRM EVR\n");
    return;
  }

  // Setup
  BITSET(NAT,32,mrm->base, DataTxCtrl, DataTxCtrl_ena|DataTxCtrl_mode);

  // Zero size
  BITCLR(NAT,32,mrm->base, DataTxCtrl, DataTxCtrl_len_mask);

  for(int i=0; i<size; i++)
    WRITE8(mrm->base, DataTx(i), val+i);

  // Set size and start
  BITSET(NAT,32,mrm->base, DataTxCtrl, size|DataTxCtrl_trig);

  // Wait for completion
  epicsUInt32 ctrl;
  while( (ctrl=READ32(mrm->base, DataTxCtrl))&DataTxCtrl_run)
    epicsThreadSleep(epicsThreadSleepQuantum());

  printf("Finished %sok\n",
    ctrl&DataTxCtrl_done ? "":"not "
  );
}

static
void mrmsetupreg()
{
  initHookRegister(&inithooks);
  iocshRegister(&mrmEvrSetupPCIFuncDef,mrmEvrSetupPCICallFunc);
  iocshRegister(&mrmEvrSetupVMEFuncDef,mrmEvrSetupVMECallFunc);
  iocshRegister(&mrmEvrDumpMapFuncDef,mrmEvrDumpMapCallFunc);
}

epicsExportRegistrar(mrmsetupreg);

static
drvet drvEvrMrm = {
    2,
    (DRVSUPFUN)report,
    NULL
};

epicsExportAddress (drvet, drvEvrMrm);
