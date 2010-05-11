
#include "evr/evr.h"
#include "evr/pulser.h"
#include "evr/output.h"
#include "evr/input.h"
#include "evr/prescaler.h"
#include "evr/cml_short.h"
#include "evr/util.h"

#include "dbCommon.h"

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

Pulser::~Pulser()
{
}

Output::~Output()
{
}

Input::~Input()
{
}

PreScaler::~PreScaler()
{
}

CMLShort::~CMLShort()
{
}

long get_ioint_info_statusChange(int dir,dbCommon* prec,IOSCANPVT* io)
{
  IOStatus* stat=static_cast<IOStatus*>(prec->dpvt);

  if(!stat) return 1;

  *io=stat->statusChange(dir);
  
  return 0;
}
