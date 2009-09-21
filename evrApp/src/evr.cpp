
#include "evr.hpp"

/**@file evr.cpp
 *
 * Contains implimentations of the pure virtual
 * destructors of the interfaces for C++ reasons.
 * These must be present event though they are never
 * called.  If they are absent that linking will
 * fail in some cases.
 */

EVR::~EVR()
{
}

IOStatus::~IOStatus()
{
}

Pulser::~Pulser()
{
}

Output::~Output()
{
}

PreScaler::~PreScaler()
{
}
