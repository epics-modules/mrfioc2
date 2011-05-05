
#include "mrf/databuf.h"

OBJECT_BEGIN(dataBufRx) {
    OBJECT_PROP2("Enable", &dataBufRx::dataRxEnabled, &dataBufRx::dataRxEnable);
} OBJECT_END(dataBufRx)
