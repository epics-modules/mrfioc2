#include <vector>
#include <algorithm>
#include <stdexcept>

#include <stdlib.h>
#include <string.h>

#include <epicsMath.h>
#include "epicsUnitTest.h"
#include "epicsString.h"
#include "testMain.h"

#include "mrf/spi.h"
#include "mrf/flash.h"

namespace {

void testTimeout()
{
    testDiag("testTimeout()");

    mrf::TimeoutCalculator C(1.0, 2.0, 0.1);

    testOk1(C.ok());
    testOk1(C.inc()==0.0);
    testOk1(C.sofar()==0.0);

    testOk1(C.ok());
    testOk1(C.inc()==0.1);
    testOk(fabs(C.sofar()-0.1)<0.001, "%f == 0.1", C.sofar());

    testOk1(C.ok());
    testOk1(C.inc()==0.2);
    testOk(fabs(C.sofar()-0.3)<0.001, "%f == 0.3", C.sofar());

    testOk1(C.ok());
    testOk1(C.inc()==0.4);
    testOk(fabs(C.sofar()-0.7)<0.001, "%f == 0.7", C.sofar());

    testOk1(C.ok());
    testOk1(C.inc()==0.8);
    testOk(fabs(C.sofar()-1.5)<0.001, "%f == 1.5", C.sofar());

    testOk1(!C.ok());
}

//! Simulation of SPi Flash device
struct TestDevice : public mrf::SPIInterface
{
    enum state_t {
        Idle,
        Addr,
        Wait,
        Data,
        Error,
    } current;
    unsigned command;
    unsigned count;
    epicsUInt32 address;
    epicsUInt8 status;
    bool selected;
    bool WE;

    std::vector<epicsUInt8> ram;

    TestDevice() :current(Idle), command(0), count(0), address(0), status(0), selected(false), WE(false) {}

    virtual void select(unsigned id)
    {
        testDiag("select %u", id);

        bool select = !!id;

        if(!select) {
            // some commands (eg. erase) clear WE on completion.
            // as there is no functional issue with sending ENABLE WRITE
            // too often, we pesemistically require this after all erase/program
            switch(command) {
            case 0xd8: // sector erase
            case 0x02: // page program
                WE = false;
            }

            current = Idle;
            command = count = 0;
            address = 0;
        }

        selected = select;
    }

