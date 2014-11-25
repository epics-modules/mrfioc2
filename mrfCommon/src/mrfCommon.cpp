#include <stdlib.h>
#include <stdexcept>

#include <epicsStdio.h>

#include "mrfCommon.h"

epicsUInt32 roundToUInt(double val, epicsUInt32 max)
{
    if(!isfinite(val))
        throw std::range_error("Value not finite");

    else if(val<0)
        throw std::range_error("Negative value not allowed");

    val+=0.5;

    if(val>(double)max)
        throw std::range_error("Value too large");

    return (epicsUInt32)val;
}

char *allocSNPrintf(size_t N, const char *fmt, ...)
{
    char *mem = (char*)calloc(1, N);
    if(!mem)
        throw std::bad_alloc();

    va_list args;

    va_start(args, fmt);

    epicsVsnprintf(mem, N, fmt, args);

    va_end(args);

    mem[N-1] = '\0';

    return mem;
}
