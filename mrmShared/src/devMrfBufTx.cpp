
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
#include "dsetshared.h"

#include <stdexcept>
#include <string>
#include <cfloat>

struct addr
{
  epicsUInt32 card;
  epicsUInt32 proto;
  char prop[20];

  dataBufTx *priv;
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
prop_entry<dataBufTx,bool> props_bool[] = {
  {"Enable",property<dataBufTx,bool>(0, &dataBufTx::dataTxEnabled, &dataBufTx::dataTxEnable)},
  {"RTS",property<dataBufTx,bool>(0, &dataBufTx::dataRTS)},
  {NULL,property<dataBufTx,bool>()}
};

static
dataBufTx* get_tx(const char* hwlink, std::string& propname)
{
  addr inst_addr;

  if (linkOptionsStore(eventdef, &inst_addr, hwlink, 0))
    throw std::runtime_error("Couldn't parse link string");

  dataBufTx* card=&datatxmap.get(inst_addr.card);
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

  paddr->priv=&datatxmap.get(paddr->card);
  if(!paddr->priv)
    throw std::runtime_error("Failed to lookup device");

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

static long write_waveform(waveformRecord* prec)
{
  if (!prec->dpvt) return -1;
try {
  addr *paddr=static_cast<addr*>(prec->dpvt);

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

PROPERTY_DSET_BI(dataBufTx, get_tx, props_bool);
PROPERTY_DSET_BO(dataBufTx, get_tx, props_bool);

dsxt dxtwaveformBufTx={add_record_waveform,del_record_empty};


static common_dset devwaveformoutdataBufTx = {
  6, NULL,
  dset_cast(&init_dset<&dxtwaveformBufTx>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&write_waveform),
  NULL };
epicsExportAddress(dset,devwaveformoutdataBufTx);

}
