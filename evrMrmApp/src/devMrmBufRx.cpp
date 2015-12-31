/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdexcept>
#include <string>
#include <cfloat>

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

// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#else
 // for htons() et al.
 #include <netinet/in.h> // on rtems
 #include <arpa/inet.h> // on linux
#endif

#define DATABUF_H_INC_LEVEL2
#include <epicsExport.h>
#include "mrf/databuf.h"
#include "linkoptions.h"
#include "devObj.h"


/**
 * Note on record scanning.
 *
 * In order to avoid making more copies of each data buffer this record is manually
 * processed from the Rx callback instead of using normal I/O Intr scanning.
 */

static
void datarx(void *, epicsStatus , epicsUInt32 , const epicsUInt8* );

struct s_priv
{
  char obj[40];
  epicsUInt32 proto;
  epicsUInt32 proto16;
  epicsUInt32 proto32;
  char prop[20];

  dataBufRx *priv;

  epicsUInt32 blen;
  const epicsUInt8* buf;
};

static const
linkOptionDef eventdef[] =
{
  linkString  (s_priv, obj , "OBJ"  , 1, 0),
  linkInt32   (s_priv, proto, "Proto", 0, 0),
  linkInt32   (s_priv, proto16, "Proto16", 0, 0),
  linkInt32   (s_priv, proto32, "Proto32", 0, 0),
  linkString  (s_priv, prop , "P", 1, 0),
  linkOptionEnd
};

static long add_record_waveform(dbCommon *praw)
{
  waveformRecord *prec=(waveformRecord*)praw;
  long ret=0;
  s_priv *paddr=NULL;
try {
  assert(prec->inp.type==INST_IO);

  std::auto_ptr<s_priv> paddr(new s_priv);
  paddr->buf=NULL;
  paddr->blen=0;
  paddr->proto = 0xff00;
  paddr->proto16 = 0;
  paddr->proto32 = 0;

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

  paddr->priv->dataRxAddReceive(datarx, praw);

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
        std::auto_ptr<s_priv> paddr((s_priv*)praw->dpvt);
        praw->dpvt = 0;

        paddr->priv->dataRxDeleteReceive(datarx, praw);

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
    waveformRecord* prec=(waveformRecord*)arg;
    s_priv *paddr=(s_priv*)prec->dpvt;
    
    //check protocol id
    if (paddr->proto != 0xff00 && paddr->proto != buf[0]) return;
    if (paddr->proto16 && paddr->proto16 != ntohs(((epicsUInt16*)buf)[0])) return;
    if (paddr->proto32 && paddr->proto32 != ntohl(((epicsUInt32*)buf)[0])) return;

    dbScanLock((dbCommon*)prec);

    if (!prec->dpvt) {
        dbScanUnlock((dbCommon*)prec);
        return;
    }


    if (ok) {
        // An error occured
        paddr->buf=NULL;
        paddr->blen=ok;
        return;
    } else {
        paddr->buf=buf;
        paddr->blen=len;
    }

    rset *prset=(rset*)prec->rset;
    (*(long (*)(waveformRecord*))prset->process)(prec);

    paddr->buf=NULL;
    paddr->blen=0;

    dbScanUnlock((dbCommon*)prec);
}

static long write_waveform(waveformRecord* prec)
{
  if (!prec->dpvt) return -1;
try {
  s_priv *paddr=static_cast<s_priv*>(prec->dpvt);

  if (!paddr->buf && paddr->blen) {
      // Error condition set INVALID_ALARM
      (void)recGblSetSevr(prec, READ_ALARM, MAJOR_ALARM);

  } else if(paddr->buf) {

      long esize = dbValueSize(prec->ftvl);
      epicsUInt32 capacity=prec->nelm*esize;

      if (paddr->blen > capacity) {
          paddr->blen=capacity;
          (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
      }

      if(esize==1 || esize>8) // char or string
          memcpy(prec->bptr, paddr->buf, paddr->blen);
      else {
          epicsUInt8 *buf=(epicsUInt8*)prec->bptr;
          for(size_t i=0; i<paddr->blen; i+=esize) {
              switch(esize) {
              case 2:
                *(epicsUInt16*)(buf+i) = htons( *(epicsUInt16*)(paddr->buf+i) );
                break;
              case 4:
                *(epicsUInt32*)(buf+i) = htonl( *(epicsUInt32*)(paddr->buf+i) );
                break;
              case 8:
    #if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
                *(epicsUInt32*)(buf+i) = *(epicsUInt32*)(paddr->buf+i);
                *(epicsUInt32*)(buf+i+4) = *(epicsUInt32*)(paddr->buf+i+4);
    #else
                *(epicsUInt32*)(buf+i+4) = htonl( *(epicsUInt32*)(paddr->buf+i) );
                *(epicsUInt32*)(buf+i) = htonl( *(epicsUInt32*)(paddr->buf+i+4) );
    #endif
                break;
              }
          }
      }

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

