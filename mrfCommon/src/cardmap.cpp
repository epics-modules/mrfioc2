
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
