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

// defines macros for printf() so that output is captured
// for iocsh when redirecting to file.
#include <epicsStdio.h>

#include "mrfCommon.h"
#include "mrf/spi.h"
#include "mrf/flash.h"

#include <epicsExport.h>


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

        {
            mrf::CFIStreamBuf sbuf(mem);
            std::istream strm(&sbuf);
            mrf::XilinxBitInfo image;
            if(image.read(strm)) {
                printf("Bit Stream found\n  Project: %s\n  Part: %s\n  Build: %s\n",
                       image.project.c_str(), image.part.c_str(), image.date.c_str());
            }
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
            const epicsUInt32 N = std::min(count, mem.blockSize());
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
            addr += buf.size();
        }

        printf("\nDone\n");

    }catch(std::exception& e){
        printf("Error: %s\n", e.what());
    }
}
}

extern "C" {
void flasherase(const char *name, int addrraw, int countraw)
{
    try {
        mrf::SPIDevice dev;
        if(!mrf::SPIDevice::lookupDev(name, &dev)) {
            printf("No such device");
            return;
        }
        epicsUInt32 addr = addrraw, count = countraw;

        mrf::CFIFlash mem(dev);

        mem.erase(addr, count);

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

static const iocshArg flasheraseArg0 = { "device",iocshArgString};
static const iocshArg flasheraseArg1 = { "address",iocshArgInt};
static const iocshArg flasheraseArg2 = { "count",iocshArgInt};
static const iocshArg * const flasheraseArgs[3] =
    {&flasheraseArg0,&flasheraseArg1,&flasheraseArg2};
static const iocshFuncDef flasheraseFuncDef =
    {"flasherase",3,flasheraseArgs};

static void flasheraseCall(const iocshArgBuf *args)
{
    flasherase(args[0].sval, args[1].ival, args[2].ival);
}

static void registrarFlashOps()
{
    iocshRegister(&flashinfoFuncDef, &flashinfoCall);
    iocshRegister(&flashreadFuncDef, &flashreadCall);
    iocshRegister(&flashwriteFuncDef, &flashwriteCall);
    iocshRegister(&flasheraseFuncDef, &flasheraseCall);
}
epicsExportRegistrar(registrarFlashOps);
