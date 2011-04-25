
#include "mrf/object.h"

#include <sstream>
#include <cstdio>

#include <errlog.h>
#include <epicsMutex.h>
#include <epicsGuard.h>

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

Object::Object(const std::string& n)
    :m_obj_name(n)
{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);
    objects_t::const_iterator it=objects->find(n);
    if(it!=objects->end()) {
        std::ostringstream strm;
        strm<<"Object name '"<<n<<"' already exists.";
        throw std::runtime_error(strm.str());
    }
    (*objects)[n]=this;
}

Object::~Object()
{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);
    objects_t::iterator it=objects->find(name());
    if(it==objects->end())
        errlogPrintf("deleting Object '%s' not in global list?\n", name().c_str());
    else
        objects->erase(it);
}

propertyBase::~propertyBase() {}

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

extern "C"
void dol(int lvl)
{
try{
    initObjectsOnce();
    epicsGuard<epicsMutex> g(*objectsLock);

    printf("%lu Device Objects\n", (unsigned long)objects->size());

    for(objects_t::const_iterator it=objects->begin();
            it!=objects->end(); ++it)
    {
        printf("Object: %s\n", it->second->name().c_str());
    }
}catch(std::exception& e){
    printf("Error: %s\n", e.what());
}
}

#include <iocsh.h>

static const iocshArg dolArg0 = { "level",iocshArgInt};
static const iocshArg * const dolArgs[1] =
{&dolArg0};
static const iocshFuncDef dolFuncDef =
    {"dol",1,dolArgs};
static void dolCallFunc(const iocshArgBuf *args)
{
    dol(args[0].ival);
}

static
void objectsreg()
{
    iocshRegister(&dolFuncDef,dolCallFunc);
}

#include <epicsExport.h>

epicsExportRegistrar(objectsreg);
