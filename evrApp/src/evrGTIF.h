
#ifndef EVRGTIF_H_INC
#define EVRGTIF_H_INC

#include <epicsTypes.h>
#include <epicsTime.h>
#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*General Time Interface
 *
 * C interface to the timestamp supply functions
 * of the EVR interface for use with generalTime.
 */

/**@brief (Un)Register interest in event N.
 *
 * Only interesting events have the arrival time stored.
 *
 *@param card  Card index #
 *@param event [1,255]  - Timestamp of last received event N
 *@param add 1 - Increment interest counter.
 *@param add 0 - Decrement interest counter.
 *
 * Error codes:
 *   0 - Ok
 *  -1 - Argument error: Invalid card index
 *  -2 - Unable to become interested.  Event out of range or HW error
 */
epicsShareFunc
epicsStatus ErEventInterest(int card, epicsInt32 event, int add);

/**@brief Get current real-time or time of last event N
 *
 *@param card Card index #
 *@param event >0  - Timestamp of last received event N
 *@param event <=0 - Current real-time
 *@param ts Pointer to be filled with result time
 *@return 0 Ok.  Result written to 'ts'.
 *@return !0 Fail.  Value pointed to by 'ts' is undefined
 *
 * Error codes:
 *   0 - Ok
 *  -1 - Argument error: Invalid card index or ts is null
 *  -2 - Unavailable because of system error (link down, heartbeat timeout, etc.)
 */
epicsShareFunc
epicsStatus ErGetTimeStamp (epicsInt32 card, epicsInt32 event, epicsTimeStamp* ts);

/**@brief Get current value of tick counter
 *
 *@param card  Card index #
 *@param ticks Pointer to be filled with result
 *@return 0 Ok.  Result written to 'ts'.
 *@return !0 Fail.  Value pointed to by 'ts' not modified
 *
 * Error codes:
 *   0 - Ok
 *  -1 - Argument error: Invalid card index, or ticks is null
 *  -2 - Unavailable because of system error (link down, heartbeat timeout, etc.)
 */
epicsShareFunc
epicsStatus ErGetTicks (int card, epicsUInt32* ticks);

#ifdef __cplusplus
}
#endif

#endif /* EVRGTIF_H_INC */
