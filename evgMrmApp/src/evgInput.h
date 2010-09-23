#ifndef EVG_INPUT_H
#define EVG_INPUT_H

#include <epicsTypes.h>

enum InputType {
	None_Input = 0,
	FP_Input,
	Univ_Input,
	TB_Input
};

class evgInput {
public:
	evgInput(const epicsUInt32, const InputType, volatile epicsUInt8* const);
	~evgInput();

	epicsStatus enaExtIrq(bool);
	epicsStatus setInpDbusMap(epicsUInt32);
	epicsStatus setInpSeqTrigMap(epicsUInt32 seqTrigMap);
	epicsStatus setInpTrigEvtMap(epicsUInt32);

private:
	volatile epicsUInt8* const 	m_pInMap;
};
#endif //EVG_INPUT_H
