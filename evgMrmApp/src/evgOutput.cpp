#include "evgOutput.h"

#include <iostream>
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <errlog.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgOutput::evgOutput(const epicsUInt32 num, const OutputType type,
                     volatile epicsUInt8* const pReg):
m_num(num),
m_type(type),
m_pReg(pReg) {
    switch(m_type) {        
        case(FP_Output):
            if(m_num >= evgNumFpOut)
                throw std::runtime_error("EVG Front panel output ID out of range");
            break;

        case(Univ_Output):
            if(m_num >= evgNumUnivOut)
                throw std::runtime_error("EVG Universal output ID out of range");
            break;

        default:
            throw std::runtime_error("Wrong EVG Output type");
    }
}

evgOutput::~evgOutput() {
}

epicsStatus
evgOutput::setOutMap(epicsUInt16 map) {
    epicsStatus ret = OK;
    switch(m_type) {
        case(FP_Output):
            WRITE16(m_pReg, FPOutMap(m_num), map);
            ret = OK;
            break;

        case(Univ_Output):
            WRITE16(m_pReg, UnivOutMap(m_num), map);
            ret = OK;
            break;

        default:
            throw std::runtime_error("Wrong EVG Ouput type");
    }
    
    return ret;
}
