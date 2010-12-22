/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include "cardmap.h"

#include <set>

static
std::set<short> ids;

static
epicsMutex idLock;

bool cardIdInUse(short id)
{
    SCOPED_LOCK(idLock);
    return ids.find(id) != ids.end();
}

void cardIdAdd(short id)
{
    SCOPED_LOCK(idLock);
    ids.insert(id);
}
