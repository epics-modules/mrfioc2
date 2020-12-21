#include "epicsExport.h"
#include "mrf/databuf.h"


OBJECT_BEGIN(dataBufRx) {
    OBJECT_PROP2("Enable", &dataBufRx::dataRxEnabled, &dataBufRx::dataRxEnable);
} OBJECT_END(dataBufRx)

OBJECT_BEGIN(dataBufTx) {
    OBJECT_PROP2("Enable", &dataBufTx::dataTxEnabled, &dataBufTx::dataTxEnable);
    OBJECT_PROP1("Ready to send", &dataBufTx::dataRTS);
    OBJECT_PROP1("Max length", &dataBufTx::lenMax);
} OBJECT_END(dataBufTx)

dataBufTx::dataBufTx(const std::string& n) : mrf::ObjectInst<dataBufTx>(n)
{
    OBJECT_INIT;
}

dataBufRx::dataBufRx(const std::string& n) : mrf::ObjectInst<dataBufRx>(n)
{
    OBJECT_INIT;
}

// definition for pure virtual is required in most cases (apparently not MSVC w/ static linking?)
// If bottom 2 lines are removed, MSVC does not report warning C4273
#if !defined(_WIN32) || (defined(_WIN32) && defined(_DLL))
dataBufTx::~dataBufTx() {}
dataBufRx::~dataBufRx() {}
#endif
