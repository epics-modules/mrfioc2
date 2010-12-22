/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <cstdlib>
#include <devLib.h>

#include "dsetshared.h"

extern "C" {

long init_record_empty(void *)
{
  return 0;
}

long init_record_return2(void *)
{
  return 2;
}

long del_record_empty(dbCommon*)
{
  return 0;
}

}
