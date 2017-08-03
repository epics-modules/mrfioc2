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

namespace mrf {

//! Interface for SPI Master
struct SPIInterface
{
    SPIInterface();
    virtual ~SPIInterface();

    //! Select numbered device.  0 clears selection.
    virtual void select(unsigned id);

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

class SPIDevice
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
        Selector(SPIDevice& dev) :dev(dev)
        { dev.spi->select(dev.id); }
        ~Selector()
        { dev.spi->select(0u); }
    };

    static bool lookupDev(const std::string& name, SPIDevice*);
    static void registerDev(const std::string& name, const SPIDevice& );
    static void unregisterDev(const std::string& name);
};

} // namespace mrf

#endif // SPI_H
