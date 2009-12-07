
#ifndef EVRMRMIOCSH_H
#define EVRMRMIOCSH_H

#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Extra noise control
 *
 * Controls level of extra diagnositic output from the
 * EVR MRM device support.  Error messages are always
 * printed.
 *
 * 0 - No output
 * 1 - Basic Startup
 * 2 - Extended startup some "write"
 * 3 - All "write" (output record processing)
 * 4 - Some "read" (input record processing) and IRQ
 */
epicsShareExtern int evrmrmVerb;


#ifdef __cplusplus
}
#endif

#endif /* EVRMRMIOCSH_H */
