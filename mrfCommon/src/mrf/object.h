/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/**
 * @author Michael Davidsaver <mdavidsaver@bnl.gov>
 *
 * Defines a scheme for working with "properties".
 * A property being a pair of setter and getter methods
 * used to move a value in and out of an otherwise opaque
 * object.
 *
 * Properties are associated with a class by a property name
 * which can be used to retrieve a property.
 *
 @internal
 *
 * Properties are stored unbound (not associated with an instance).
 * Currently a new bound property is allocated for each request.
 */
#ifndef MRFOBJECT_H
#define MRFOBJECT_H

#include <ostream>
#include <sstream>
#include <map>
#include <set>
#include <cstring>
#include <string>
#include <memory>
#include <stdexcept>
#include <typeinfo>

#include <epicsThread.h>
#include <epicsTypes.h>

#include "mrfCommon.h"

namespace mrf {

struct info_t{
    std::string position;
    epicsUInt32 address;
    epicsUInt32 irqVector;
    epicsUInt32 irqLevel;
    epicsUInt32 vendor;
    epicsUInt32 board;
    epicsUInt32 revision;
};

//! @brief Requested operation is not implemented by the property
class opNotImplemented : public std::runtime_error
{
public:
    opNotImplemented(const std::string& m) : std::runtime_error(m) {}
};

/** @brief An un-typed property.
 *
 * Required to implement Object::getProperty<P>().
 * There is no way other then to up-cast in ObjectInst<C>
 * and then down-cast in getProperty<P>().
 */
struct propertyBase
{
    virtual ~propertyBase()=0;
    virtual const char* name() const=0;
    virtual const std::type_info& type() const=0;
    //! @brief Print the value of the field w/o leading or trailing whitespace
    virtual void  show(std::ostream&) const;
};

static inline
bool operator==(const propertyBase& a, const propertyBase& b)
{
    return a.type()==b.type() && strcmp(a.name(),b.name())==0;
}
static inline
bool operator!=(const propertyBase& a, const propertyBase& b)
{ return !(a==b); }

//! @brief A bound, typed scalar property
template<typename P>
struct property : public propertyBase
{
    virtual ~property(){}
    //! @brief The setter for this property
    virtual void set(P)=0;
    virtual P    get() const=0;
};

//! @brief A bound, typed array property
template<typename P>
struct property<P[1]> : public propertyBase
{
    virtual ~property(){}
    /** @brief The setter for this property
     *
     @param arr Array to copy from
     @param L   Number of element in @var arr
     */
    virtual void set(const P* arr, epicsUInt32 L)=0;
    /** @brief The getter for this property
     *
     @param arr Array to copy to
     @param L   Max number of element to write to @var arr
     @returns The number of elements actual written
     */
    virtual epicsUInt32 get(P*, epicsUInt32) const=0;
};

namespace detail {

/** @brief An un-typed, un-bound property for class C
 *
 * This is the form in which properties are stored
 * by ObjectInst<T>.
 */
template<class C>
struct unboundPropertyBase
{
    virtual ~unboundPropertyBase(){};
    virtual const std::type_info& type() const=0;

    //! @brief Create a bound property with the given instance
    virtual propertyBase* bind(C*)=0;
};

//! @brief An un-bound, typed scalar property
template<class C, typename P>
class unboundProperty : public unboundPropertyBase<C>
{
public:
    typedef void (C::*setter_t)(P);
    typedef P    (C::*getter_t)() const;

    const char * const name;
    getter_t const getter;
    setter_t const setter;

    unboundProperty(const char* n, getter_t g, setter_t s)
        :name(n), getter(g), setter(s) {}

