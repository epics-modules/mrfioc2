#ifndef BUF_H_
#define BUF_H_

#include <epicsTypes.h>

typedef enum {
    bufferOK,
    bufferDataNull,
    bufferTxNotInit,
    bufferRxNotInit,
} buffer_status;

typedef struct buffer_info_t buffer_info;

extern "C" buffer_info *bufInit(char *dev_name);
extern "C" buffer_status maxLen(buffer_info *data, epicsUInt32 *maxLength);
extern "C" buffer_status bufSend(buffer_info *data, epicsUInt32 len, epicsUInt8 *buf);
extern "C" buffer_status bufRegCallback(buffer_info *data, void (*bufRecievedCallback)(void *, epicsStatus , epicsUInt32, const epicsUInt8 *), void *);

#endif /* BUF_H_ */
