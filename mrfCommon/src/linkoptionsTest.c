/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdio.h>
#include <string.h>

#include "linkoptions.h"

#include "epicsUnitTest.h"
#include "epicsString.h"
#include "testMain.h"

struct adev {
  epicsUInt32 a;
  double b;
  char c[10];
  int d;
};

static const
linkOptionEnumType colorEnum[] = { {"Red",1}, {"Green",2}, {"Blue",3}, {NULL,0} };

static const
linkOptionDef myStructDef[] = {
  linkInt32 (struct adev, a, "A" , 0, 0),
  linkDouble(struct adev, b, "BEE"  , 1, 0),
  linkString(struct adev, c, "name"  , 1, 0),
  linkEnum  (struct adev, d, "Color"   , 1, 0, colorEnum),
  linkOptionEnd
};

const char one[]="BEE=4.2, A=42, Color=Green, name=fred";
const char two[]="BEE=2.4, Color=Blue, name=ralph";

MAIN(linkoptionsTest)
{
    struct adev X;

    testPlan(10);

    X.a=0;
    X.b=0.0;
    X.c[0]='\0';
    X.d=0;

    testOk1(linkOptionsStore(myStructDef, &X, one, 0)==0);

    testOk1(X.a==42);
    testOk1(X.b==4.2);
    testOk1(strcmp(X.c, "fred")==0);
    testOk1(X.d==2);

    X.a=14;
    X.b=0.0;
    X.c[0]='\0';
    X.d=0;

    testOk1(linkOptionsStore(myStructDef, &X, two, 0)==0);

    testOk1(X.a==14);
    testOk1(X.b==2.4);
    testOk1(strcmp(X.c, "ralph")==0);
    testOk1(X.d==3);

    return 0;
}
