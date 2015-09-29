/*************************************************************************\
* Copyright (c) 2014 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#include <stdio.h>

// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include <epicsExport.h>
#include "mrf/object.h"
#include "sfp.h"

#include "mrfCommonIO.h"

#include "sfpinfo.h"

epicsInt16 SFP::read16(unsigned int offset) const
{
    epicsUInt16 val = buffer[offset];
    val<<=8;
    val |= buffer[offset+1];
    return val;
}

SFP::SFP(const std::string &n, volatile unsigned char *reg)
    :mrf::ObjectInst<SFP>(n)
    ,base(reg)
    ,buffer(SFPMEM_SIZE)
    ,valid(false)
{
    updateNow();

    /* Check for SFP with LC connector */
    if(valid)
        fprintf(stderr, "Found SFP EEPROM\n");
    else
        fprintf(stderr, "Found SFP Strangeness %02x%02x%02x%02x\n",
                buffer[0],buffer[1],buffer[2],buffer[3]);
}

SFP::~SFP() {}

void SFP::updateNow(bool)
{
    /* read I/O 4 bytes at a time to preserve endianness
     * for both PCI and VME
     */
    epicsUInt32* p32=(epicsUInt32*)&buffer[0];

    for(unsigned int i=0; i<SFPMEM_SIZE/4; i++)
        p32[i] = be_ioread32(base+ i*4);

    valid = buffer[0]==3 && buffer[2]==7;
}

double SFP::linkSpeed() const
{
    if(!valid){
        return -1;
    }
    return buffer[SFP_linkrate] * 100.0; // Gives MBits/s
}

double SFP::temperature() const
{
    if(!valid){
        return -40;
    }
    return read16(SFP_temp) / 256.0; // Gives degrees C
}

double SFP::powerTX() const
{
    if(!valid){
        return -1e-6;
    }
    return read16(SFP_tx_pwr) * 0.1e-6; // Gives Watts
}

double SFP::powerRX() const
{
    if(!valid){
        return -1e-6;
    }
    return read16(SFP_rx_pwr) * 0.1e-6; // Gives Watts
}

static const char nomod[] = "<No Module>";

std::string SFP::vendorName() const
{
    if(!valid)
        return std::string(nomod);
    buffer_t::const_iterator it=buffer.begin()+SFP_vendor_name;
    return std::string(it, it+16);
}

std::string SFP::vendorPart() const
{
    if(!valid)
        return std::string(nomod);
    buffer_t::const_iterator it=buffer.begin()+SFP_part_num;
    return std::string(it, it+16);
}

std::string SFP::vendorRev() const
{
    if(!valid)
        return std::string(nomod);
    buffer_t::const_iterator it=buffer.begin()+SFP_part_rev;
    return std::string(it, it+4);
}

std::string SFP::serial() const
{
    if(!valid)
        return std::string(nomod);
    buffer_t::const_iterator it=buffer.begin()+SFP_serial;
    return std::string(it, it+16);
}

std::string SFP::manuDate() const
{
    if(!valid)
        return std::string(nomod);
    std::string ret("20XX/XX");
    ret[2]=buffer[SFP_man_date];
    ret[3]=buffer[SFP_man_date+1];
    ret[5]=buffer[SFP_man_date+2];
    ret[6]=buffer[SFP_man_date+3];
    return ret;
}

void SFP::report() const
{
    printf("SFP tranceiver information\n"
            " Temp: %.1f C\n"
            " Link: %.1f MBits/s\n"
            " Tx Power: %.1f uW\n"
            " Rx Power: %.1f uW\n",
            temperature(),
            linkSpeed(),
            powerTX()*1e6,
            powerRX()*1e6);
    printf(" Vendor:%s\n Model: %s\n Rev: %s\n Manufacture date: %s\n Serial: %s\n",
           vendorName().c_str(),
           vendorPart().c_str(),
           vendorRev().c_str(),
           manuDate().c_str(),
           serial().c_str());
}

OBJECT_BEGIN(SFP) {

    OBJECT_PROP2("Update", &SFP::junk, &SFP::updateNow);

    OBJECT_PROP1("Vendor", &SFP::vendorName);
    OBJECT_PROP1("Part", &SFP::vendorPart);
    OBJECT_PROP1("Rev", &SFP::vendorRev);
    OBJECT_PROP1("Serial", &SFP::serial);
    OBJECT_PROP1("Date", &SFP::manuDate);

    OBJECT_PROP1("Temperature", &SFP::temperature);
    OBJECT_PROP1("Link Speed", &SFP::linkSpeed);
    OBJECT_PROP1("Power TX", &SFP::powerTX);
    OBJECT_PROP1("Power RX", &SFP::powerRX);

} OBJECT_END(SFP)
