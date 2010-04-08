
#include "drvemOutput.h"

#include <mrfIoOps.h>
#include "evrRegMap.h"

#include <stdexcept>

MRMOutput::MRMOutput(volatile unsigned char *b)
    :base(b)
{
}

MRMOutput::~MRMOutput()
{
}

epicsUInt32
MRMOutput::source() const
{
    return nat_ioread16(base);
}

void
MRMOutput::setSource(epicsUInt32 v)
{
    if( ! ( (v<=63 && v>=62) ||
            (v<=42 && v>=32) ||
            (v<=9) )
    )
        throw std::out_of_range("Mapping code is out of range");

    nat_iowrite16(base, v);
}

const char*
MRMOutput::sourceName(epicsUInt32 id) const
{
    switch(id){
    case 63: return "Force High";
    case 62: return "Force Low";
    // 43 -> 61 Reserved
    case 42: return "Prescaler (Divider) 2";
    case 41: return "Prescaler (Divider) 1";
    case 40: return "Prescaler (Divider) 0";
    case 39: return "Distributed Bus Bit 7";
    case 38: return "Distributed Bus Bit 6";
    case 37: return "Distributed Bus Bit 5";
    case 36: return "Distributed Bus Bit 4";
    case 35: return "Distributed Bus Bit 3";
    case 34: return "Distributed Bus Bit 2";
    case 33: return "Distributed Bus Bit 1";
    case 32: return "Distributed Bus Bit 0";
    // 10 -> 31 Reserved
    case 9: return "Pulse generator 9";
    case 8: return "Pulse generator 8";
    case 7: return "Pulse generator 7";
    case 6: return "Pulse generator 6";
    case 5: return "Pulse generator 5";
    case 4: return "Pulse generator 4";
    case 3: return "Pulse generator 3";
    case 2: return "Pulse generator 2";
    case 1: return "Pulse generator 1";
    case 0: return "Pulse generator 0";
    default: return "Invalid";
    }
}
