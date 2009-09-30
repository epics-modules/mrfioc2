
#ifndef PROPERTY_H_INC
#define PROPERTY_H_INC

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

  property(class_type* o,getter_t g, setter_t s=0, updater_t u=0) :
    inst(o), setter(s), getter(g),updater(u) {};

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

template<class C,typename P,typename S>
static long get_ioint_info(int dir,dbCommon* prec,IOSCANPVT* io)
{
  property<C,P,S> *prop=static_cast<property<C,P,S>*>(prec->dpvt);

  *io = prop->update();

  return 0;
}

#endif // PROPERTY_H_INC
