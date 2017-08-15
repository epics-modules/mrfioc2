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
public:
    explicit CFIFlash(const SPIDevice& dev);
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
                    sectorSize, //!< SECTOR ERASE (0xd8) size.  A power of 2
                    pageSize;   //!< PAGE PROGRAM (0x02) size.  A power of 2

        std::vector<epicsUInt8> SN;
    };

    //! execute command 0x9f JEDEC-ID read
    void readID(ID *id);

    inline bool writable() { check(); return info.pageSize>0 && info.sectorSize>0; }
    // worst case alignment for start address
    inline epicsUInt32 alignement() { check(); return (info.pageSize-1)|(info.sectorSize-1); }
    // optimal block size of write operation
    inline epicsUInt32 blockSize() { return alignement()+1u; }

    void read(epicsUInt32 start, epicsUInt32 count, epicsUInt8 *in);
    inline void read(epicsUInt32 start, std::vector<epicsUInt8>& in)
    { read(start, in.size(), &in[0]); }

    void write(epicsUInt32 start, epicsUInt32 count, const epicsUInt8 *out, bool strict = true);
    void write(epicsUInt32 start, const std::vector<epicsUInt8>& out, bool strict = true)
    { write(start, out.size(), &out[0], strict); }

    void erase(epicsUInt32 start, epicsUInt32 count, bool strict = true);
private:
    SPIDevice dev;
    bool haveinfo;
    ID info;

    void check();
    unsigned status();

    void writeEnable(bool e);
    void busyWait(double timeout, unsigned n=10);

    struct WriteEnabler
    {
        CFIFlash& dev;
        explicit WriteEnabler(CFIFlash& dev) : dev(dev)
        {}
        void enable()
        { dev.writeEnable(true); }
        ~WriteEnabler()
        { dev.writeEnable(false); }
    };
};

} // namespace mrf

#endif // FLASH_H
