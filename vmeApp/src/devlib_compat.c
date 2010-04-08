
#define epicsExportSharedSymbols
#include <shareLib.h>

/*
 * Most devlib function go through an indirection table with a null
 * implimentation provided for systems which doen't impliment some
 * functionality.  However, the functions below don't use this table.
 * Provide a null implimentation so that devLib applications will
 * still link on these systems.
 */

epicsShareFunc long devEnableInterruptLevelVME (unsigned vectorNumber)
{
  return -1;
}

epicsShareFunc long devConnectInterruptVME (
                        unsigned vectorNumber,
                        void (*pFunction)(void *),
                        void  *parameter)
{
  return -1;
}

epicsShareFunc long devDisconnectInterruptVME (
			unsigned vectorNumber,
			void (*pFunction)(void *))
{
  return -1;
}

epicsShareFunc int  devInterruptInUseVME (unsigned vectorNumber)
{
   return -1;
}
