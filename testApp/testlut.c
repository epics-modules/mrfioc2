/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <errlog.h>
#include <alarm.h>
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <testMain.h>

void testmrf_registerRecordDeviceDriver(struct dbBase *);

MAIN(testlut)
{
    testPlan(15);

    testdbPrepare();

    testdbReadDatabase("testmrf.dbd", 0, 0);
    testmrf_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("lut.db", 0, 0);

    eltc(0);
    testIocInitOk();
    eltc(1);

    testDiag("Initial values");
    testdbGetFieldEqual("output.STAT", DBF_LONG, UDF_ALARM);
    testdbGetFieldEqual("output.SEVR", DBF_LONG, INVALID_ALARM);
    testdbGetFieldEqual("output.VAL", DBF_STRING, "unknown");

    testDiag("Set valid 0");
    testdbPutFieldOk("input", DBR_LONG, 0);
    testdbGetFieldEqual("output.STAT", DBF_LONG, 0);
    testdbGetFieldEqual("output.SEVR", DBF_LONG, 0);
    testdbGetFieldEqual("output.VAL", DBF_STRING, "zero");

    testDiag("Set valid 5");
    testdbPutFieldOk("input", DBR_LONG, 5);
    testdbGetFieldEqual("output.STAT", DBF_LONG, 0);
    testdbGetFieldEqual("output.SEVR", DBF_LONG, 0);
    testdbGetFieldEqual("output.VAL", DBF_STRING, "five");

    testDiag("Set invalid 3");
    testdbPutFieldOk("input", DBR_LONG, 3);
    testdbGetFieldEqual("output.STAT", DBF_LONG, READ_ALARM);
    testdbGetFieldEqual("output.SEVR", DBF_LONG, INVALID_ALARM);
    testdbGetFieldEqual("output.VAL", DBF_STRING, "unknown");

    testIocShutdownOk();

    testdbCleanup();

    return testDone();
}
