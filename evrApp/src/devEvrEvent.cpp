
#include <stdlib.h>
#include <string.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <eventRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "cardmap.h"
#include "evr/evr.h"

#include <stdexcept>
#include <string>

/***************** Event *****************/

struct priv {
  EVR* evr;
  int event;
};

static
long add_record(struct dbCommon *precord)
{
  eventRecord* prec=(eventRecord*)(precord);
  long ret=0;
try {
  assert(prec->inp.type==VME_IO);

  priv *p=new priv;

  p->evr=getEVR<EVR>(prec->inp.value.vmeio.card);
  if(!p->evr)
    throw std::runtime_error("Failed to lookup device");

  p->event=prec->inp.value.vmeio.signal;

  if (!p->evr->interestedInEvent(p->event, true))
    throw std::runtime_error("Failed to register interest");

  prec->dpvt=static_cast<void*>(p);

  return 0;
} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  mrfDisableRecord((dbCommon*)prec);
  return ret;
}

static
long del_record(struct dbCommon *precord)
{
  eventRecord* prec=(eventRecord*)(precord);
  priv *p=static_cast<priv*>(prec->dpvt);
  long ret=0;
try {

  p->evr->interestedInEvent(p->event, false);

  delete p;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  return ret;
}

dsxt devext={add_record,del_record};

static
long init(int i)
{
  if (i==0) devExtend(&devext);
  return 0;
}

static
long init_record(eventRecord *prec)
{
  return 0;
}

static
long
get_ioint_info(int dir,dbCommon* precord,IOSCANPVT* io)
{
  eventRecord* prec=(eventRecord*)(precord);
  priv *p=static_cast<priv*>(prec->dpvt);
  long ret=0;
try {

  if(!p) return 1;

  if(!dir)
    *io=p->evr->eventOccurred(p->event);
  else
    *io=NULL;

  return 0;
} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  *io=NULL;
  return ret;
}

static long read_event(eventRecord *precord)
{
  eventRecord* prec=(eventRecord*)(precord);
  priv *p=static_cast<priv*>(prec->dpvt);
  long ret=0;
try {

  if(prec->tse==2){
    p->evr->getTimeStamp(&prec->time,p->event);
  }

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

extern "C" {

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_event;
} devEventEVR = {
    5,
    NULL,
    (DEVSUPFUN) init,
    (DEVSUPFUN) init_record,
    (DEVSUPFUN) get_ioint_info,
    (DEVSUPFUN) read_event
};
epicsExportAddress(dset,devEventEVR);

};
