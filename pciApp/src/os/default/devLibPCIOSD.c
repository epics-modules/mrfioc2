#include <epicsExport.h>

/* Only exists so that this registrar will be present for all targets */
void devLibPCIRegisterBaseDefault(void)
{
}
epicsExportRegistrar(devLibPCIRegisterBaseDefault);
