#ifndef EVGFPIO_H
#define EVGFPIO_H

#include <epicsTypes.h>

enum IOtype {
	FP_Output = 0,
	Univ_Output,
	FP_Input,
	Univ_Input
};

class evgFPio {
public:
	evgFPio(const IOtype, const epicsUInt32, volatile epicsUInt8* const);
	~evgFPio();
	epicsStatus setIOMap(epicsUInt32);

private:
	const IOtype 				m_type;
	const epicsUInt32 			m_id;
	volatile epicsUInt8* const	m_pReg;
};
#endif //EVGFPIO_H