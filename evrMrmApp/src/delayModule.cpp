/*************************************************************************\
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include "drvem.h"
#include "delayModule.h"

#define SERIAL_DATA_BIT(idx)            (1 << (0+4*idx))
#define SERIAL_CLOCK_BIT(idx)           (1 << (1+4*idx))
#define TRANSFER_LATCH_CLOCK_BIT(idx)   (1 << (2+4*idx))
#define OUTPUT_DISABLE_BIT(idx)         (1 << (3+4*idx))


DelayModule::DelayModule(const std::string& n, EVRMRM* o, unsigned int idx)
    : DelayModuleEvr(n)
    ,N_(idx)
    ,gpio_(o->gpio())
{
}

DelayModule::~DelayModule()
{
}

void DelayModule::setDelay0(double val)
{
    if(val < 2.200) val = 2.200;
    if(val > 12.430)val = 12.430;
	setDelay(true, false, (epicsUInt16)((val - 2.2) * 100 + 0.5), 0);
}

double DelayModule::getDelay0() const
{
    return (dly0_ / 100.0) + 2.200;
}

void DelayModule::setDelay1(double val)
{
    if(val < 2.200) val = 2.200;
    if(val > 12.430)val = 12.430;
	setDelay(false, true, 0, (epicsUInt16)((val - 2.200) * 100.0 + 0.5));
}

double DelayModule::getDelay1() const
{
    return (dly1_ / 100.0) + 2.200;
}

void DelayModule::setState(bool enabled)
{
    epicsGuard<epicsMutex> g(gpio_->lock_);
    setGpioOutput();
    if(enabled) enable();
    else disable();
}

bool DelayModule::enabled() const
{
    epicsUInt32 data;

    data = gpio_->getOutput();
    return !(data && OUTPUT_DISABLE_BIT(N_));
}

// ---------------------- //
// Private methods below  //
// ---------------------- //

void DelayModule::setGpioOutput(){
    epicsUInt32 direction;

    // set 4 pins affecting the module to output
    direction = gpio_->getDirection();
    direction |= SERIAL_DATA_BIT(N_) | SERIAL_CLOCK_BIT(N_) | TRANSFER_LATCH_CLOCK_BIT(N_) | OUTPUT_DISABLE_BIT(N_);
    gpio_->setDirection(direction);
}

void DelayModule::enable(){
    epicsUInt32 data;

    //clear output disable bit to enable the module
    data = gpio_->getOutput();
    data &= ~OUTPUT_DISABLE_BIT(N_);
    gpio_->setOutput(data);
}

void DelayModule::disable(){
    epicsUInt32 data;

    //set output disable bit to disable the module
    data = gpio_->getOutput();
    data |= OUTPUT_DISABLE_BIT(N_);
    gpio_->setOutput(data);
}

void DelayModule::setDelay(bool output0, bool output1, epicsUInt16 value0, epicsUInt16 value1){
    epicsUInt32 latch=0, delay=0;

    //printf("out0: %d, out1: %d, value0: %d, value1: %d\n", output0, output1, value0, value1);
    if(output0 | output1){
        //we need to lock GPIO pins, so nobody else messes with them.
        epicsGuard<epicsMutex> g(gpio_->lock_);
        setGpioOutput();

        // We have to shift in the bits in following order:
        //   DA7, DA6, DA5, DA4, DA3, DA2, DA1, DA0,
        //    DB3, DB2, DB1, DB0, LENA, 0, DA9, DA8,
        //    LENB, 0, DB9, DB8, DB7, DB6, DB5, DB4

        if(output0){  // output 0 selected = UNIV 0 = DB
            delay = ((value0 & 0x00f) << 12) | (value0 >> 4);
            latch = 0x000080;
            dly0_ = value0;
        }
        if(output1){ // output 1 selected = UNIV 1 = DA
            delay |= ((value1 & 0x0ff) << 16) | (value1 & 0x300);
            latch |= 0x000800;
            dly1_ = value1;
        }

        // we output the data, set the latches and output the data again. This is to ensure that the delay values are stable when we latch them.
        pushData(delay);
        pushData(delay | latch);
        pushData(delay);
    }
}

void DelayModule::pushData(epicsUInt32 data){
    epicsUInt32 bit, gpio;
    epicsUInt8 i;

    // Prepare gpio mask before sending the data. Allways use mask so we don't ruin data on other GPIO pins.
    gpio = gpio_->getOutput() & ~(SERIAL_DATA_BIT(N_) | SERIAL_CLOCK_BIT(N_) | TRANSFER_LATCH_CLOCK_BIT(N_));

    for( i = 24; i; i-- ){
        bit = 0;
        if(data & 0x00800000) bit = SERIAL_DATA_BIT(N_);
        gpio_->setOutput(gpio | bit);
        gpio_->setOutput(gpio | bit | SERIAL_CLOCK_BIT(N_));
        gpio_->setOutput(gpio | bit);
        data <<= 1;
    }
    // finish the transmission. A rising edge on LCLK transfers the data from the shift register to the actual delay chips
    gpio_->setOutput(gpio | TRANSFER_LATCH_CLOCK_BIT(N_));
    gpio_->setOutput(gpio);
}
