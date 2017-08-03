/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdexcept>
#include <vector>

#include <stdio.h>
#include <errno.h>

#include <iocsh.h>

#include "mrfCommon.h"
#include "mrf/spi.h"
#include "mrf/flash.h"

#include <epicsExport.h>

namespace mrf {

CFIFlash::CFIFlash(const SPIDevice &dev)
    :dev(dev)
{}

CFIFlash::~CFIFlash() {}

void
CFIFlash::readID(ID *id)
{
    epicsUInt8 cmd[4] = {0x9f, 0u, 0u, 0u},
               response[4];
    SPIInterface::Operation op = {4, cmd, response};

    {
        SPIDevice::Selector S(dev);
        dev.interface()->cycles(1, &op);
    }

    id->vendor = response[1];
    id->dev_type = response[2];
    id->dev_id = response[3];

    id->vendorName = "<Unknown>";
    id->capacity = id->sectorSize = id->pageSize = 0u;

    if(id->vendor==0x20) { // Micron
        id->vendorName = "Micron";
        switch(id->dev_type) {
        case 0xba: // mt25q
        case 0xbb:
            id->capacity = 2u<<(id->dev_id+2);
            id->sectorSize = 64*1024u;
            id->pageSize = 256;
            break;
        }
    }
}

void CFIFlash::read(epicsUInt32 start, epicsUInt32 count, epicsUInt8 *data)
{
    check();

    epicsUInt8 cmd[5];

    // as we don't control clock speed, assume support for "fast" read
    // w/ dummy byte (wait state) after 24-bit address
    cmd[0] = 0x0b;
    cmd[1] = (start>>16)&0xff;
    cmd[2] = (start>> 8)&0xff;
    cmd[3] = (start>> 0)&0xff;
    cmd[4] = 0; // dummy

    SPIInterface::Operation ops[2];

    ops[0].ncycles = 5;
    ops[0].out = NULL;
    ops[0].in = cmd;

    ops[1].ncycles = count;
    ops[1].out = data;
    ops[1].in = NULL;

    {
        SPIDevice::Selector S(dev);
        dev.interface()->cycles(2, ops);
    }
}

void CFIFlash::write(epicsUInt32 start, epicsUInt32 count, const epicsUInt8 *data)
{}

void CFIFlash::erase(epicsUInt32 start, epicsUInt32 count)
{}

void CFIFlash::check()
{
    ID id;

    readID(&id);

    if(id.vendor==0xff)
        throw std::runtime_error("Invalid Flash vendor ID");
}

} // namespace mrf

extern "C" {
void flashinfo(const char *name)
{
    try {
        mrf::SPIDevice dev;
        if(!mrf::SPIDevice::lookupDev(name, &dev)) {
            printf("No such device");
            return;
        }

        mrf::CFIFlash mem(dev);

        mrf::CFIFlash::ID info;

        mem.readID(&info);

        printf("Vendor: %02x (%s)\nDevice: %02x\nID: %02x\n",
               info.vendor, info.vendorName, info.dev_type, info.dev_id);
        if(info.capacity)
            printf("Capacity: 0x%x\n", (unsigned)info.capacity);
        if(info.sectorSize)
            printf("Sector: 0x%x\n", (unsigned)info.sectorSize);
        if(info.pageSize)
            printf("Page: 0x%x\n", (unsigned)info.pageSize);

    }catch(std::exception& e){
        printf("Error: %s\n", e.what());
    }
}
}

namespace {
struct FILEWrapper {
    FILE *fp;
    FILEWrapper() :fp(0) {}
    void open(FILE *fp)
    {
        if(!fp)
            throw std::runtime_error(SB()<<"File error "<<errno);
        this->fp = fp;
    }
    ~FILEWrapper() { close(); }
    void close()
    {
        if(fp && fclose(fp))
            printf("Error closing file %u\n", errno);
        fp = 0;
    }
    void write(const void *buf, size_t blen)
    {
        if(fp && fwrite(buf, 1, blen, fp)!=blen)
            printf("Write Error %u\n", errno);
    }
};
}// namespace

extern "C" {
void flashread(const char *name, int addrraw, int countraw, const char *outfile)
{
    try {
        mrf::SPIDevice dev;
        if(!mrf::SPIDevice::lookupDev(name, &dev)) {
            printf("No such device");
            return;
        }
        epicsUInt32 addr = addrraw, count = countraw;

        if(addr&0xff000000) {
            printf("Error: 24-bit address required\n");
            return;
        }

        mrf::CFIFlash mem(dev);

        FILEWrapper out;
        if(outfile)
            out.open(fopen(outfile, "wb"));

        while(count) {
            const epicsUInt32 N = std::min(count, 4096u);
            std::vector<epicsUInt8> buf(N);

            mem.read(addr, N, &buf[0]);

            if(outfile) {
                out.write(&buf[0], buf.size());

            } else {
                for(size_t i=0; i<buf.size(); i++) {
                    printf("%02x", int(buf[i]));
                    if(i%16==15)
                        printf("\n");
                    else if(i%4==3)
                        printf(" ");
                }
            }

            addr += N;
            count -= N;
            if(outfile)
                printf("| %u\n", unsigned(count));
        }

        printf("\nDone\n");

    }catch(std::exception& e){
        printf("Error: %s\n", e.what());
    }
}
}

static const iocshArg flashinfoArg0 = { "device",iocshArgString};
static const iocshArg * const flashinfoArgs[1] =
    {&flashinfoArg0};
static const iocshFuncDef flashinfoFuncDef =
    {"flashinfo",1,flashinfoArgs};

static void flashinfoCall(const iocshArgBuf *args)
{
    flashinfo(args[0].sval);
}

static const iocshArg flashreadArg0 = { "device",iocshArgString};
static const iocshArg flashreadArg1 = { "address",iocshArgInt};
static const iocshArg flashreadArg2 = { "count",iocshArgInt};
static const iocshArg flashreadArg3 = { "file",iocshArgString};
static const iocshArg * const flashreadArgs[4] =
    {&flashreadArg0,&flashreadArg1,&flashreadArg2,&flashreadArg3};
static const iocshFuncDef flashreadFuncDef =
    {"flashread",4,flashreadArgs};

static void flashreadCall(const iocshArgBuf *args)
{
    flashread(args[0].sval, args[1].ival, args[2].ival, args[3].sval);
}

static void registrarFlashOps()
{
    iocshRegister(&flashinfoFuncDef, &flashinfoCall);
    iocshRegister(&flashreadFuncDef, &flashreadCall);
}
epicsExportRegistrar(registrarFlashOps);
