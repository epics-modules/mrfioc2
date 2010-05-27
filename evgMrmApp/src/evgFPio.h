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
	const IOtype 				type;
	const epicsUInt32 			id;
	volatile epicsUInt8* const	pReg;
};
#endif //EVGFPIO_H