    virtual epicsUInt8 cycle(epicsUInt8 in)
    {
        testDiag("cycle(%02x) current=%u command=%02x count=%u", (unsigned)in, current, command, count);

        if(!selected) throw std::logic_error("cycle() when not selected");

        if((status&1) && current==Idle && in!=0x05)
            throw std::logic_error("Only READ STATUS allowed while busy");

        switch(current) {
        case Idle:
            command = in;
            switch(in) {
            case 0x04: // write disable
                testDiag("Write Disable");
                WE = false;
                current = Error;
                break;
            case 0x06: // write enable
                testDiag("Write Enable");
                WE = true;
                status |= 2;
                current = Error;
                break;
            case 0xd8: // sector erase
            case 0x02: // page program
                if(!WE)
                    throw std::logic_error("Can't erase/program when !WE");
            case 0x0b: // fast read
                count = 0;
                current = Addr;
                break;
            case 0x05: // read status
            case 0x9f: // read ID
                count = 0;
                current = Data;
                break;
            default:
                throw std::logic_error("Unknown command");
            }
            break;
        case Addr:
            switch(count++) {
            case 0: address = epicsUInt32(in)<<16; break;
            case 1: address |= epicsUInt32(in)<<8; break;
            case 2:
                address |= epicsUInt32(in)<<0;
                switch(command) {
                case 0x0b:
                    current = Wait;
                    break;
                case 0xd8:
                    testDiag("Erase sector %06x", (unsigned)address);
                    status |= 1;
                    for(epicsUInt32 cur=address, end=address+64*1024u; cur<end && cur<ram.size(); cur++)
                        ram[cur] = 0xff;
                    current = Error;
                    break;
                case 0x02:
                    testDiag("program page %06x", (unsigned)address);
                    count = 0;
                default:
                    current = Data;
                    break;
                }
            }
            break;
        case Wait:
            current = Data;
            break;
        case Data:
            if(command==0x02) {
                if(count++>=256)
                    throw std::logic_error("Can't program more than one page");

                if(address>=ram.size())
                    ram.resize(address+1, 0xff);

                if(ram[address]!=0xff)
                    throw std::logic_error("Write w/ erase");

                ram[address] = in;
                address++;

            } else if(command==0x05) {
                epicsUInt8 ret = status;
                status &= ~1;
                testDiag("status -> %02x", ret);
                return ret;

            } else if(command==0x0b) {
                count++;
                if(address<ram.size())
                    return ram[address++];

            } else if(command==0x9f) {
                switch(++count) {
                case 1: return 0x20;
                case 2: return 0xba;
                case 3: return 0x17; // 64MB
                case 4: return 0x10; // 16 bytes payload
                case 5: return 0x00;
                case 6: return 0x00;
                    // arbitrary UID
                case 7: return 0x01;
                case 8: return 0x02;
                case 9: return 0x03;
                case 10: return 0x04;
                case 11: return 0x05;
                case 12: return 0x06;
                case 13: return 0x07;
                case 14: return 0x08;
                case 15: return 0x09;
                case 16: return 0x0a;
                case 17: return 0x0b;
                case 18: return 0x0c;
                case 19: return 0x0d;
                case 20: return 0x0e;
                }
            }
            break;
        case Error:
            throw std::logic_error("Cycle during bad state");
            break;
        }
        return 0xff;
    }
};

void testReadID()
{
    testDiag("testReadID()");

    TestDevice iface;
    mrf::SPIDevice dev(&iface, 1);
    mrf::CFIFlash mem(dev);

    mrf::CFIFlash::ID info;

    mem.readID(&info);

    testOk1(info.vendor==0x20);
    testOk1(info.dev_type==0xba);
    testOk1(info.dev_id==0x17);
    // capacity >16MB is truncated to 16MB (we use 24-bit operations)
    testOk1(info.capacity==16*1024*1024);
    testOk1(info.pageSize==256);
    testOk1(info.sectorSize==64*1024);
}

void testRead()
{
    testDiag("testRead");

    TestDevice iface;

    iface.ram.resize(128);
    for(size_t i=0; i<iface.ram.size(); i++)
        iface.ram[i] = i|0x80;

    mrf::SPIDevice dev(&iface, 1);
    mrf::CFIFlash mem(dev);

    {
        testDiag("Read one");
        epicsUInt8 val;

        mem.read(0, 1, &val);

        testOk1(val==0x80);

        mem.read(0x23, 1, &val);

        testOk1(val==0xa3);

        mem.read(0x256, 1, &val);

        testOk1(val==0xff);
    }

    {
        testDiag("Read many");
        epicsUInt8 val[8];

        mem.read(0, 8, val);
        testOk1(val[0]==0x80);
        testOk1(val[1]==0x81);
        testOk1(val[2]==0x82);
        testOk1(val[3]==0x83);
        testOk1(val[4]==0x84);
        testOk1(val[5]==0x85);
        testOk1(val[6]==0x86);
        testOk1(val[7]==0x87);

        mem.read(16, 8, val);
        testOk1(val[0]==0x90);
        testOk1(val[1]==0x91);
        testOk1(val[2]==0x92);
        testOk1(val[3]==0x93);
        testOk1(val[4]==0x94);
        testOk1(val[5]==0x95);
        testOk1(val[6]==0x96);
        testOk1(val[7]==0x97);
    }
}

void testWrite()
{
    static const epicsUInt8 helloworld[] = "helloworld";
    static const epicsUInt8 thisisatest[] = "thisisatest";
    testDiag("testWrite()");

    TestDevice iface;
    mrf::SPIDevice dev(&iface, 1);
    mrf::CFIFlash mem(dev);

    testDiag("Write one page");

    mem.write(0, 10, helloworld, false);

    testOk(iface.ram.size()==10, "%zu == 10", iface.ram.size());

    testOk1(memcmp(helloworld, &iface.ram[0], 10)==0);

    testDiag("Write one page");

    mem.write(2*64*1024, 11, thisisatest, false);

    testOk(iface.ram.size()==2*64*1024 + 11, "%zu == 64kb + 11", iface.ram.size());

    testOk1(memcmp(helloworld, &iface.ram[0], 10)==0);
    testOk1(iface.ram[10]==0xff);
    testOk1(iface.ram[2*64*1024 - 1]==0xff);
    testOk1(memcmp(thisisatest, &iface.ram[2*64*1024], 11)==0);

    testDiag("Write many pages");

    std::vector<epicsUInt8> bigbuf(6*64*1024);
    for(epicsUInt32 i=0, cnt=0; i<bigbuf.size(); i+=4, cnt++)
    {
        bigbuf[i+0] = cnt>>24;
        bigbuf[i+1] = cnt>>16;
        bigbuf[i+2] = cnt>> 8;
        bigbuf[i+3] = cnt>> 0;
    }

    mem.write(0, bigbuf, true);

    testOk(iface.ram.size()==bigbuf.size(), "%zu == %zu", iface.ram.size(), bigbuf.size());

    bool match=true;
    for(size_t i=0, N=std::min(iface.ram.size(), bigbuf.size()); i<N; i++)
    {
        bool ok = iface.ram[i]==bigbuf[i];
        match &= ok;
    }
    testOk1(!!match);
}

} // namespace

MAIN(flashTest)
{
    testPlan(50);
    try{
        testTimeout();
        testReadID();
        testRead();
        testWrite();
    }catch(std::exception& e){
        testAbort("Exception %s", e.what());
    }
    return testDone();
}
