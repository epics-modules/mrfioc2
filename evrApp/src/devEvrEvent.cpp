
#include <stdlib.h>
#include <string.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <longoutRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "evrmap.h"
#include "evr/evr.h"

#include "linkoptions.h"

#include <stdexcept>
#include <string>

/***************** Event *****************/

struct priv {
  EVR* evr;
  epicsUInt32 card;
  int event;
};
typedef struct priv priv;

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (priv, card , "Card" , 1, 0),
  linkInt32   (priv, event, "Code", 1, 0),
  linkOptionEnd
};

static
long add_record(struct dbCommon *precord)
{
  longoutRecord* prec=(longoutRecord*)(precord);
  long ret=0;
  priv *p=NULL;
try {
  assert(prec->out.type==INST_IO);

  if (prec->dpvt) {
    p=static_cast<priv*>(prec->dpvt);
    prec->dpvt=NULL;
  } else
    p=new priv;
  p->card=0;
  p->event=0;

  if (linkOptionsStore(eventdef, p, prec->out.value.instio.string, 0))
    throw std::runtime_error("Couldn't parse link string");

  p->evr=&evrmap.get(p->card);
  if(!p->evr)
    throw std::runtime_error("Failed to lookup device");

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
  if (!prec->dpvt) delete p;
  return ret;
}

static
long del_record(struct dbCommon *precord)
{
  longoutRecord* prec=(longoutRecord*)(precord);
  priv *p=static_cast<priv*>(prec->dpvt);
  long ret=0;
try {

  p->evr->interestedInEvent(p->event, false);

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
long init_record(longoutRecord *prec)
{
  return 0;
}

static
long
get_ioint_info(int dir,dbCommon* precord,IOSCANPVT* io)
{
  longoutRecord* prec=(longoutRecord*)(precord);
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

static long read_event(longoutRecord *precord)
{
  longoutRecord* prec=(longoutRecord*)(precord);
  priv *p=static_cast<priv*>(prec->dpvt);
  long ret=0;
try {

  if (prec->val>=0 && prec->val<=255)
    post_event(prec->val);

  if(prec->tse==epicsTimeEventDeviceTime){
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
