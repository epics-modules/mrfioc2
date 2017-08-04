/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef FLASH_H
#define FLASH_H

#include <vector>

#include <epicsTypes.h>

namespace mrf {

struct SPIDevice;

//! Handling for Common Flash Interfafce compliant chips
class CFIFlash
{
    SPIDevice dev;
public:
    CFIFlash(const SPIDevice& dev);
    ~CFIFlash();

    struct ID {
        /* vendors
         *   0x20 - Micron
         */
        epicsUInt8 vendor,
                   dev_type,
                   dev_id;

        const char *vendorName;
        epicsUInt32 capacity, //!< total capacity in bytes
                    sectorSize, //!< SECTOR ERASE (0xd8) size
                    pageSize;   //!< PAGE PROGRAM (0x02) size

        std::vector<epicsUInt8> SN;
    };

    //! execute command 0x9f JEDEC-ID read
    void readID(ID *id);

    void read(epicsUInt32 start, epicsUInt32 count, epicsUInt8 *in);

    void write(epicsUInt32 start, epicsUInt32 count, const epicsUInt8 *in);

    void erase(epicsUInt32 start, epicsUInt32 count);

private:
    void check();
};

} // namespace mrf

#endif // FLASH_H
