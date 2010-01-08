
#ifndef INPUT_HPP_INC
#define INPUT_HPP_INC

#include "evr/util.h"

#include <epicsTypes.h>

enum TrigMode {
  TrigNone=0,
  TrigLevel=1,
  TrigEdge=2
};

class Input : public IOStatus
{
public:
  virtual ~Input()=0;

  //! Set mask of dbus bits are driven by this input
  virtual void dbusSet(epicsUInt16)=0;
  virtual epicsUInt16 dbus() const=0;

  //! Set active high/low when using level trigger mode
  virtual void levelHighSet(bool)=0;
  virtual bool levelHigh() const=0;

  //! Set active rise/fall when using edge trigger mode
  virtual void edgeRiseSet(bool)=0;
  virtual bool edgeRise() const=0;

  //! Set external (local) event trigger mode
  virtual void extModeSet(TrigMode)=0;
  virtual TrigMode extMode() const=0;

  //! Set the event code sent by an externel (local) event
  virtual void extEvtSet(epicsUInt32)=0;
  virtual epicsUInt32 extEvt() const=0;

  //! Set the backwards event trigger mode
  virtual void backModeSet(TrigMode)=0;
  virtual TrigMode backMode() const=0;

  //! Set the event code sent by an a backwards event
  virtual void backEvtSet(epicsUInt32)=0;
  virtual epicsUInt32 backEvt() const=0;


  /**\ingroup devhelp
   */
  /*@{*/
  void extModeSetraw(epicsUInt16 r){extModeSet((TrigMode)r);};
  epicsUInt16 extModeraw() const{return (TrigMode)extMode();};

  void backModeSetraw(epicsUInt16 r){backModeSet((TrigMode)r);};
  epicsUInt16 backModeraw() const{return (TrigMode)backMode();};
  /*@}*/
};

#endif /* INPUT_HPP_INC */
