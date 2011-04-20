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
#include "dsetshared.h"

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
void datarx(void *, epicsStatus ,epicsUInt32 , const epicsUInt8* );

struct addr
{
  epicsUInt32 card;
  epicsUInt32 proto;
  char prop[20];

  dataBufRx *priv;

  epicsUInt32 blen;
  const epicsUInt8* buf;
};

static const
linkOptionDef eventdef[] =
{
  linkInt32   (addr, card , "C", 1, 0),
  linkInt32   (addr, proto, "Proto", 1, 0),
  linkString  (addr, prop , "P", 1, 0),
  linkOptionEnd
};

static const
prop_entry<dataBufRx,bool> props_bool[] = {
  {"Enable",property<dataBufRx,bool>(0, &dataBufRx::dataRxEnabled, &dataBufRx::dataRxEnable)},
  {NULL,property<dataBufRx,bool>()}
};

static
dataBufRx* get_rx(const char* hwlink, std::string& propname)
{
  addr inst_addr;

  if (linkOptionsStore(eventdef, &inst_addr, hwlink, 0))
    throw std::runtime_error("Couldn't parse link string");

  dataBufRx* card=&datarxmap.get(inst_addr.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  propname=std::string(inst_addr.prop);

  return card;
}

static long add_record_waveform(dbCommon *praw)
{
  waveformRecord *prec=(waveformRecord*)praw;
  long ret=0;
  addr *paddr=NULL;
try {
  assert(prec->inp.type==INST_IO);

  paddr = getdpvt<addr>(prec);

  if (linkOptionsStore(eventdef, paddr, prec->inp.value.instio.string, 0))
    throw std::runtime_error("Couldn't parse link string");

  paddr->priv=&datarxmap.get(paddr->card);
  if(!paddr->priv)
    throw std::runtime_error("Failed to lookup device");

  paddr->priv->dataRxAddReceive(paddr->proto, datarx, praw);

  // prec->dpvt is set again to indicate
  // This also serves to indicate successful
  // initialization to other dset functions
  setdpvt(prec, paddr);

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
    addr *paddr=static_cast<addr*>(praw->dpvt);
    try {

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
void datarx(void *arg, epicsStatus ok,
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
    addr *paddr=(addr*)prec->dpvt;

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
  addr *paddr=static_cast<addr*>(prec->dpvt);

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

PROPERTY_DSET_BI(dataBufRx, get_rx, props_bool);
PROPERTY_DSET_BO(dataBufRx, get_rx, props_bool);

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

