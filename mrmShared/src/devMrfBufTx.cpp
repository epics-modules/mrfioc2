
#include <stdlib.h>
#include <math.h>

#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>
#include <mbboRecord.h>
#include <waveformRecord.h>
#include <menuFtype.h>
#include <epicsEndian.h>

#ifdef _WIN32
 #include <Winsock2.h>
#else
 // for htons() et al.
 #include <netinet/in.h> // on rtems
 #include <arpa/inet.h> // on linux
#endif


#include "linkoptions.h"
#include "devObj.h"
#include "mrf/databuf.h"

#include <stdexcept>
#include <string>
#include <cfloat>


#include <epicsExport.h>
struct priv
{
  char obj[40];
  epicsUInt32 proto;
  char prop[20];

  dataBufTx *priv;
  epicsUInt8 *scratch;
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

  // scratch space for endian swap if needed
  if(dbValueSize(prec->ftvl)>1 && dbValueSize(prec->ftvl)<=8)
      paddr->scratch = new epicsUInt8[prec->nelm*dbValueSize(prec->ftvl)];
  else
      paddr->scratch = 0;

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
        std::auto_ptr<priv> paddr((priv*)praw->dpvt);
        praw->dpvt = 0;
        delete[] paddr->scratch;

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

  epicsUInt32 capacity=paddr->priv->lenMax();
  const long esize=dbValueSize(prec->ftvl);
  epicsUInt32 requested=prec->nord*esize;

  if (requested > capacity)
    requested=capacity;

  epicsUInt8 *buf;
  if(esize==1 || esize>8)
      buf=static_cast<epicsUInt8*>(prec->bptr);
  else {
      buf=paddr->scratch;
      for(size_t i=0; i<requested; i+=esize) {
          switch(esize) {
          case 2:
            *(epicsUInt16*)(buf+i) = htons( *(epicsUInt16*)((char*)prec->bptr+i) );
            break;
          case 4:
            *(epicsUInt32*)(buf+i) = htonl( *(epicsUInt32*)((char*)prec->bptr+i) );
            break;
          case 8:
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
            *(epicsUInt32*)(buf+i) = *(epicsUInt32*)((char*)prec->bptr+i);
            *(epicsUInt32*)(buf+i+4) = *(epicsUInt32*)((char*)prec->bptr+i+4);
#else
            *(epicsUInt32*)(buf+i+4) = htonl( *(epicsUInt32*)((char*)prec->bptr+i) );
            *(epicsUInt32*)(buf+i) = htonl( *(epicsUInt32*)((char*)prec->bptr+i+4) );
#endif
            break;
          }
      }
  }

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
