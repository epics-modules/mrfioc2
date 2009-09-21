
#ifndef CARDMAP_H_INC
#define CARDMAP_H_INC

#include "evr/evr.h"

/**@file cardmap.h
 *
 * Utilities for interaction between IOC Shell functions
 * and device support initialization.
 */

/**@brief Lookup the EVR* associated with 'id'
 *
 *@returns NULL in id has no association.
 */
EVR* getEVRBase(short id);

//! Lookup an EVR* and attempt to cast to the requested sub-class.
template<typename EVRSubclass>
EVRSubclass*
getEVR(short id)
{
  return dynamic_cast<EVRSubclass*>(getEVRBase(id));
};

/**@brief Save the association between 'id' and 'dev'.
 *
 * throws std::runtime_error if 'id' has already been used.
 */
void storeEVR(short id, EVR* dev);

#endif /* CARDMAP_H_INC */
