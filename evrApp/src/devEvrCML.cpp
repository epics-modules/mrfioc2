
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

#include "cardmap.h"
#include "evr/cml.h"
#include "linkoptions.h"
#include "dsetshared.h"

#include <stdexcept>
#include <string>
#include <cfloat>

struct addr
{
  epicsUInt32 card;
  epicsUInt32 output;
  char prop[20];
};

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (addr, card , "C", 1, 0),
  linkInt32   (addr, output,"I", 1, 0),
  linkString  (addr, prop , "P", 1, 0),
  linkOptionEnd
};

static const
prop_entry<CML,bool> props_bool[] = {
  {"Enable",property<CML,bool>(0, &CML::enabled, &CML::enable)},
  {"Reset", property<CML,bool>(0, &CML::inReset, &CML::reset)},
  {"Power", property<CML,bool>(0, &CML::powered, &CML::power)},
  {"Freq Trig Lvl", property<CML,bool>(0, &CML::polarityInvert, &CML::setPolarityInvert)},
  {NULL,property<CML,bool>()}
};

static const
prop_entry<CML,epicsUInt32> props_epicsUInt32[] = {
  {"Pat Rise", property<CML,epicsUInt32>(0, &CML::getPattern<cmlShortRise>,
                                            &CML::setPattern<cmlShortRise>)},
  {"Pat High", property<CML,epicsUInt32>(0, &CML::getPattern<cmlShortHigh>,
                                            &CML::setPattern<cmlShortHigh>)},
  {"Pat Fall", property<CML,epicsUInt32>(0, &CML::getPattern<cmlShortFall>,
                                            &CML::setPattern<cmlShortFall>)},
  {"Pat Low", property<CML,epicsUInt32>(0,  &CML::getPattern<cmlShortLow>,
                                            &CML::setPattern<cmlShortLow>)},
  {"Counts High", property<CML,epicsUInt32>(0, &CML::countHigh, &CML::setCountHigh)},
  {"Counts Low", property<CML,epicsUInt32>(0, &CML::countLow, &CML::setCountLow)},
  {NULL,property<CML,epicsUInt32>()}
};

static const
prop_entry<CML,epicsUInt16> props_epicsUInt16[] = {
  {"Mode", property<CML,epicsUInt16>(0, &CML::modeRaw, &CML::setModRaw)},
  {NULL,            property<CML,epicsUInt16>()}
};

static
CML* get_cml(const char* hwlink, std::string& propname)
{
  addr inst_addr;

  if (linkOptionsStore(eventdef, &inst_addr, hwlink, 0))
    throw std::runtime_error("Couldn't parse link string");

  EVR* card=getEVR<EVR>(inst_addr.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  CML* cml=card->cml(inst_addr.output);
  if(!cml)
    throw std::runtime_error("Failed to lookup CML output");

  propname=std::string(inst_addr.prop);

  return cml;
}

static long write_mbbo(mbboRecord* prec)
{
  if (!prec->dpvt) return -1;
try {
  property<CML,epicsUInt16> *priv=static_cast<property<CML,epicsUInt16>*>(prec->dpvt);

  switch(prec->rval){
  case cmlModeOrig:
  case cmlModeFreq:
  case cmlModePattern:
    priv->set((epicsUInt16)prec->rval);

    prec->rbv=priv->get();
    break;
  default:
    throw std::out_of_range("Invalid CML trigger mode");
  }

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

static long add_record_waveform(dbCommon *praw)
{
  waveformRecord *prec=(waveformRecord*)praw;
  long ret=0;
  CML* pcml=NULL;
try {
  assert(prec->inp.type==INST_IO);

  prec->dpvt=NULL;

  std::string parm;

  pcml=get_cml(prec->inp.value.instio.string, parm);

  // Ignore parm for the moment

  // prec->dpvt is set again to indicate
  // This also serves to indicate successful
  // initialization to other dset functions
  setdpvt(prec, pcml);

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

static long get_ioint_info_waveform(int dir,dbCommon* prec,IOSCANPVT* io)
{
if (!prec->dpvt) return -1;
try {
  CML *pcml=static_cast<CML*>(prec->dpvt);

  *io = pcml->statusChange();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

static long write_waveform(waveformRecord* prec)
{
  if (!prec->dpvt) return -1;
try {
  CML *pcml=static_cast<CML*>(prec->dpvt);

  unsigned char *buf=static_cast<unsigned char*>(prec->bptr);
  epicsUInt32 blen=pcml->lenPatternMax();

  blen=blen > prec->nord ? prec->nord : blen;

  switch(prec->ftvl) {
  case menuFtypeUCHAR:
    break;
  default:
    throw std::out_of_range("Invalid element type");
  }

  pcml->setPattern(buf, blen);

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

static long read_waveform(waveformRecord* prec)
{
  if (!prec->dpvt) return -1;
try {
  CML *pcml=static_cast<CML*>(prec->dpvt);

  unsigned char *buf=static_cast<unsigned char*>(prec->bptr);
  epicsUInt32 blen=prec->nelm;

  pcml->getPattern(buf, &blen);
  prec->nord = blen;

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  prec->nord=0;
  return S_db_noMemory;
}
}

/*************** boiler plate *******************/

extern "C" {

PROPERTY_DSET_LONGIN (CML, get_cml, props_epicsUInt32);
PROPERTY_DSET_LONGOUT(CML, get_cml, props_epicsUInt32);

PROPERTY_DSET_BI(CML, get_cml, props_bool);
PROPERTY_DSET_BO(CML, get_cml, props_bool);

// impliments add_CML_mbbo
PROPERTY_DSET_ADDFN(mbbo, epicsUInt16, out, CML, get_cml, props_epicsUInt16);
PROPERTY_DSET_TABLE(mbbo, epicsUInt16, CML,
                    add_CML_mbbo,
                    init_record_empty,
                    write_mbbo) 

dsxt dxtwaveformCML={add_record_waveform,del_record_empty};

static common_dset devwaveforminCML = {
  6, NULL,
  dset_cast(&init_dset<&dxtwaveformCML>),
  (DEVSUPFUN) init_record_empty,
  dset_cast(&get_ioint_info_waveform),
  dset_cast(&read_waveform),
  NULL };
epicsExportAddress(dset,devwaveforminCML);

static common_dset devwaveformoutCML = {
  6, NULL,
  dset_cast(&init_dset<&dxtwaveformCML>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&write_waveform),
  NULL };
epicsExportAddress(dset,devwaveformoutCML);

}
