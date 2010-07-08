
#include "cardmap.h"

#include <set>

static
std::set<short> ids;

bool cardIdInUse(short id)
{
    return ids.find(id) != ids.end();
}

void cardIdAdd(short id)
{
    ids.insert(id);
}
