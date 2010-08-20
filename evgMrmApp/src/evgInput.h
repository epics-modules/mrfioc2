#ifndef EVG_INPUT_H
#define EVG_INPUT_H

#include <epicsTypes.h>

enum InputType {
	FP_Input = 0,
	Univ_Input,
	TB_Input
};

class evgInput {
public:
	evgInput(const epicsUInt32, volatile epicsUInt8* const, const InputType);
	~evgInput();
	epicsStatus setInpDbusMap(epicsUInt32);
	epicsStatus setInpTrigEvtMap(epicsUInt32);
	epicsStatus enaExtIrq(bool);

private:
	const epicsUInt32 			m_id;
	volatile epicsUInt8* const	m_pReg;
	const InputType 			m_type;
};
#endif //EVG_INPUT_H
