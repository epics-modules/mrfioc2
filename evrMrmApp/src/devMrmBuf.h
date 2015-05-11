/**
 * @file    devMrmBuf.h
 * @author  Jure Krasna <jure.krasna@cosylab.com>
 * @brief   C interface for data buffer sending and receiving
 */

#ifndef DEVMRMBUF_H_
#define DEVMRMBUF_H_

#include <epicsTypes.h>

/**
 * @brief The buffer information data structure
 *
 * This structure holds references to receive and send classes for
 * a specific device - either a receiver or a generator.
 */
typedef struct mrmBufferInfo mrmBufferInfo_t;

/**
 * @brief Buffer received callback function
 *
 * @param arg       Parameter which is assigned at callback registration
 * @param status    Status of the receive action (0 = ok)
 * @param length    Length of the buffer in bytes
 * @param buffer    Buffer data
 */
typedef void (*mrmBufRecievedCallback)(void *arg, epicsStatus status, epicsUInt32 length, const epicsUInt8 *buffer);

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
mrmBufferInfo_t epicsShareFunc *mrmBufInit(const char *dev_name);

/**
 * @brief Checks whether receive buffer is supported.
 *
 * Since some devices (EVR or EVG) might only support transfer
 * functionality this function allows you to check whether you
 * can call all receive-related functions for this device.
 *
 * @param data      The buffer information data structure
 *
 * @return Returns 1 if supported, 0 if not and -1 if data is NULL.
 */
epicsStatus mrmBufRxSupported(mrmBufferInfo_t *data);

/**
 * @brief Checks whether transferring buffer is supported.
 *
 * Since some devices (EVR or EVG) might only support receive
 * functionality this function allows you to check whether you
 * can call all transfer-related functions for this device.
 *
 * @param data      The buffer information data structure
 *
 * @return Returns 1 if supported, 0 if not and -1 if data is NULL.
 */
epicsStatus mrmBufTxSupported(mrmBufferInfo_t *data);

/**
 * @brief Disable buffer sending logic.
 *
 * Enables data buffer logic for both transfer and receive functionality.
 * This is already called by the enable record so it is only needed if
 * the records for buffer enabling are not present.
 *
 * @param data      The buffer information data structure
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc mrmBufEnable(mrmBufferInfo_t *data);

/**
 * @brief Disable buffer sending logic.
 *
 * Disables data buffer logic for both transfer and receive functionality.
 *
 * @param data      The buffer information data structure
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc mrmBufDisable(mrmBufferInfo_t *data);

/**
 * @brief Get maximum supported buffer length.
 *
 * @param data      The buffer information data structure
 * @param maxLength Maximum buffer length in bytes
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc mrmBufMaxLen(mrmBufferInfo_t *data, epicsUInt32 *maxLength);

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
epicsStatus epicsShareFunc mrmBufSend(mrmBufferInfo_t *data, epicsUInt32 len, epicsUInt8 *buf);

/**
 * @brief Register data receive callback function
 *
 * @param data      The buffer information data structure
 * @param callback  Callback function to be registered
 * @param param     Parameter which is passed to the calllback when it is executed
 *
 * @return Returns 0 on success -1 on failure.
 */
epicsStatus epicsShareFunc mrmBufRegCallback(mrmBufferInfo_t *data, mrmBufRecievedCallback callback, void *param);

#ifdef __cplusplus
}
#endif

#endif /* DEVMRMBUF_H_ */
