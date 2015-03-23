#include "delayModule.h"
#include "drvem.h"
#include "mrmGpio.h"
//bite z define

DelayModule::DelayModule(const std::string& n, EVRMRM* o, unsigned int idx)
    : mrf::ObjectInst<DelayModule>(n)
    ,serialData_        (1 << (0+4*idx))
    ,serialClock_       (1 << (1+4*idx))
    ,transferLatchClock_(1 << (2+4*idx))
    ,outputDisable_     (1 << (3+4*idx))
    //,owner_(o)
    //,N_(idx)
    ,gpio_(o->gpio())
{
}

void DelayModule::setGpioOutput(){
    epicsUInt32 direction;

    // set 4 pins affecting the module to output
    gpio_->lock();
    direction = gpio_->getDirection();
    direction |= serialData_ | serialClock_ | transferLatchClock_ | outputDisable_;
    gpio_->setDirection(direction);
    gpio_->unlock();
}

void DelayModule::enable(){
    epicsUInt32 data;

    //clear output disable bit to enable the module
    gpio_->lock();
    data = gpio_->getOutput();
    data &= ~outputDisable_;
    gpio_->setOutput(data);
    gpio_->unlock();
}

void DelayModule::disable(){
    epicsUInt32 data;

    //set output disable bit to disable the module
    gpio_->lock();
    data = gpio_->getOutput();
    data |= (outputDisable_);
    gpio_->setOutput(data);
    gpio_->unlock();
}

void DelayModule::setDelay(epicsUInt16 delay0, epicsUInt16 delay1){
    epicsUInt32 data, delay;

    enable();

    // limit delay values
    delay0 &= 0x03FF;
    delay1 &= 0x03FF;

    // We have to shift in the bits in following order:
    //   DA7, DA6, DA5, DA4, DA3, DA2, DA1, DA0,
    //    DB3, DB2, DB1, DB0, LENA, 0, DA9, DA8,
    //    LENB, 0, DB9, DB8, DB7, DB6, DB5, DB4

    delay = ((delay1 & 0x0ff) << 16) | (delay1 & 0x300) |
            ((delay0 & 0x00f) << 12) | (delay0 >> 4);

    pushData(delay);

    // only activate shifter latches so the data passes through all 3 delays
    data = 0x000880;
    pushData(data);
    //through third delay also...
    pushData(data);

}

void DelayModule::setDelay(bool output0, bool output1, epicsUInt16 value0, epicsUInt16 value1){
    epicsUInt32 latch=0, delay=0;

    printf("out0: %d, out1: %d, value0: %d, value1: %d\n", output0, output1, value0, value1);
    if(output0 | output1){
        setGpioOutput();
        enable();

        // limit delay values
        value0 &= 0x03FF;
        value1 &= 0x03FF;

        // We have to shift in the bits in following order:
        //   DA7, DA6, DA5, DA4, DA3, DA2, DA1, DA0,
        //    DB3, DB2, DB1, DB0, LENA, 0, DA9, DA8,
        //    LENB, 0, DB9, DB8, DB7, DB6, DB5, DB4

        if(output0){  // output 0 selected
            delay = ((value0 & 0x00f) << 12) | (value0 >> 4);
            latch = 0x000800;
        }
        if(output1){ // output 1 selected
            delay |= ((value1 & 0x0ff) << 16) | (value1 & 0x300);
            latch |= 0x000080;
        }
    printf("data to be pushed: %x\n", delay);
        pushData(delay);
    printf("Latch: %x\n", latch);
        // only activate shifter latches so the data passes through all 3 delays
        pushData(latch);
        //through third delay also...
        pushData(latch);
    }

}

void DelayModule::pushData(epicsUInt32 data){
    epicsUInt32 bit, gpio;
    epicsUInt8 i;

    gpio_->lock(); // prevent others messing with our output and make sure we don't destroy data others are setting on GPIO
    // Prepare gpio mask before sending the data
    gpio = gpio_->getOutput() & ~(serialData_ | serialClock_ | transferLatchClock_);

    for( i = 24; i; i-- ){
        bit = 0;
        if(data & 0x00800000) bit = serialData_;
        gpio_->setOutput(gpio | bit);
        gpio_->setOutput(gpio | bit | serialClock_);
        gpio_->setOutput(gpio | bit);
        data <<= 1;
    }
    // finish the transmission. A rising edge on LCLK transfers the data from the shift register to the actual delay chips
    gpio_->setOutput(gpio | transferLatchClock_);
    gpio_->setOutput(gpio);
    gpio_->unlock();
}
