/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/**
 * @author Michael Davidsaver <mdavidsaver@gmail.com>
 *
 * Defines a scheme for working with "properties".
 * A property being a some methods (eg. pair of setter and getter)
 * used to move a value in and out of an otherwise opaque
 * object.
 *
 * Properties are associated with a class by a property name
 * which can be used to retrieve a property.
 *
 * Method signatures supported
 *
 * get/set of scalar value P
 *
 *   void klass::setter(P v);
 *   P klass::getter() const;
 *
 * get/set of array value P
 *
 *   void klass::setter(const P* vals, epicsUInt32 nelem);
 *   epicsUInt32 klass::getter(P* vals, epicsUInt32 maxelem); // returns nelem
 *
 * A momentery/command
 *
 *   void klass::execer();
 *
 @internal
 *
 * Properties are stored unbound (not associated with an instance).
 * Currently a new bound property is allocated for each request.
 */
#ifndef MRFOBJECT_H
#define MRFOBJECT_H

#ifdef _WIN32
/*
 * The warnings C4251 and C4275 happen because all objects have a DLL
 * interface when compiled on VC++ but derive from standard STL objects
 * that don't have a DLL interface. This can be safely ignored when
 * deriving from STL objects but not otherwise. See [1] and [2] for
 * more information.
 *
 * [1]: https://msdn.microsoft.com/en-us/library/3tdb471s.aspx
 * [2]: https://msdn.microsoft.com/en-us/library/esew7y1w.aspx
 */
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )
/*
 * The warning C4661 is happening due to separate code instantiation
 * that happens with macros defined in this file (OBJECT_START,
 * OBJECT_PROP1, OBJECT_PROP2 and OBJECT_END). The objects define
 * custom instances of generic templates which means that in some code
 * files where these macros are missing the compiler assumes that
 * specific instances of functions are missing.
 */
#pragma warning( disable: 4661 )
#endif

// dmm: the old gcc v2.96 appears to have problems to find the "new" version of <ostream>
#if (defined __GNUC__ && __GNUC__ < 3)
	#include <ostream.h>
#else
	#include <ostream>
#endif
#include <iostream>
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

//! @brief Requested operation is not implemented by the property
class epicsShareClass opNotImplemented : public std::runtime_error
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
struct epicsShareClass propertyBase
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

//! @brief A momentary/command
template<>
struct property<void> : public propertyBase
{
    virtual ~property(){}
    virtual void exec()=0;
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
class epicsShareClass unboundProperty : public unboundPropertyBase<C>
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
class epicsShareClass unboundProperty<C,P[1]> : public unboundPropertyBase<C>
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

//! @brief An un-bound momentary/command
template<class C>
class epicsShareClass unboundProperty<C,void> : public unboundPropertyBase<C>
{
public:
    typedef void (C::*exec_t)();

    const char * const name;
    exec_t const execer;

    unboundProperty(const char *n, exec_t e) :name(n), execer(e) {}
    virtual const std::type_info& type() const{return typeid(void);}
    inline virtual property<void>* bind(C*);
};

template<class C>
static inline
unboundProperty<C,void>*
makeUnboundProperty(const char* n,
                    void (C::*e)())
{
    return new unboundProperty<C,void>(n,e);
}

//! @brief final scalar implementation
template<class C, typename P>
class epicsShareClass  propertyInstance : public property<P>
{
  C *inst;
  unboundProperty<C,P> prop;
public:

  propertyInstance(C* c, const unboundProperty<C,P>& p)
    :inst(c)
    ,prop(p)
  {}
  virtual ~propertyInstance() {}

  virtual const char* name() const{return prop.name;}
  virtual const std::type_info& type() const{return prop.type();}
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
class epicsShareClass propertyInstance<C,P[1]> : public property<P[1]>
{
  C *inst;
  unboundProperty<C,P[1]> prop;
public:

  propertyInstance(C* c, const unboundProperty<C,P[1]>& p)
    :inst(c)
    ,prop(p)
  {}
  virtual ~propertyInstance() {}

