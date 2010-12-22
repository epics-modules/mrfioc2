/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include "evr/evr.h"
#include "evr/pulser.h"
#include "evr/output.h"
#include "evr/input.h"
#include "evr/prescaler.h"
#include "evr/cml.h"
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

CML::~CML()
{
}

long get_ioint_info_statusChange(int dir,dbCommon* prec,IOSCANPVT* io)
{
    IOStatus* stat=static_cast<IOStatus*>(prec->dpvt);

    if(!stat) return 1;

    *io=stat->statusChange(dir);

    return 0;
}
