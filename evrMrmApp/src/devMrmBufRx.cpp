/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>
#include <mbboRecord.h>
#include <waveformRecord.h>
#include <menuFtype.h>
#include <callback.h>

#include "mrf/databuf.h"
#include "linkoptions.h"
#include "devObj.h"

#include <stdexcept>
#include <string>
#include <cfloat>

/**
 * Note on record scanning.
 *
 * In order to avoid making more copies of each data buffer this record is manually
 * processed from the Rx callback instead of using normal I/O Intr scanning.
 */

static
void datarx(void *, epicsStatus ,epicsUInt8 , epicsUInt32 , const epicsUInt8* );

struct priv
{
  char obj[40];
  epicsUInt32 proto;
  char prop[20];

  dataBufRx *priv;

  epicsUInt32 blen;
  const epicsUInt8* buf;
};

static const
linkOptionDef eventdef[] =
{
  linkString  (priv, obj , "OBJ"  , 1, 0),
  linkInt32   (priv, proto, "Proto", 1, 0),
  linkString  (priv, prop , "P", 1, 0),
  linkOptionEnd
};

static long add_record_waveform(dbCommon *praw)
{
  waveformRecord *prec=(waveformRecord*)praw;
  long ret=0;
  priv *paddr=NULL;
try {
  assert(prec->inp.type==INST_IO);

  std::auto_ptr<priv> paddr(new priv);
  paddr->buf=NULL;
  paddr->blen=0;

  if (linkOptionsStore(eventdef, paddr.get(), prec->inp.value.instio.string, 0))
    throw std::runtime_error("Couldn't parse link string");

  mrf::Object *O=mrf::Object::getObject(paddr->obj);
  if(!O) {
      errlogPrintf("%s: failed to find object '%s'\n", prec->name, paddr->obj);
      return S_db_errArg;
  }
  paddr->priv=dynamic_cast<dataBufRx*>(O);
  if(!paddr->priv)
    throw std::runtime_error("Failed to lookup device");

  paddr->priv->dataRxAddReceive(paddr->proto, datarx, praw);

  // prec->dpvt is set again to indicate
  // This also serves to indicate successful
  // initialization to other dset functions
  prec->dpvt = (void*)paddr.release();

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  delete paddr;
  return ret;
}

static long del_record_waveform(dbCommon *praw)
{
    long ret=0;
    if (!praw->dpvt) return 0;
    try {
        std::auto_ptr<priv> paddr((priv*)praw->dpvt);
        praw->dpvt = 0;

        paddr->priv->dataRxDeleteReceive(paddr->proto, datarx, praw);

    } catch(std::runtime_error& e) {
        recGblRecordError(S_dev_noDevice, (void*)praw, e.what());
        ret=S_dev_noDevice;
    } catch(std::exception& e) {
        recGblRecordError(S_db_noMemory, (void*)praw, e.what());
        ret=S_db_noMemory;
    }
    return ret;
}

static
void datarx(void *arg, epicsStatus ok, epicsUInt8,
            epicsUInt32 len, const epicsUInt8* buf)
{
    //TODO: scan lock!
    waveformRecord* prec=(waveformRecord*)arg;

    dbScanLock((dbCommon*)prec);

    if (!prec->dpvt) {
        dbScanUnlock((dbCommon*)prec);
        return;
    }

    rset *prset=(rset*)prec->rset;
    priv *paddr=(priv*)prec->dpvt;

    if (ok) {
        // An error occured
        paddr->buf=NULL;
        paddr->blen=ok;
        return;
    } else {
        paddr->buf=buf;
        paddr->blen=len;
    }

    (*(long (*)(waveformRecord*))prset->process)(prec);

    paddr->buf=NULL;
    paddr->blen=0;

    dbScanUnlock((dbCommon*)prec);
}

static long write_waveform(waveformRecord* prec)
{
  epicsUInt32 dummy;

  if (!prec->dpvt) return -1;
try {
  priv *paddr=static_cast<priv*>(prec->dpvt);

  if (!paddr->buf && paddr->blen) {
      // Error condition set INVALID_ALARM
      dummy = recGblSetSevr(prec, READ_ALARM, MAJOR_ALARM);

  } else if(paddr->buf) {

      epicsUInt32 capacity=prec->nelm*dbValueSize(prec->ftvl);

      if (paddr->blen > capacity) {
          paddr->blen=capacity;
          dummy = recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
      }

      memcpy(prec->bptr, paddr->buf, paddr->blen);

      prec->nord = paddr->blen/dbValueSize(prec->ftvl);
  }

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/*************** boiler plate *******************/

extern "C" {

dsxt dxtwaveforminBufTx={add_record_waveform,del_record_waveform};


static common_dset devwaveformindataBufRx = {
  6, NULL,
  dset_cast(&init_dset<&dxtwaveforminBufTx>),
  (DEVSUPFUN) &init_record_empty,
  NULL,
  dset_cast(&write_waveform),
  NULL };
epicsExportAddress(dset,devwaveformindataBufRx);

}

