
#ifndef EVRNULL_H_INC
#define EVRNULL_H_INC

#include "evr/evr.h"

#include <string>
#include <set>
#include <map>
#include <utility>

#include <dbScan.h>

#include "outnull.h"

/**@brief An implimentation of the EVR interface which does nothing.
 *
 * A simple implimentation of EVR which maintains some state,
 * and prints to screen.
 *
 * codes and sub-units
 *  0 - invalid/disabled
 *  1->255 - event code mapping
 *  256->264 - distributed bus bits
 *  265->267 - prescalers (clock dividers)
 *  268->280 - outputs
 */
class EVRNull : public EVR
{
public:
  static inline epicsUInt32 id2code(epicsUInt32 i)
    {return i>=1 && i<=255 ? i : 0;};
  static inline epicsUInt32 id2dbus(epicsUInt32 i)
    {return i>=256 && i<=264 ? i-256 : 0;};
  static inline epicsUInt32 id2ps(epicsUInt32 i)
    {return i>=265 && i<=267 ? i-265 : 0;};
  static inline epicsUInt32 id2out(epicsUInt32 i)
    {return i>=268 && i<=280 ? i-268 : 0;};

public:
  EVRNull(const char*);

  virtual ~EVRNull();

  virtual epicsUInt32 model() const{return 0x12345678;};

  virtual epicsUInt32 version() const{return 0x87654321;};

  virtual bool enabled() const{return m_enabled;};
  virtual void enable(bool v);

  virtual Pulser* pulser(epicsUInt32){return 0;};
  virtual const Pulser* pulser(epicsUInt32) const{return 0;};

  virtual OutputNull* output(OutputType,epicsUInt32 o)
    {outmap_t::const_iterator it=outmap.find(o);
     return it!=outmap.end() ? it->second : NULL;
    };
  virtual const OutputNull* output(OutputType,epicsUInt32 o) const
    {outmap_t::const_iterator it=outmap.find(o);
     return it!=outmap.end() ? it->second : NULL;
    };

  virtual PreScaler* prescaler(epicsUInt32){return 0;};
  virtual const PreScaler* prescaler(epicsUInt32) const{return 0;};

  virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func) const;
  virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool);
  virtual const char* idName(epicsUInt32 src) const{return "Nothing";};


  virtual epicsUInt32 pllCtrl() const{return 0;};
  virtual void pllSetCtrl(epicsUInt32){};

  virtual bool pllLocked() const{return m_locked;};
  virtual IOSCANPVT pllChanged(){return pllNotify;};

  virtual epicsUInt32 eventClockDiv() const{return 0;};
  virtual void setEventClockDiv(epicsUInt32){};

  virtual IOSCANPVT recvError(){return 0;};
  virtual epicsUInt32 recvErrorCount() const{return 0;};

  virtual epicsUInt32 tsDiv() const{return 0;};
  virtual void setTsDiv(epicsUInt32){};

  virtual void tsLatch(){};
  virtual void tsLatchReset(){};
  virtual epicsUInt32 tsLatchSec() const{return 0;};
  virtual epicsUInt32 tsLatchCount() const{return 0;};

  virtual epicsUInt16 dbus() const{return 0;};

  virtual void enableHeartbeat(bool){};
  virtual IOSCANPVT heartbeatOccured(){return 0;};

private:
  bool m_enabled;
  bool m_locked;
  std::string m_name;
  IOSCANPVT pllNotify;

  typedef std::pair<epicsUInt32,epicsUInt32> smap_ent_t;
  typedef std::set<smap_ent_t> smap_t;
  smap_t smap;

  typedef std::map<epicsUInt32,OutputNull*> outmap_t;
  outmap_t outmap;
}; // class EVRNull

#endif // EVRNULL_H_INC
