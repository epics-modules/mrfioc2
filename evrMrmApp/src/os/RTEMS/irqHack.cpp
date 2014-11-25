
#include <devLib.h>
#include <epicsInterrupt.h>

/* Ugly workaround for VME interrupt ack. timing problem
 * in the VME MRM EVRs.
 *
 * VME EVRs acknowledge interrupts by RORA (release on register access)
 * writing to the IRQFlags register.  This causes the card to stop
 * asserting its interrupt line.  Unfortunatly there is occasionally
 * a delay between when the write completes (DTACK asserted) and
 * when the interrupt line is released.  During this time the VME
 * master detects that the interrupt is still being asserted and
 * starts another IACK cycle.  However, by this time the interrupt
 * line has gone down and the card does not complete the second
 * IACK cycle resulting in a bus error.
 *
 * Unfortuanatly the RTEMS VME driver reports the bus error as an
 * interrupt on vector 0xff.  When it receives an interrupt on a
 * vector with no handler it disables the level.
 *
 * "vmeTsi148 ISR: ERROR: no handler registered (level 4) IACK 0x000000FF -- DISABLING level 4"
 *
 * Until the firmware can be fixed we install a handler for vector 0xff
 * which prints a message.
 */

static
void
nullISR(void*)
{
    epicsInterruptContextMessage("Spurious interrupt on vector 0xff");
}

extern "C"
void
installRTEMSHack(void)
{
    devConnectInterruptVME(0xff, &nullISR, NULL);
}

#include <iocsh.h>
#include <epicsExport.h>
static const iocshArg * const installRTEMSHackArgs[0] = {};

static const iocshFuncDef installRTEMSHackDef = {"installRTEMSHack", 0, installRTEMSHackArgs};
void
installRTEMSHackCall(const iocshArgBuf *args)
{
    installRTEMSHack();
}

static
void
registerISRHack(void)
{
    iocshRegister(&installRTEMSHackDef,&installRTEMSHackCall);
}
epicsExportRegistrar(registerISRHack);
