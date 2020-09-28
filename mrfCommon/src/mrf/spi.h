/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef SPI_H
#define SPI_H

#include <stdlib.h>

#include <string>

#include <epicsTypes.h>
#include <epicsMutex.h>
#include <mrf/mrfCommonAPI.h>

namespace mrf {

//! Interface for SPI Master
struct MRFCOMMON_API SPIInterface
{
    SPIInterface();
    virtual ~SPIInterface();

    //! Select numbered device.  0 clears selection.
    virtual void select(unsigned id) =0;

    //! Perform a single SPI transaction
    //! @throws std::runtime_error on timeout
    virtual epicsUInt8 cycle(epicsUInt8 in) =0;

    struct Operation {
        size_t ncycles;
        const epicsUInt8 *in;
        epicsUInt8 *out;
    };

    virtual void cycles(size_t nops,
                          const Operation *ops);

    //! timeout in seconds for an individual cycle()
    double timeout() const;
    void setTimeout(double t);

protected:
    mutable epicsMutex mutex;
private:
    double optimo;
};

class MRFCOMMON_API SPIDevice
{
    SPIInterface * spi;
    unsigned id;
public:
    SPIDevice() :spi(0), id(0) {}
    SPIDevice(SPIInterface *spi, unsigned id) :spi(spi), id(id) {}

    inline SPIInterface* interface() const { return spi; }
    inline unsigned selector() const { return id; }

    class Selector {
        SPIDevice& dev;
    public:
        explicit Selector(SPIDevice& dev) :dev(dev)
        { dev.spi->select(dev.id); }
        ~Selector()
        { dev.spi->select(0u); }
    };

    static bool lookupDev(const std::string& name, SPIDevice*);
    static void registerDev(const std::string& name, const SPIDevice& );
    static void unregisterDev(const std::string& name);
};

class TimeoutCalculator
{
    const double total;
    const double factor;
    const double initial;
    double accumulated;
    double next;
public:
    TimeoutCalculator(double total, double factor=2.0, double initial=0.01)
        :total(total), factor(factor), initial(initial), accumulated(0.0), next(0.0)
    {}
    bool ok() const { return accumulated<total; }
    double inc() {
        double ret=next;
        accumulated+=ret;
        if(next)
            next*=factor;
        else
            next=initial;
        return ret;
    }
    double sofar() const { return accumulated; }
};

} // namespace mrf

#endif // SPI_H
