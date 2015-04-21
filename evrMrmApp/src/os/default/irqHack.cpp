#include <epicsExport.h>
static
void
registerISRHack(void)
{
    // Unneeded
}
extern "C"{
 epicsExportRegistrar(registerISRHack);
}