  virtual const char* name() const{return prop.name;}
  virtual const std::type_info& type() const{return prop.type();}
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

template<class C>
class epicsShareClass propertyInstance<C,void> : public property<void>
{
    C *inst;
    unboundProperty<C,void> prop;
public:

    propertyInstance(C *c, const unboundProperty<C,void>& p)
        :inst(c), prop(p) {}
    virtual ~propertyInstance() {}

    virtual const char* name() const{return prop.name;}
    virtual const std::type_info& type() const{return prop.type();}
    virtual void exec() {
        (inst->*prop.execer)();
    }
};

//! Binder for momentary/command instances
template<class C>
property<void>*
unboundProperty<C,void>::bind(C* inst)
{
    return new propertyInstance<C,void>(inst,*this);
}

} // namespace detail

/** @brief Base object inspection
 *
 * Interface for introspection operations.  Allows access
 * to properties.
 */

class epicsShareClass Object
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

    //! Fetch named Object
    //! returns NULL if not found
    static Object* getObject(const std::string& name);

    typedef std::map<std::string, std::string> create_args_t;

    //! Fetch or or create named Object
    //! Throws an exception if creation fails
    static Object* getCreateObject(const std::string& name, const std::string& klass, const create_args_t& args = create_args_t());

    typedef Object* (*create_factory_t)(const std::string& name, const std::string& klass, const create_args_t& args);

    static void addFactory(const std::string& klass, create_factory_t fn);

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
template<class C, typename Base = Object>
class ObjectInst : public Base
{
    typedef std::multimap<std::string, detail::unboundPropertyBase<C>*> m_props_t;
    static m_props_t *m_props;
public:
    static int initObject();
protected:
    ObjectInst(const std::string& n) : Base(n) {}
    template<typename A>
    ObjectInst(const std::string& n, A a) : Base(n, a) {}
    virtual ~ObjectInst(){}
public:

    virtual propertyBase* getPropertyBase(const char* pname, const std::type_info& ptype)
    {
        std::string emsg;
        if(!m_props)
            throw std::runtime_error(emsg);
        typename m_props_t::const_iterator it=m_props->lower_bound(pname),
                                          end=m_props->upper_bound(pname);
        for(;it!=end;++it) {
            if(it->second->type()==ptype)
                return it->second->bind(static_cast<C*>(this));
        }
        // continue checking for Base class properties
        return Base::getPropertyBase(pname, ptype);
    }

    virtual void visitProperties(bool (*cb)(propertyBase*, void*), void* arg)
    {
        std::string emsg;
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
        Base::visitProperties(cb, arg);
    }
};

#define OBJECT_BEGIN2(klass, Base) namespace mrf {\
template<> ObjectInst<klass, Base>::m_props_t* ObjectInst<klass, Base>::m_props = 0; \
template<> int ObjectInst<klass, Base>::initObject() { \
    const char *klassname = #klass; (void)klassname; \
    try { std::auto_ptr<m_props_t> props(new m_props_t); {

#define OBJECT_BEGIN(klass) OBJECT_BEGIN2(klass, Object)

#define OBJECT_PROP1(NAME, GET) \
    props->insert(std::make_pair(static_cast<const char*>(NAME), detail::makeUnboundProperty(NAME, GET) ))

#define OBJECT_PROP2(NAME, GET, SET) \
    props->insert(std::make_pair(static_cast<const char*>(NAME), detail::makeUnboundProperty(NAME, GET, SET) ))

#define OBJECT_FACTORY(FN) addFactory(klassname, FN)

#define OBJECT_END(klass) \
} m_props = props.release(); return 1; \
} catch(std::exception& e) { \
std::cerr<<"Failed to build property table for "<<typeid(klass).name()<<"\n"<<e.what()<<"\n"; \
throw std::runtime_error("Failed to build"); \
  }}} \
static int done_##klass EPICS_UNUSED = klass::initObject();

} // namespace mrf

#endif // MRFOBJECT_H