    virtual const std::type_info& type() const{return typeid(P);}
    inline virtual property<P>* bind(C*);
};

template<class C, typename P>
static inline
unboundProperty<C,P>*
makeUnboundProperty(const char* n, P (C::*g)() const, void (C::*s)(P)=0)
{
    return new unboundProperty<C,P>(n,g,s);
}

//! @brief An un-bound, typed array property
template<class C, typename P>
class unboundProperty<C,P[1]> : public unboundPropertyBase<C>
{
public:
    typedef void   (C::*setter_t)(const P*, epicsUInt32);
    typedef epicsUInt32 (C::*getter_t)(P*, epicsUInt32) const;

    const char * const name;
    getter_t const getter;
    setter_t const setter;

    unboundProperty(const char* n, getter_t g, setter_t s)
        :name(n), getter(g), setter(s) {}

    virtual const std::type_info& type() const{return typeid(P[1]);}
    inline virtual property<P[1]>* bind(C*);
};

template<class C, typename P>
static inline
unboundProperty<C,P[1]>*
makeUnboundProperty(const char* n,
                    epicsUInt32 (C::*g)(P*, epicsUInt32) const,
                    void (C::*s)(const P*, epicsUInt32)=0)
{
    return new unboundProperty<C,P[1]>(n,g,s);
}

//! @brief final scalar implementation
template<class C, typename P>
class propertyInstance : public property<P>
{
  C *inst;
  unboundProperty<C,P> prop;
public:

  propertyInstance(C* c, const unboundProperty<C,P>& p)
    :inst(c)
    ,prop(p)
  {}

  virtual const char* name() const{return prop.name;}
  virtual const std::type_info& type() const{return typeid(P);}
  virtual void set(P v)
  {
      if(!prop.setter)
          throw opNotImplemented("void set(T) not implemented");
      (inst->*(prop.setter))(v);
  }
  virtual P get() const{
      if(!prop.getter)
          throw opNotImplemented("T get() not implemented");
      return (inst->*(prop.getter))();
  }
  virtual void show(std::ostream& strm) const
  {
      strm<<get();
  }
};

//! Binder for scalar instances
template<class C, typename P>
property<P>*
unboundProperty<C,P>::bind(C* inst)
{
    return new propertyInstance<C,P>(inst,*this);
}

//! @brief final array implementation
template<class C, typename P>
class propertyInstance<C,P[1]> : public property<P[1]>
{
  C *inst;
  unboundProperty<C,P[1]> prop;
public:

  propertyInstance(C* c, const unboundProperty<C,P[1]>& p)
    :inst(c)
    ,prop(p)
  {}

  virtual const char* name() const{return prop.name;}
  virtual const std::type_info& type() const{return typeid(P[1]);}
  virtual void   set(const P* a, epicsUInt32 l)
    { (inst->*(prop.setter))(a,l); }
  virtual epicsUInt32 get(P* a, epicsUInt32 l) const
    { return (inst->*(prop.getter))(a,l); }
};

//! Binder for scalar instances
template<class C, typename P>
property<P[1]>*
unboundProperty<C,P[1]>::bind(C* inst)
{
    return new propertyInstance<C,P[1]>(inst,*this);
}

} // namespace detail

/** @brief Base object inspection
 *
 * Interface for introspection operations.  Allows access
 * to properties.
 */
class Object
{
public:
    struct _compName {
        bool operator()(const Object* a, const Object* b) const{return a->name()<b->name();}
    };
private:
    const std::string m_obj_name;
    const Object * const m_obj_parent;
    typedef std::set<Object*,_compName> m_obj_children_t;
    mutable m_obj_children_t m_obj_children;
protected:
    Object(const std::string& n, const Object *par=0);
    virtual ~Object()=0;
public:
    const std::string& name() const{return m_obj_name;}
    const Object* parent() const{return m_obj_parent;}

    virtual void lock() const =0;
    virtual void unlock() const =0;

    typedef m_obj_children_t::const_iterator child_iterator;
    child_iterator beginChild() const{return m_obj_children.begin();}
    child_iterator endChild() const{return m_obj_children.end();}

