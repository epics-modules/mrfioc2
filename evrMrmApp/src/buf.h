#ifndef BUF_H_
#define BUF_H_

#include <epicsTypes.h>

enum {
    statusOK,
    statusERROR,
};

typedef struct bufferInfo bufferInfo_t;

#ifdef __cplusplus
extern "C" {
#endif

bufferInfo_t *bufInit(char *dev_name);
epicsStatus bufEnable(bufferInfo_t *data);
epicsStatus bufDisable(bufferInfo_t *data);
epicsStatus bufMaxLen(bufferInfo_t *data, epicsUInt32 *maxLength);
epicsStatus bufSend(bufferInfo_t *data, epicsUInt32 len, epicsUInt8 *buf);
epicsStatus bufRegCallback(bufferInfo_t *data, void (*bufRecievedCallback)(void *, epicsStatus , epicsUInt32, const epicsUInt8 *), void *);

#ifdef __cplusplus
}
#endif

#endif /* BUF_H_ */
