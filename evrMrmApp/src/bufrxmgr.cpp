/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */



#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#include <errlog.h>
#include <epicsTypes.h>
#include <cantProceed.h>
#include <dbDefs.h>
#include "mrf/databuf.h"


#include <epicsExport.h>
#include "bufrxmgr.h"

/* Subtract member byte offset, returning pointer to parent object */
#ifndef CONTAINER
# ifdef __GNUC__
#   define CONTAINER(ptr, structure, member) ({                     \
        const __typeof(((structure*)0)->member) *_ptr = (ptr);      \
        (structure*)((char*)_ptr - offsetof(structure, member));    \
    })
# else
#   define CONTAINER(ptr, structure, member) \
        ((structure*)((char*)(ptr) - offsetof(structure, member)))
# endif
#endif

#define CBINIT(ptr, prio, fn, valptr) \
do { \
  callbackSetPriority(prio, ptr); \
  callbackSetCallback(fn, ptr);   \
  callbackSetUser(valptr, ptr);   \
  (ptr)->timer=NULL;              \
} while(0)

void defaulterr(void *, epicsStatus err,
                epicsUInt32 , const epicsUInt8* )
{
    switch(err) {
    case 0: break; // no error
    case 1: errlogPrintf("Data buffer Rx rate too high, dropped buffers\n"); break;
    case 2: errlogPrintf("Data buffer Rx checksum error\n"); break;
    default: errlogPrintf("Data buffer Rx error %d\n",err);
    }
}

bufRxManager::bufRxManager(const std::string& n, unsigned int qdepth, unsigned int bsize)
  :dataBufRx(n)
  ,guard()
  ,onerror(defaulterr)
  ,onerror_arg(NULL)
  ,m_bsize(bsize ? bsize : 2048)
{
    ellInit(&dispatch);
    ellInit(&freebufs);
    ellInit(&usedbufs);

    CBINIT(&received_cb, priorityMedium, &bufRxManager::received, this);

    for(unsigned int i=0; i<qdepth; i++) {
        buffer *t=(buffer*)callocMustSucceed(1, sizeof(buffer)-1+m_bsize, "bufRxManager buffer");
        ellAdd(&freebufs, &t->node);
    }
}

bufRxManager::~bufRxManager()
{
    ELLNODE *node, *next;

    SCOPED_LOCK(guard);

    for(node=ellFirst(&freebufs), next=node ? ellNext(node):NULL;
        node;
        node=next, next=next ? ellNext(next) : NULL)
    {
        buffer *b=CONTAINER(node, buffer, node);
        free(b);
    }
}

epicsUInt8*
bufRxManager::getFree(unsigned int* blen)
{
    ELLNODE *node;
    {
        SCOPED_LOCK(guard);
        node=ellGet(&freebufs);
    }

    if (!node)
        return NULL;

    buffer *buf=CONTAINER(node, buffer, node);

    if (blen) *blen=bsize();

    buf->used=0;
    return buf->data;
}

void
bufRxManager::receive(epicsUInt8* raw,unsigned int usedlen)
{
    /* CONTAINER doesn't work when the member is a pointer
     * because the GNU version's check isn't correct
    buffer *buf=CONTAINER(raw, buffer, data);
     */
    buffer *buf=(buffer*)((char*)(raw) - offsetof(buffer, data));

    if (usedlen>bsize())
        throw std::out_of_range("User admitted overflowing Rx buffer");
    buf->used=usedlen;

    if (usedlen==0) {
        // buffer returned w/o being used
        {
            SCOPED_LOCK(guard);
            ellAdd(&freebufs, &buf->node);
        }
        return;
    }

    {
        SCOPED_LOCK(guard);
        ellAdd(&usedbufs, &buf->node);
    }

    callbackRequest(&received_cb);
}

void
bufRxManager::received(CALLBACK* cb)
{
    void *vptr;
    callbackGetUser(vptr,cb);
    bufRxManager &self=*static_cast<bufRxManager*>(vptr);

    SCOPED_LOCK2(self.guard, G);

    while(true) {
        ELLNODE *node=ellGet(&self.usedbufs);

        if (!node)
            break;
        buffer *buf=CONTAINER(node, buffer, node);

        G.unlock();

        for(ELLNODE *cur=ellFirst(&self.dispatch); cur; cur=ellNext(cur)) {
            listener *action=CONTAINER(cur, listener, node);
            (action->fn)(action->fnarg, 0, buf->used, buf->data);
        }

        G.lock();

        ellAdd(&self.freebufs, &buf->node);
    };
}

void
bufRxManager::dataRxError(dataBufComplete fn, void* arg)
{
    SCOPED_LOCK(guard);
    onerror=fn;
    onerror_arg=arg;
}

void
bufRxManager::dataRxAddReceive(dataBufComplete fn,void* arg)
{
    listener *l;

    SCOPED_LOCK(guard);

    for(ELLNODE *node=ellFirst(&dispatch); node; node=ellNext(node))
    {
        l=CONTAINER(node, listener, node);
        // Don't add duplicates
        if (l->fn==fn && l->fnarg==arg) {
            return;
        }
    }

    l=new listener;
    l->fn=fn;
    l->fnarg=arg;

    ellAdd(&dispatch, &l->node);
}

void
bufRxManager::dataRxDeleteReceive(dataBufComplete fn,void* arg)
{
    listener *l;

    SCOPED_LOCK(guard);

    for(ELLNODE *node=ellFirst(&dispatch); node; node=ellNext(node))
    {
        l=CONTAINER(node, listener, node);
        if (l->fn==fn && l->fnarg==arg) {
            ellDelete(&dispatch, node);
            delete l;
            return;
        }
    }
}
