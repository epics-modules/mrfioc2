/*************************************************************************\
* Copyright (c) 2016 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef MRMSEQ_H
#define MRMSEQ_H

#include <vector>
#include <map>
#include <string>

#include <dbScan.h>
#include <shareLib.h>

#include <mrf/object.h>

// special trigger sources w/ HW dependent representation
#define SEQ_TRIG_NONE (-1)
#define SEQ_TRIG_SW   (-2)
#define SEQ_TRIG_INP(n) (-100-(n))

struct SeqHW;
struct SoftSequence;

class epicsShareClass SeqManager : public mrf::ObjectInst<SeqManager>
{
    typedef mrf::ObjectInst<SeqManager> base_t;
public:
    // Which model card?
    // used handle external and software trigger source mapping
    enum Type {
        TypeEVG, // "classic" 230/300 series EVG as well as EVM
        TypeEVR, // 300DC EVR
    };
    const Type type;

    SeqManager(const std::string& name, Type t);
    virtual ~SeqManager();

    // no locking needed.  our members are effectivly "const" after addHW() during sub-class ctor
    virtual void lock() const {}
    virtual void unlock() const {}

    static mrf::Object* buildSW(const std::string& name, const std::string& klass, const mrf::Object::create_args_t& args);

    //! Call from ISR
    void doStartOfSequence(unsigned i);
    //! Call from ISR
    void doEndOfSequence(unsigned i);

    //! sub-class implement to provide scaling between time and frequency.
    //! Units of Hz
    //! called with a SoftSeq mutex held.
    virtual double getClkFreq() const =0;

    //! sub-class implement
    //! Setup (or clear w/ zero) the external trigger source
    //! Called from interrupt context
    virtual void mapTriggerSrc(unsigned i, unsigned src) =0;

    virtual epicsUInt32 testStartOfSeq() =0;

protected:
    void addHW(unsigned i,
               volatile void *ctrl,
               volatile void *ram);
private:
    typedef std::vector<SeqHW*> hw_t;
    hw_t hw;
    friend struct SoftSequence;
};

epicsShareExtern int SeqManagerDebug;

#endif // MRMSEQ_H
