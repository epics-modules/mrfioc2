#ifndef MRMSEQ_H
#define MRMSEQ_H

#include <vector>
#include <map>
#include <string>

#include <dbScan.h>

#include <mrf/object.h>

// special trigger sources w/ HW dependent representation
#define SEQ_TRIG_NONE (-1)
#define SEQ_TRIG_SW   (-2)
#define SEQ_TRIG_INP(n) (-100-(n))

struct SeqHW;
struct SoftSequence;

class SeqManager
{
public:
    // Which model card?
    // used handle external and software trigger source mapping
    enum Type {
        TypeEVG, // "classic" 230/300 series EVG as well as EVM
        TypeEVR, // 300DC EVR
    };
    const Type type;

    SeqManager(Type t);
    virtual ~SeqManager();

    static mrf::Object* buildSW(const std::string& name, const std::string& klass, const mrf::Object::create_args_t& args);

    //! Call from ISR
    void doStartOfSequence(unsigned i);
    void doEndOfSequence(unsigned i);

    //! sub-class implement to provide scaling between time and frequency.
    //! Units of Hz
    //! called with a SoftSeq mutex held.
    virtual double getClkFreq() const =0;

    //! sub-class implement
    //! Setup (or clear w/ zero) the external trigger source
    virtual void mapTriggerSrc(unsigned i, unsigned src) =0;

#ifdef __linux__
    virtual void pollISR() =0;
#endif
protected:
    void addHW(unsigned i,
               volatile void *ctrl,
               volatile void *ram);
private:
    typedef std::vector<SeqHW*> hw_t;
    hw_t hw;
    friend struct SoftSequence;
};

#endif // MRMSEQ_H
