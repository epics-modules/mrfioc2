/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdexcept>
#include <vector>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <iocsh.h>
#include <epicsThread.h>

#include "mrfCommon.h"
#include "mrf/spi.h"
#include "mrf/flash.h"

#include <epicsExport.h>

namespace mrf {

CFIFlash::CFIFlash(const SPIDevice &dev)
    :dev(dev)
    ,haveinfo(false)
{}

CFIFlash::~CFIFlash() {}

void
CFIFlash::readID(ID *id)
{
    epicsUInt8 cmd[7] = {0x9f, 0u, 0u, 0u},
               response[7];
    SPIInterface::Operation ops[2] = {{4, cmd, response}};
    ops[0].ncycles = 4;
    ops[0].in = cmd;
    ops[0].out = response;

    {
        SPIDevice::Selector S(dev);
        dev.interface()->cycles(1, ops);
    }

    if(response[1]==0xff)
    {
        /* the first read after power up seems to fail.
         * Presumably some missing initializtion on my part.
         * A single re-try.
         */
        SPIDevice::Selector S(dev);
        dev.interface()->cycles(1, ops);
    }

    id->vendor = response[1];
    id->dev_type = response[2];
    id->dev_id = response[3];

    id->vendorName = "<Unknown>";
    id->capacity = id->sectorSize = id->pageSize = 0u;

    if(id->vendor==0x20) { // Micron
        /* Some Micron parts support the Serial Flash Discoverable Parameter (SFDP) spec.
         * using command 0x5a (see tech note TN-25-06).
         * However, so far there isn't anything so interesting.
         */
        id->vendorName = "Micron";

        switch(id->dev_type) {
        case 0xba: // mt25q
        case 0xbb:
            id->capacity = 2u<<(id->dev_id+2);
            id->sectorSize = 64*1024u;
            id->pageSize = 256;
            break;
        }

        /* Micron puts some extra info after the regular 0x9f payload,
         * including a serial number.
         * Byte 4 is the length of the additional payload
         */
        ops[0].ncycles = 5;

        {
            SPIDevice::Selector S(dev);
            dev.interface()->cycles(1, ops);
        }

        if(response[4]>2) {
            ops[0].ncycles = 7;

            id->SN.resize(response[4]-2);
            ops[1].ncycles = id->SN.size();
            ops[1].in = NULL;
            ops[1].out = &id->SN[0];

            SPIDevice::Selector S(dev);
            dev.interface()->cycles(2, ops);

            // if this byte isn't 0, then the spec isn't being followed
            if(response[6]!=0)
                id->SN.clear();

            if((response[5]&0x3)!=0) {
                // sector size not known
                id->sectorSize = 0;
            }
        }
    }

    // we only use 24-bit read/write/erase ops
    // so capacity beyond 16MB is not accessible.
    if(id->capacity>0x1000000)
        id->capacity = 0x1000000;
}

void CFIFlash::read(epicsUInt32 start, epicsUInt32 count, epicsUInt8 *data)
{
    if((start&0xff000000) || (count&0xff000000) || ((start+count)&0xff000000))
        std::runtime_error("start/count exceeds 24-bit addressing");

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

void CFIFlash::write(const epicsUInt32 start,
                     const epicsUInt32 count,
                     const epicsUInt8 * const data,
                     const bool strict)
{
    if((start&0xff000000) || (count&0xff000000) || ((start+count)&0xff000000))
        std::runtime_error("start/count exceeds 24-bit addressing");

    check();

    if(strict && info.capacity==0)
        throw std::runtime_error("Won't attempt to write when capacity isn't known");

    else if(start>=info.capacity || (start+count)>info.capacity)
        throw std::runtime_error("Can't write beyond capacity");

    if(info.pageSize==0 || info.sectorSize==0)
        throw std::runtime_error("Won't attempt to write to unsupported flash chip");

    {
        epicsUInt32 mask = (info.pageSize-1u) | (info.sectorSize-1u);
        if(start&mask)
            throw std::runtime_error("start address not aligned to page & sector sizes");

        if(strict && ((start+count)&mask))
            throw std::runtime_error("end address not aligned to page & sector sizes");
    }

    const epicsUInt32 end = start+count;

    const double timeout = dev.interface()->timeout();

    {
        WriteEnabler WE(*this);

        // erase necessary sectors

        for(epicsUInt32 addr=start; addr<end; addr+=info.sectorSize)
        {
            busyWait(timeout);

            WE.enable();

            epicsUInt8 cmd[4];
            cmd[0] = 0xd8; // SECTOR ERASE
            cmd[1] = (addr>>16)&0xff;
            cmd[2] = (addr>> 8)&0xff;
            cmd[3] = (addr>> 0)&0xff;
            SPIInterface::Operation op = {4, cmd, NULL};

            {
                SPIDevice::Selector S(dev);
                dev.interface()->cycles(1, &op);
            }
        }

        // program all pages

        const epicsUInt8 *cur = data;
        for(epicsUInt32 addr=start; addr<end; addr+=info.pageSize, cur+=info.pageSize)
        {
            busyWait(timeout);

            WE.enable();

            const epicsUInt32 N = std::min(info.pageSize, end-addr);

            epicsUInt8 cmd[4];
            cmd[0] = 0x02; // PAGE PROGRAM
            cmd[1] = (addr>>16)&0xff;
            cmd[2] = (addr>> 8)&0xff;
            cmd[3] = (addr>> 0)&0xff;

            SPIInterface::Operation ops[2];
            ops[0].ncycles = 4;
            ops[0].in = cmd;
            ops[0].out = NULL;

            ops[1].ncycles = N;
            ops[1].in = cur;
            ops[1].out = NULL;

            {
                SPIDevice::Selector S(dev);
                dev.interface()->cycles(2, ops);
            }
        }

        busyWait(timeout);

        // end of write phase
    }

    // readback to verify by sector

    std::vector<epicsUInt8> scratch;

    const epicsUInt8 *cur = data;
    bool ok = true;
    for(epicsUInt32 addr=start; addr<end; addr+=info.sectorSize, cur+=info.sectorSize)
    {
        const epicsUInt32 N = std::min(info.sectorSize, end-addr);

        scratch.resize(N);

        read(addr, scratch);

        if(memcmp(cur, &scratch[0], N)!=0) {
            printf("FLASH readback mis-match in sector 0x%06x\n", (unsigned)addr);
            ok = false;
        }
    }

    if(!ok)
        throw std::runtime_error("FLASH readback error");
}

void CFIFlash::check()
{
    if(!haveinfo) {

        readID(&info);

        if(info.vendor==0xff)
            throw std::runtime_error("Invalid Flash vendor ID");

        haveinfo = true;
    }
}

unsigned CFIFlash::status()
{
    epicsUInt8 cmd[2] = {0x05, 0}, response[2];
    SPIInterface::Operation op = {2, cmd, response};

    SPIDevice::Selector S(dev);
    dev.interface()->cycles(1, &op);

    return response[1]&0x03; // pass out only 0x01 busy and 0x02 write enabled
}

void CFIFlash::writeEnable(bool e)
{
    epicsUInt8 cmd = e ? 0x06 : 0x04;
    SPIInterface::Operation op = {1, &cmd, NULL};

    SPIDevice::Selector S(dev);
    dev.interface()->cycles(1, &op);
}

void CFIFlash::busyWait(double timeout, unsigned n)
{
    TimeoutCalculator T(timeout);

    while(T.ok() && (status()&1))
        epicsThreadSleep(T.inc());

    if(!T.ok())
        throw std::runtime_error("Timeout waiting for operation to complete");
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
        if(!info.SN.empty()) {
            printf("S/N: ");
            for(size_t i=0; i<info.SN.size(); i++)
                printf(" %02x", unsigned(info.SN[i]));
            printf("\n");
        }

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
    void write(const std::vector<epicsUInt8>& buf)
    {
        if(fp && fwrite(&buf[0], 1, buf.size(), fp)!=buf.size())
            throw std::runtime_error(SB()<<"Write Error "<<int(errno));
    }
    void read(std::vector<epicsUInt8>& buf)
    {
        if(fp) {
            ssize_t ret = fread(&buf[0], 1, buf.size(), fp);
            if(ret<0)
                throw std::runtime_error(SB()<<"Read Error "<<int(errno));
            buf.resize(ret);
        } else {
            buf.resize(0);
        }
    }
    long size()
    {
        if(fseek(fp, 0, SEEK_END)!=0)
            throw std::runtime_error(SB()<<"Seek error "<<errno);
        long size = ftell(fp);
        if(size==-1)
            throw std::runtime_error(SB()<<"Tell error "<<errno);
        if(fseek(fp, 0, SEEK_SET)!=0)
            throw std::runtime_error(SB()<<"Seek2 error "<<errno);
        return size;
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

        mrf::CFIFlash mem(dev);

        FILEWrapper out;
        if(outfile)
            out.open(fopen(outfile, "wb"));

        while(count) {
            const epicsUInt32 N = std::min(count, 4096u);
            std::vector<epicsUInt8> buf(N);

            mem.read(addr, buf);

            if(outfile) {
                out.write(buf);

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

extern "C" {
void flashwrite(const char *name, int addrraw, const char *infile)
{
    if(!infile || infile[0]=='\0') {
        printf("Usage: flashwrite <name> <start_address> <filename>\n");
        return;
    }

    try {
        mrf::SPIDevice dev;
        if(!mrf::SPIDevice::lookupDev(name, &dev)) {
            printf("No such device");
            return;
        }
        epicsUInt32 addr = addrraw;

        mrf::CFIFlash mem(dev);
        if(!mem.writable()) {
            printf("Device not writable\n");
            return;
        }

        FILEWrapper inp;
        inp.open(fopen(infile, "rb"));
        const long fsize = inp.size();

        std::vector<epicsUInt8> buf;

        long pos=0;
        while(true) {
            printf("| %u/%u\n", (unsigned)pos, (unsigned)fsize);

            buf.resize(mem.blockSize());
            inp.read(buf);
            if(buf.empty())
                break;
            pos += buf.size();

            mem.write(addr, buf, false);
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

static const iocshArg flashwriteArg0 = { "device",iocshArgString};
static const iocshArg flashwriteArg1 = { "address",iocshArgInt};
static const iocshArg flashwriteArg2 = { "file",iocshArgString};
static const iocshArg * const flashwriteArgs[3] =
    {&flashwriteArg0,&flashwriteArg1,&flashwriteArg2};
static const iocshFuncDef flashwriteFuncDef =
    {"flashwrite",3,flashwriteArgs};

static void flashwriteCall(const iocshArgBuf *args)
{
    flashwrite(args[0].sval, args[1].ival, args[2].sval);
}

static void registrarFlashOps()
{
    iocshRegister(&flashinfoFuncDef, &flashinfoCall);
    iocshRegister(&flashreadFuncDef, &flashreadCall);
    iocshRegister(&flashwriteFuncDef, &flashwriteCall);
}
epicsExportRegistrar(registrarFlashOps);
