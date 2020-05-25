/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include "devObj.h"

const
linkOptionEnumType readbackEnum[] = { {"No",0}, {"Yes",1} };

const
linkOptionDef objdef[] =
{
    linkString  (addrBase, obj , "OBJ"  , 1, 0),
    linkString  (addrBase, prop , "PROP"  , 1, 0),
    linkEnum    (addrBase, rbv, "RB"   , 0, 0, readbackEnum),
    linkString  (addrBase, klass , "CLASS"  , 0, 0),
    linkString  (addrBase, parent , "PARENT"  , 0, 0),
    linkOptionEnd
};
