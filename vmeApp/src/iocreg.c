
#include <iocsh.h>
#include <epicsExport.h>

#include "devcsr.h"

/* vmecsrprint */
static const iocshArg vmecsrprintArg0 = { "slot (1-31)",iocshArgInt};
static const iocshArg vmecsrprintArg1 = { "verbosity (>=0)",iocshArgInt};
static const iocshArg * const vmecsrprintArgs[2] =
    {&vmecsrprintArg0,&vmecsrprintArg1};
static const iocshFuncDef vmecsrprintFuncDef =
    {"vmecsrprint",2,vmecsrprintArgs};

static
void vmecsrprintCall(const iocshArgBuf *arg)
{
  vmecsrprint(arg[0].ival,arg[1].ival);
}

/* vmecsrdump */
static const iocshArg vmecsrdumpArg0 = { "verbosity (>=0)",iocshArgInt};
static const iocshArg * const vmecsrdumpArgs[1] =
    {&vmecsrdumpArg0};
static const iocshFuncDef vmecsrdumpFuncDef =
    {"vmecsrdump",1,vmecsrdumpArgs};

static
void vmecsrdumpCall(const iocshArgBuf *arg)
{
  vmecsrdump(arg[0].ival);
}

void vmecsr(void)
{
    iocshRegister(&vmecsrprintFuncDef,vmecsrprintCall);
    iocshRegister(&vmecsrdumpFuncDef,vmecsrdumpCall);
}
epicsExportRegistrar(vmecsr);
