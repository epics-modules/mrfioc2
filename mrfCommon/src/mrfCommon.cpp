
#include <stdexcept>

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
