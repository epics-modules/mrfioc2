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

#include <compilerDependencies.h>
#include <epicsThread.h>
#include <epicsTypes.h>

#ifndef EPICS_UNUSED
#  define EPICS_UNUSED
#endif

#include "mrfCommon.h"

// when dset should signal alarm w/o printing a message
class MRFCOMMON_API alarm_exception : public std::exception
{
    const short sevr, stat;
public:
    explicit alarm_exception(short sevr = INVALID_ALARM, short stat = COMM_ALARM) : sevr(sevr), stat(stat) {}
    virtual ~alarm_exception() throw() {}
    virtual const char *what() throw();
    inline short severity() const { return sevr; }
    inline short status() const { return stat; }
};

namespace mrf {

//! @brief Requested operation is not implemented by the property
class MRFCOMMON_API opNotImplemented : public std::runtime_error
{
public:
    explicit opNotImplemented(const std::string& m) : std::runtime_error(m) {}
};

/** @brief An un-typed property.
 *
 * Required to implement Object::getProperty<P>().
 * There is no way other then to up-cast in ObjectInst<C>
 * and then down-cast in getProperty<P>().
 */
struct MRFCOMMON_API propertyBase
{
    virtual ~propertyBase()=0;
    virtual const std::type_info& type() const=0;
    virtual propertyBase* clone() const =0;
    //! @brief Print the value of the field w/o leading or trailing whitespace
    virtual void  show(std::ostream&) const;
};

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

//! @brief final scalar implementation
template<class C, typename P>
class propertyInstance : public property<P>
{
public:
  typedef void (C::*setter_t)(P);
  typedef P    (C::*getter_t)() const;
private:
  C *inst;
  getter_t const getter;
  setter_t const setter;
public:

  propertyInstance(C* c, getter_t g, setter_t s)
    :inst(c)
    ,getter(g)
    ,setter(s)
  {}
  virtual ~propertyInstance() {}

  virtual const std::type_info& type() const OVERRIDE {return typeid(P);}
  virtual propertyInstance* clone() const OVERRIDE { return new propertyInstance(inst, getter, setter); }
  virtual void set(P v) OVERRIDE
  {
      if(!setter)
          throw opNotImplemented("void set(T) not implemented");
      (inst->*(setter))(v);
  }
  virtual P get() const OVERRIDE {
      if(!getter)
          throw opNotImplemented("T get() not implemented");
      return (inst->*(getter))();
  }
  virtual void show(std::ostream& strm) const OVERRIDE
  {
      strm<<get();
  }
};

//! @brief final array implementation
template<class C, typename P>
class propertyInstance<C,P[1]> : public property<P[1]>
{
public:
    typedef void   (C::*setter_t)(const P*, epicsUInt32);
    typedef epicsUInt32 (C::*getter_t)(P*, epicsUInt32) const;
private:
  C *inst;
  getter_t const getter;
  setter_t const setter;
public:

  propertyInstance(C* c, getter_t g, setter_t s)
    :inst(c)
    ,getter(g)
    ,setter(s)
  {}
  virtual ~propertyInstance() {}

  virtual const std::type_info& type() const OVERRIDE {return typeid(P[1]);}
  virtual propertyInstance* clone() const OVERRIDE { return new propertyInstance(inst, getter, setter); }
  virtual void   set(const P* a, epicsUInt32 l) OVERRIDE
    { (inst->*(setter))(a,l); }
  virtual epicsUInt32 get(P* a, epicsUInt32 l) const OVERRIDE
    { return (inst->*(getter))(a,l); }
};

template<class C>
class propertyInstance<C,void> : public property<void>
{
public:
    typedef void (C::*exec_t)();
private:
    C *inst;
    exec_t const execer;
public:

    propertyInstance(C *c, exec_t e)
        :inst(c), execer(e) {}
    virtual ~propertyInstance() {}

