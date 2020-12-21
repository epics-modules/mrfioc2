#ifndef INC_evrAPI_h
#define INC_evrAPI_h

#include <epicsVersion.h>

#ifndef VERSION_INT
#  define VERSION_INT(V,R,M,P) ( ((V)<<24) | ((R)<<16) | ((M)<<8) | (P))
#endif

#ifndef EPICS_VERSION_INT
#  define EPICS_VERSION_INT VERSION_INT(EPICS_VERSION, EPICS_REVISION, EPICS_MODIFICATION, EPICS_PATCH_LEVEL)
#endif

/* Prior to 3.15, the signal for a DLL build was inverted */
#if defined(_WIN32) && EPICS_VERSION_INT<VERSION_INT(3,15,0,0) && !defined(EPICS_DLL_NO)
#    define EPICS_BUILD_DLL
#    define EPICS_CALL_DLL
#endif

#if defined(_WIN32) || defined(__CYGWIN__)

#  if !defined(epicsStdCall)
#    define epicsStdCall __stdcall
#  endif

#  if defined(BUILDING_EVR_API) && defined(EPICS_BUILD_DLL)
/* Building library as dll */
#    define EVR_API __declspec(dllexport)
#  elif !defined(BUILDING_EVR_API) && defined(EPICS_CALL_DLL)
/* Calling library in dll form */
#    define EVR_API __declspec(dllimport)
#  endif

#elif __GNUC__ >= 4
#  define EVR_API __attribute__ ((visibility("default")))
#endif

#if !defined(EVR_API)
#  define EVR_API
#endif

#if !defined(epicsStdCall)
#  define epicsStdCall
#endif

#endif /* INC_evrAPI_h */
