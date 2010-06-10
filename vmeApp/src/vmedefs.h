
#ifndef VMEDEFS_H
#define VMEDEFS_H

#if defined(__vxWorks__)

/* include? */

#elif defined(__rtems__)

#include <bsp/vme_am_defs.h>

#else

#define VME_AM_STD_SUP_DATA 0x3d
#define VME_AM_EXT_SUP_DATA 0x0d


#endif

#endif /* VMEDEFS_H */
