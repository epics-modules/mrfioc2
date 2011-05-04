
#include <stdlib.h>
#include <math.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>
#include <mbboRecord.h>
#include <waveformRecord.h>
#include <menuFtype.h>

#include "mrf/databuf.h"
#include "linkoptions.h"
#include "devObj.h"

#include <stdexcept>
#include <string>
#include <cfloat>

struct priv
{
    char obj[40];
  epicsUInt32 proto;
  char prop[20];

  dataBufTx *priv;
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
try {
  assert(prec->inp.type==INST_IO);

  std::auto_ptr<priv> paddr(new priv);

  if (linkOptionsStore(eventdef, paddr.get(), prec->inp.value.instio.string, 0))
    throw std::runtime_error("Couldn't parse link string");

  mrf::Object *O=mrf::Object::getObject(paddr->obj);
  if(!O) {
      errlogPrintf("%s: failed to find object '%s'\n", prec->name, paddr->obj);
      return S_db_errArg;
  }
  paddr->priv=dynamic_cast<dataBufTx*>(O);
  if(!paddr->priv)
    throw std::runtime_error("Failed to lookup device");

  // prec->dpvt is set again to indicate
  // This also serves to indicate successful
  // initialization to other dset functions
  prec->dpvt = paddr.release();

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  return ret;
}

static long del_record_waveform(dbCommon *praw)
{
    long ret=0;
    if (!praw->dpvt) return 0;
    try {
        delete (priv*)praw->dpvt;
        praw->dpvt = 0;

    } catch(std::runtime_error& e) {
        recGblRecordError(S_dev_noDevice, (void*)praw, e.what());
        ret=S_dev_noDevice;
    } catch(std::exception& e) {
        recGblRecordError(S_db_noMemory, (void*)praw, e.what());
        ret=S_db_noMemory;
    }
    return ret;
}

static long write_waveform(waveformRecord* prec)
{
  if (!prec->dpvt) return -1;
try {
  priv *paddr=static_cast<priv*>(prec->dpvt);

  unsigned char *buf=static_cast<unsigned char*>(prec->bptr);

  epicsUInt32 capacity=paddr->priv->lenMax();
  const long esize=dbValueSize(prec->ftvl);
  epicsUInt32 requested=prec->nord*esize;

  if (requested > capacity)
    requested=capacity;

  paddr->priv->dataSend(paddr->proto,requested,buf);

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/*************** boiler plate *******************/

extern "C" {

dsxt dxtwaveformBufTx={add_record_waveform,del_record_waveform};


static common_dset devwaveformoutdataBufTx = {
  6, NULL,
  dset_cast(&init_dset<&dxtwaveformBufTx>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&write_waveform),
  NULL };
epicsExportAddress(dset,devwaveformoutdataBufTx);

}
