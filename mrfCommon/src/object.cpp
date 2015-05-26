#include <sstream>
#include <iostream>
#include <errlog.h>
#include <epicsMutex.h>
#include <epicsGuard.h>

#include <epicsExport.h>
#include "mrf/object.h"

using namespace mrf;

typedef std::map<const std::string, Object*> objects_t;
static objects_t *objects=0;

static epicsMutex *objectsLock=0;

static
void initObjects(void* rmsg)
{
    std::string *emsg=(std::string*)rmsg;
    try{
        objects = new objects_t;
        objectsLock = new epicsMutex;
    } catch(std::exception& e) {
        objects=0;
        *emsg = e.what();
    }
}

static
epicsThreadOnceId initOnce = EPICS_THREAD_ONCE_INIT;

static
void initObjectsOnce()
{
    std::string emsg;
    epicsThreadOnce(&initOnce, &initObjects, (void*)&emsg);
    if(!objects)
        throw std::runtime_error(emsg);
}

epicsShareFunc
propertyBase::~propertyBase() {}

epicsShareFunc
void
propertyBase::show(std::ostream& strm) const
{
    strm<<"<?>";
}

Object::Object(const std::string& n, const Object *par)
    :m_obj_name(n)
    ,m_obj_parent(par)
    ,m_obj_children()
{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);

    if(n.size()==0)
        throw std::invalid_argument("Object name can not be empty string");

    objects_t::const_iterator it=objects->find(n);
    if(it!=objects->end()) {
        std::ostringstream strm;
        strm<<"Object name '"<<n<<"' already exists.";
        throw std::runtime_error(strm.str());
    }
    (*objects)[n]=this;

    if(m_obj_parent)
        m_obj_parent->m_obj_children.insert(this);
}

Object::~Object()
{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);

    if(m_obj_parent)
        m_obj_parent->m_obj_children.erase(this);

    objects_t::iterator it=objects->find(name());
    if(it==objects->end())
        errlogPrintf("not deleting: Object '%s' not in global list?\n", name().c_str());
    else
        objects->erase(it);
}

Object*
Object::getObject(const std::string& n)
{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);
    objects_t::const_iterator it=objects->find(n);
    if(it==objects->end())
        return 0;
    return it->second;
}

void
Object::visitObjects(bool (*cb)(Object*, void*), void *arg)
{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);

    for(objects_t::const_iterator it=objects->begin();
            it!=objects->end(); ++it)
    {
        if(!(*cb)(it->second, arg))
            break;
    }
}

struct propArgs {
    std::ostream& strm;
    std::string indent;
    propArgs(std::ostream& s, std::string i) :strm(s), indent(i) {}
};

static
bool showProp(propertyBase* prop, void* raw)
{
    propArgs *args=(propArgs*)raw;
    args->strm <<args->indent <<prop->type().name() << " " <<prop->name() << " = ";
    try {
        prop->show(args->strm);
    } catch (std::exception& e) {
        args->strm << "<Error: "<<e.what()<<">";
    }

    args->strm<<"\n";
    return true;
}

static
void showObject(std::ostream& strm, Object& obj, std::string indent, int depth, int maxdepth, bool props)
{
    if(depth>=maxdepth)
        return;
    propArgs args(strm, indent+"  ");
    strm <<indent <<"Object: " <<obj.name() <<"\n";
    if(props)
        obj.visitProperties(&showProp, (void*)&args);
    for(Object::child_iterator it=obj.beginChild(); it!=obj.endChild(); ++it)
        showObject(strm, **it, args.indent, depth+1, maxdepth, props);
}

extern "C"
void dol(int lvl, const char* obj)
{
try{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);

    std::cout <<objects->size() <<" Device Objects\n";

    if(!obj) {
        for(objects_t::const_iterator it=objects->begin();
        it!=objects->end(); ++it)
        {
            if(it->second->parent())
                continue;
            showObject(std::cout, *it->second, "", 0, lvl+1, false);
        }

    } else {
        objects_t::const_iterator it=objects->find(obj);
        if(it==objects->end()) {
            std::cout<<"Object '"<<obj<<"' does not exist\n";
            return;
        }
        showObject(std::cout, *it->second, "", 0, lvl+1, false);
    }
}catch(std::exception& e){
    epicsPrintf("Error: %s\n", e.what());
}
}

extern "C"
void dor(int lvl, const char* obj)
{
try{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);

    std::cout <<objects->size() <<" Device Objects\n";

    if(!obj) {
        for(objects_t::const_iterator it=objects->begin();
        it!=objects->end(); ++it)
        {
            if(it->second->parent())
                continue;
            showObject(std::cout, *it->second, "", 0, lvl+1, true);
        }

    } else {
        objects_t::const_iterator it=objects->find(obj);
        if(it==objects->end()) {
            std::cout<<"Object '"<<obj<<"' does not exist\n";
            return;
        }
        showObject(std::cout, *it->second, "", 0, lvl+1, true);
    }
}catch(std::exception& e){
    epicsPrintf("Error: %s\n", e.what());
}
}

#include <iocsh.h>

static const iocshArg dolArg0 = { "level",iocshArgInt};
static const iocshArg dolArg1 = { "object name",iocshArgString};
static const iocshArg * const dolArgs[2] =
{&dolArg0,&dolArg1};
static const iocshFuncDef dolFuncDef =
    {"dol",2,dolArgs};
static void dolCallFunc(const iocshArgBuf *args)
{
    dol(args[0].ival, args[1].sval);
}

static const iocshArg dorArg0 = { "level",iocshArgInt};
static const iocshArg dorArg1 = { "object name",iocshArgString};
static const iocshArg * const dorArgs[2] =
{&dorArg0,&dorArg1};
static const iocshFuncDef dorFuncDef =
    {"dor",2,dorArgs};
static void dorCallFunc(const iocshArgBuf *args)
{
    dor(args[0].ival, args[1].sval);
}

static
void objectsreg()
{
    iocshRegister(&dolFuncDef,dolCallFunc);
    iocshRegister(&dorFuncDef,dorCallFunc);
}

#include <epicsExport.h>

extern "C" {
epicsExportRegistrar(objectsreg);
}
