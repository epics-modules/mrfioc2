/**
 * @file    buf.h
 * @author  Jure Krasna <jure.krasna@cosylab.com>
 * @brief   C interface for data buffer sending and receiving
 */

#ifndef BUF_H_
#define BUF_H_

#include <epicsTypes.h>

/**
 * @brief The buffer information data structure
 *
 * This structure holds references to receive and send classes for
 * a specific device - either a receiver or a generator.
 */
typedef struct bufferInfo bufferInfo_t;

/**
 * @brief Buffer received callback function
 *
 * @param arg       Parameter which is assigned at callback registration
 * @param status    Status of the receive action (0 = ok)
 * @param length    Length of the buffer in bytes
 * @param buffer    Buffer data
 */
typedef void (*bufRecievedCallback)(void *arg, epicsStatus status, epicsUInt32 length, const epicsUInt8 *buffer);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize buffer data structure
 *
 * This function allocates and initializes the structure for use with
 * other functions. Device name should correspond to the same name as is
 * used when initializing EVR or EVG device with either mrmEv*SetupVME or
 * mrmEv*SetupPCI functions / iocsh commands.
 *
 * @param dev_name  The name of the device to use for buffer sending and
 *                  receiving
 *
 * @return Returns the structure pointer on success and NULL on failure.
 */
bufferInfo_t epicsShareFunc *bufInit(char *dev_name);

/**
 * @brief Disable buffer sending logic.
 *
 * Enables data buffer logic for both transfer and receive functionality.
 * This is already called by the enable record so it is only needed if
 * the record for buffer enabling are not present.
 *
 * @param data      The buffer information data structure
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc bufEnable(bufferInfo_t *data);

/**
 * @brief Disable buffer sending logic.
 *
 * Disables data buffer logic for both transfer and receive functionality.
 *
 * @param data      The buffer information data structure
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc bufDisable(bufferInfo_t *data);

/**
 * @brief Get maximum supported buffer length.
 *
 * @param data      The buffer information data structure
 * @param maxLength Maximum buffer length in bytes
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc bufMaxLen(bufferInfo_t *data, epicsUInt32 *maxLength);

/**
 * @brief Send buffer data
 *
 * The function blocks until data is sent completely.
 *
 * @param data      The buffer information data structure
 * @param len       Buffer length in bytes
 * @param buf       Array of buffer data
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc bufSend(bufferInfo_t *data, epicsUInt32 len, epicsUInt8 *buf);

/**
 * @brief Register data receive callback function
 *
 * @param data      The buffer information data structure
 * @param callback  Callback function to be registered
 * @param param     Parameter which is passed to the calllback when it is executed
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc bufRegCallback(bufferInfo_t *data, bufRecievedCallback callback, void *param);

#ifdef __cplusplus
}
#endif

#endif /* BUF_H_ */
