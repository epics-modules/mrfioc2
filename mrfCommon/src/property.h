
#ifndef PROPERTY_H_INC
#define PROPERTY_H_INC

#include <stdexcept>

#include <devSup.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <dbCommon.h>
#include <recGbl.h>

#include <aoRecord.h>
#include <aiRecord.h>
#include <boRecord.h>
#include <biRecord.h>
#include <longoutRecord.h>
#include <longinRecord.h>

#include <menuConvert.h>

/**@brief Getter/Setter access to a pair of member functions
 *
 * Acts as an adapter for a pair of class member functions
 * of the form P a=x->get() and x->set(S) where S==P by default.
 *
 * It also presents a function IOSCANPVT p=x->update() which can
 * be used for notification if the value which would be returned
 * by x->get() has changed.
 *
 */
template<class C,typename P,typename S=P>
class property
{
public:
  typedef C class_type;
  typedef P reading_type;
  typedef S setting_type;

  typedef void (C::*setter_t)(S);
  typedef P (C::*getter_t)() const;
  typedef IOSCANPVT (C::*updater_t)();

  property() :
    inst(0), setter(0), getter(0),updater(0) {};

  property(class_type* o,getter_t g, setter_t s=0, updater_t u=0) :
    inst(o), setter(s), getter(g),updater(u) {};

  property(const property& o) :
    inst(o.inst), setter(o.setter), getter(o.getter), updater(o.updater) {};

  property(const property& o, class_type* i) :
    inst(i), setter(o.setter), getter(o.getter), updater(o.updater) {};

  property& operator=(const property& o)
  {
    inst=o.inst; setter=o.setter; getter=o.getter; updater=o.updater;
    return *this;
  }

  void set_instance(class_type* i) { inst=i; }

  bool valid() const { return !!inst; }

  P get() const
  {
    return (inst->*getter)();
  };

  void set(S v)
  {
    if(!setter) return;
    (inst->*setter)(v);
  };

  IOSCANPVT update()
  {
    if(!updater) return 0;
    return (inst->*updater)();
  };

private:
  class_type* inst;
  setter_t setter;
  getter_t getter;
  updater_t updater;
};

//! An entry in a list of properties
template<class C,typename P,typename S=P>
struct prop_entry {
  const char* name;
  property<C,P,S> prop;
};

//! Search a list of properties (terminated by {NULL, property()})
template<class C,typename P,typename S>
property<C,P,S>
find_prop(const prop_entry<C,P,S>* list, std::string& name, C* c)
{
  for(; list->name; list++) {
    if (name==list->name)
      return property<C,P,S>(list->prop, c);
  }
  return property<C,P,S>();
}

#endif // PROPERTY_H_INC
