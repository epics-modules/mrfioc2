#include "evgOutput.h"

#include <iostream>
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <errlog.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgOutput::evgOutput(const epicsUInt32 id, volatile epicsUInt8* const pReg,
                                         const OutputType type):
m_id(id),
m_pReg(pReg),
m_type(type) {
    switch(m_type) {        
        case(FP_Output):
            if(m_id >= evgNumFpOut)
                throw std::runtime_error("EVG Front panel output ID out of range");
            break;

        case(Univ_Output):
            if(m_id >= evgNumUnivOut)
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
            WRITE16(m_pReg, FPOutMap(m_id), map);
            ret = OK;
            break;

        case(Univ_Output):
            WRITE16(m_pReg, UnivOutMap(m_id), map);
            ret = OK;
            break;

        default:
            throw std::runtime_error("Wrong EVG Ouput type");
    }
    
    return ret;
}
