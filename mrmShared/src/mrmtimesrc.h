/*************************************************************************\
* Copyright (c) 2016 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef MRMTIMESRC_H
#define MRMTIMESRC_H

#include <string>

/* Handles sending shirt 0/1 events after the start of each second.
 */
class TimeStampSource
{
    struct Impl;
    Impl * const impl;
public:
    explicit TimeStampSource(double period);
    virtual ~TimeStampSource();

    //! Call to re-initialize timestamp counter from system time
    void resyncSecond();

    //! Call just after the start of each second
    void tickSecond();

    //! Whether tickSecond() has been called for the past 5 seconds
    bool validSeconds() const;

    //! last difference between
    double deltaSeconds() const;

    //! enable sending of event 125 by software timer.  Simulation of external HW clock
    void softSecondsSrc(bool enable);
    bool isSoftSeconds() const;

    std::string nextSecond() const;

protected:
    virtual void setEvtCode(epicsUInt32 evtCode) =0;

    virtual void postSoftSecondsSrc() {tickSecond();}

private:
    TimeStampSource(const TimeStampSource&);
    TimeStampSource& operator=(const TimeStampSource&);
};

#endif // MRMTIMESRC_H