    virtual const std::type_info& type() const OVERRIDE {return typeid(void);}
    virtual propertyInstance* clone() const OVERRIDE { return new propertyInstance(inst, execer); }
    virtual void exec() OVERRIDE {
        (inst->*execer)();
    }
};

template<class C, typename P>
propertyInstance<C,P>* makePropertyInstance(C* self, P (C::*getter)() const, void (C::*setter)(P) =0)
{
    return new propertyInstance<C, P>(self, getter, setter);
}

template<class C>
propertyInstance<C,void>* makePropertyInstance(C* self, void (C::*execr)())
{
    return new propertyInstance<C, void>(self, execr);
}

template<class C, typename P>
propertyInstance<C,P[1]>* makePropertyInstance(C* self,
                                           epicsUInt32 (C::*getter)(P*, epicsUInt32) const,
                                           void   (C::*setter)(const P*, epicsUInt32) =0)
{
    return new propertyInstance<C, P[1]>(self, getter, setter);
}

} // namespace detail

/** @brief Base object inspection
 *
 * Interface for introspection operations.  Allows access
 * to properties.
 */

class MRFCOMMON_API Object
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
    mrf::auto_ptr<property<P> > getProperty(const char* pname)
    {
        propertyBase *b=getPropertyBase(pname, typeid(P));
        if(!b)
            return mrf::auto_ptr<property<P> >();
        property<P> *p=dynamic_cast<property<P> *>(b);
        if(!p)
            return mrf::auto_ptr<property<P> >();
        return mrf::auto_ptr<property<P> >(reinterpret_cast<property<P>*>(p));
    }

    virtual void visitProperties(bool (*)(const char*, propertyBase*, void*), void*)=0;

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
     OBJECT_DECL(mycls)
     mycls(const std::string& name) : ObjectInst<mycls>(name) {
        OBJECT_INIT;
     }
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
    // not copyable
    ObjectInst(const ObjectInst&);
    ObjectInst& operator=(const ObjectInst&);
public:
    typedef std::multimap<std::string, propertyBase*> m_props_t;
protected:
    m_props_t m_props;
    explicit ObjectInst(const std::string& n) : Base(n) {}
    template<typename A>
    ObjectInst(const std::string& n, A& a) : Base(n, a) {}
    virtual ~ObjectInst(){}
public:

    virtual propertyBase* getPropertyBase(const char* pname, const std::type_info& ptype)
    {
        typename m_props_t::const_iterator it=m_props.lower_bound(pname),
                                          end=m_props.upper_bound(pname);
        for(;it!=end;++it) {
            if(it->second->type()==ptype)
                return it->second->clone();
        }
        // continue checking for Base class properties
        return Base::getPropertyBase(pname, ptype);
    }

    virtual void visitProperties(bool (*cb)(const char*, propertyBase*, void*), void* arg)
    {
        mrf::auto_ptr<propertyBase> cur;
        for(typename m_props_t::const_iterator it=m_props.begin();
            it!=m_props.end(); ++it)
        {
            if(!(*cb)(it->first.c_str(), it->second, arg))
                break;
        }
        Base::visitProperties(cb, arg);
    }
};

#define OBJECT_DECL(klass) static void initObject(klass*, m_props_t *);

#define OBJECT_INIT initObject(this, &this->m_props)

#define OBJECT_BEGIN2(klass, Base) \
void klass::initObject(klass *self, mrf::ObjectInst<klass, Base>::m_props_t *props) { \
    const char *klassname = #klass; (void)klassname; \
    using namespace mrf; \
    try { \
        assert(self && props);

#define OBJECT_BEGIN(klass) OBJECT_BEGIN2(klass, mrf::Object)

#define OBJECT_PROP1(NAME, GET) \
    props->insert(std::make_pair((NAME), detail::makePropertyInstance(self, GET) ))

#define OBJECT_PROP2(NAME, GET, SET) \
    props->insert(std::make_pair((NAME), detail::makePropertyInstance(self, GET, SET) ))

#define OBJECT_FACTORY(FN) Object::addFactory(klassname, FN)

#define OBJECT_END(klass) \
} catch(std::exception& e) { \
std::cerr<<"Failed to build property table for "<<typeid(klass).name()<<"\n"<<e.what()<<"\n"; \
throw std::runtime_error("Failed to build"); \
  }}

} // namespace mrf

#endif // MRFOBJECT_H
