
#include <iocsh.h>
#include <epicsExport.h>

#include "cardmap.h"
#include "errlog.h"
#include "evrnull.h"

void addEVRNull(int id, const char* name)
{
try{

  EVRNull *card=new EVRNull(name);

  storeEVR(id,card);

} catch(std::exception& e) {
  errMessage(errlogFatal,e.what());
}
}

static const iocshArg addEVRNullArg0 = { "ID number",iocshArgInt};
static const iocshArg addEVRNullArg1 = { "Name string",iocshArgString};
static const iocshArg * const addEVRNullArgs[2] =
{
    &addEVRNullArg0,&addEVRNullArg1
};
static const iocshFuncDef addEVRNullFuncDef =
    {"addEVRNull",2,addEVRNullArgs};
static void addEVRNullCallFunc(const iocshArgBuf *args)
{
    addEVRNull(args[0].ival,args[1].sval);
}

void nullreg()
{
  iocshRegister(&addEVRNullFuncDef,addEVRNullCallFunc);
}

extern "C" {
epicsExportRegistrar(nullreg);
}
