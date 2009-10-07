
#ifndef EVRNULL_H_INC
#define EVRNULL_H_INC

#include "evr/evr.h"

#include <string>

#include <dbScan.h>

/**@brief An implimentation of the EVR interface which does nothing.
 *
 * A simple implimentation of EVR which maintains some state,
 * and prints to screen.
 */
class EVRNull : public EVR
{
public:
  EVRNull(const char*);

  virtual ~EVRNull(){};

  virtual epicsUInt32 model() const{return 0x12345678;};

  virtual epicsUInt32 version() const{return 0x87654321;};

  virtual bool enabled() const{return m_enabled;};
  virtual void enable(bool v);

  virtual Pulser* pulser(epicsUInt32){return 0;};
  virtual const Pulser* pulser(epicsUInt32) const{return 0;};

  virtual Output* output(epicsUInt32){return 0;};
  virtual const Output* output(epicsUInt32) const{return 0;};

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
}; // class EVRNull

#endif // EVRNULL_H_INC