    virtual propertyBase* getPropertyBase(const char*, const std::type_info&)=0;
    template<typename P>
    std::auto_ptr<property<P> > getProperty(const char* pname)
    {
        propertyBase *b=getPropertyBase(pname, typeid(P));
        if(!b)
            return std::auto_ptr<property<P> >();
        property<P> *p=dynamic_cast<property<P> *>(b);
        if(!p)
            return std::auto_ptr<property<P> >();
        return std::auto_ptr<property<P> >(p);
    }

    virtual void visitProperties(bool (*)(propertyBase*, void*), void*)=0;

    static Object* getObject(const std::string&);

    static void visitObjects(bool (*)(Object*, void*), void*);
};

/** @brief User implementation hook
 *
 * Used to implement properties in a user class.
 @code
   class mycls : public ObjectInst<mycls> {
     ...
     int getint() const;
     void setint(int);
   };
 @endcode
 * Each user class must define an Object table.
 * This should be done once, usually with its
 * method definitions.
 @code
   ...
   void mycls::setint(int) {...}
   ...
   OBJECT_BEGIN(mycls)
   OBJECT_PROP("propname", &mycls::getint, &mycls::setint)
   ...
   OBJECT_END(mycls)
 @endcode
 */
template<class C>
class ObjectInst : public Object
{
    typedef std::multimap<std::string, detail::unboundPropertyBase<C>*> m_props_t;
    static m_props_t *m_props;
    static void initObject(void*);
    static epicsThreadOnceId initId;
protected:
    ObjectInst(const std::string& n) : Object(n) {}
    virtual ~ObjectInst(){};
public:

    virtual propertyBase* getPropertyBase(const char* pname, const std::type_info& ptype)
    {
        std::string emsg;
        epicsThreadOnce(&initId, &initObject, (void*)&emsg);
        if(!m_props)
            throw std::runtime_error(emsg);
        typename m_props_t::const_iterator it=m_props->lower_bound(pname),
                                          end=m_props->upper_bound(pname);
        for(;it!=end;++it) {
            if(it->second->type()==ptype)
                break;
        }
        if(it==end)
            return 0;
        return it->second->bind(static_cast<C*>(this));
    }

    void visitProperties(bool (*cb)(propertyBase*, void*), void* arg)
    {
        std::string emsg;
        epicsThreadOnce(&initId, &initObject, (void*)&emsg);
        if(!m_props)
            throw std::runtime_error(emsg);

        std::auto_ptr<propertyBase> cur;
        for(typename m_props_t::const_iterator it=m_props->begin();
            it!=m_props->end(); ++it)
        {
            cur.reset(it->second->bind(static_cast<C*>(this)));
            if(!cur.get())
                continue;
            if(!(*cb)(cur.get(), arg))
                break;
        }
    }
};

#define OBJECT_BEGIN(klass) namespace mrf {\
template<> ObjectInst<klass>::m_props_t* ObjectInst<klass>::m_props = 0; \
template<> epicsThreadOnceId ObjectInst<klass>::initId = EPICS_THREAD_ONCE_INIT; \
template<> void ObjectInst<klass>::initObject(void * rmsg) { \
    std::string *emsg=static_cast<std::string*>(rmsg); \
    try { std::auto_ptr<m_props_t> props(new m_props_t); {

#define OBJECT_PROP1(NAME, GET) \
    props->insert(std::make_pair(NAME, detail::makeUnboundProperty(NAME, GET) ))

#define OBJECT_PROP2(NAME, GET, SET) \
    props->insert(std::make_pair(NAME, detail::makeUnboundProperty(NAME, GET, SET) ))

#define OBJECT_END(klass) \
} m_props = props.release(); \
} catch(std::exception& e) { \
std::ostringstream strm; \
strm<<"Failed to build property table for "<<typeid(klass).name()<<"\n"<<e.what()<<"\n"; \
*emsg=strm.str(); \
  }}}

} // namespace mrf

#endif // MRFOBJECT_H
