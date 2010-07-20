
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <errlog.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <compilerDependencies.h>


#include "devLibPCIImpl.h"

/**@file devLibPCIOSD.c
 * @brief Userspace PCI access in Linux
 *
 * Full implementation of devLibPCI interface (search, mapping, ISR).
 *
 * Searching is general for all PCI devices (via proc and sysfs).
 *
 * MMIO and connecting ISRs requires a UIO kernel driver.
 *
 * See README.pci in this directory and
 * Documentation/DocBook/uio-howto.tmpl in the kernel source tree.
 *
 * Note on locking: When taking both pciLock and a device lock
 *                  always take pciLock first.
 */

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

/* The following defintions taken from RTEMS */
#define PCI_BASE_ADDRESS_SPACE         0x01  /* 0 = memory, 1 = I/O */
#define PCI_BASE_ADDRESS_SPACE_IO      0x01
#define PCI_BASE_ADDRESS_SPACE_MEMORY  0x00
#define PCI_BASE_ADDRESS_MEM_TYPE_MASK 0x06
#define PCI_BASE_ADDRESS_MEM_TYPE_32   0x00  /* 32 bit address */
#define PCI_BASE_ADDRESS_MEM_TYPE_1M   0x02  /* Below 1M */
#define PCI_BASE_ADDRESS_MEM_TYPE_64   0x04  /* 64 bit address */
#define PCI_BASE_ADDRESS_MEM_PREFETCH  0x08  /* prefetchable? */
#define PCI_BASE_ADDRESS_MEM_MASK      (~0x0fUL)
#define PCI_BASE_ADDRESS_IO_MASK      (~0x03UL)

/**@brief Info of a single PCI device
 *
 * Lifetime: Created in linuxDevPCIInit and free'd in linuxDevFinal
 *
 * Access after init is guarded by devLock
 */
struct osdPCIDevice {
    epicsPCIDevice dev; /* "public" data */

    /* result of mmap(), add offset before passing to user */
    volatile void *base[PCIBARCOUNT];
    /* offset from start of page to start of BAR */
    epicsUInt32    offset[PCIBARCOUNT];
    /* BAR length (w/o offset) */
    epicsUInt32    len[PCIBARCOUNT];
    volatile void *erom;
    epicsUInt32    eromlen;

    epicsUInt32 displayBAR[PCIBARCOUNT]; /* Raw PCI address */
    epicsUInt32 displayErom;

    char *linuxDriver;

    int fd; /* /dev/uio# */

    epicsMutexId devLock; /* guard access to isrs list */

    ELLNODE node;

    ELLLIST isrs; /* contains struct osdISR */
};
typedef struct osdPCIDevice osdPCIDevice;

#define dev2osd(dev) CONTAINER(dev, osdPCIDevice, dev)

struct osdISR {
    ELLNODE node;

    epicsThreadId waiter;
    pthread_t waiter_id; /* filled by waiter after start */
    enum {
        osdISRStarting=0, /* started, id not filled */
        osdISRRunning, /* id filled, normal operation */
        osdISRStopping, /* stop required */
        osdISRDone, /* thread done, can free resources */
    } waiter_status;

    osdPCIDevice *osd;

    EPICSTHREADFUNC fptr;
    void  *param;
};
typedef struct osdISR osdISR;

static
void isrThread(void*);

static
void stopIsrThread(osdISR *isr);

static
ELLLIST devices = {{NULL,NULL},0}; /* list of osdPCIDevices::node */

/* guard access to 'devices' list */
static
epicsMutexId pciLock=NULL;

static
long pagesize;

#define DEVLIST "/proc/bus/pci/devices"

#define BUSBASE "/sys/bus/pci/devices/0000:%02x:%02x.%1x/"

#define UIO     "uio:uio%u/"

#define fbad(FILE) ( feof(FILE) || ferror(FILE))

/* vsprintf() w/ allocation.  The result must be free'd!
 */
static
char*
vallocPrintf(const char *format, va_list args)
{
    char* ret=NULL;
    int size, size2;

    /* Take advantage of the fact that sprintf will tell us how much space to allocate */
    size=vsnprintf("",0,format,args);
    if (size<=0) {
        errlogPrintf("vaprintf: Failed to convert format '%s'\n",format);
        goto fail;
    }
    ret=malloc(size+1);
    if (!ret) {
        errlogPrintf("vaprintf: Failed to allocate memory for format '%s'\n",format);
        goto fail;
    }
    size2=vsprintf(ret,format,args);
    if (size!=size2) {
        errlogPrintf("vaprintf: Format yielded different size %d %d : %s\n",size,size2,format);
        goto fail;
    }

    return ret;
fail:
    free(ret);
    return NULL;
}

static
char*
allocPrintf(const char *format, ...) EPICS_PRINTF_STYLE(1,2);

static
char*
allocPrintf(const char *format, ...)
{
    char* ret;
    va_list args;
    va_start(args, format);
    ret=vallocPrintf(format,args);
    va_end(args);
    return ret;
}

/* Read a file containing only a hex number with the prefix '0x'.
 * This format is common in the /sys/ tree.
 */
static
unsigned long
vread_sysfs_hex(int *err, const char *fileformat, va_list args)
{
    unsigned long ret=0;
    int size;
    char *scratch=NULL;
    FILE *fd=NULL;

    if (*err) return ret;
    *err=1;

    scratch=vallocPrintf(fileformat, args);
    if (!scratch)
        goto done;

    fd=fopen(scratch, "r");
    if (!fd) {
        errlogPrintf("vread_sysfs_hex: Failed to open %s\n",fileformat);
        goto done;
    }
    size=fscanf(fd, "0x%8lx",&ret);
    if(size!=1 || ferror(fd)) {
        errlogPrintf("vread_sysfs_hex: Failed to read %s\n",fileformat);
        goto done;
    }

    *err=0;
done:
    if (fd) fclose(fd);
    free(scratch);
    return ret;
}

static
unsigned long
read_sysfs_hex(int *err, const char *fileformat, ...) EPICS_PRINTF_STYLE(2,3);

static
unsigned long
read_sysfs_hex(int *err, const char *fileformat, ...)
{
    unsigned long ret;
    va_list args;
    va_start(args, fileformat);
    ret=vread_sysfs_hex(err,fileformat,args);
    va_end(args);
    return ret;
}

static
int match_uio(const struct dirent *ent)
{
    unsigned int X;
    /* poor mans regexp... */
    return sscanf(ent->d_name, "uio:uio%u", &X)==1;
}

/* Each PCI device with a UIO instance attached to in should have
 * a directory like /sys/bus/pci/devices/xxxxxx/uio:uio#/...
 * and this call should return '#' it such a directory exists.
 */
static
int
find_uio_number(const struct osdPCIDevice* osd)
{
    int N, ret=-1;
    char *devdir;
    struct dirent **namelist=NULL;

    devdir=allocPrintf(BUSBASE, osd->dev.bus, osd->dev.device, osd->dev.function);
    if (!devdir)
        goto fail;

    N=scandir(devdir, &namelist, match_uio, alphasort );
    if (N<0) {
        errlogPrintf("find_uio_number: Search of %s failed\n",devdir);
        perror("scandir");
        goto fail;
    } else if (N==0) {
        errlogPrintf("No UIO driver associated with %u:%u.%u\n"
                     "Looked in %s",
                     osd->dev.bus, osd->dev.device, osd->dev.function,
                     devdir);
        goto fail;
    } else if (N>1) {
        errlogPrintf("Warning: More the one driver associated with  %u:%u.%u\n"
                     "Using %s/%s",
                     osd->dev.bus, osd->dev.device, osd->dev.function,
                     devdir, namelist[0]->d_name);
        goto fail;
    }
    /* N==1 */
    if (sscanf(namelist[0]->d_name, "uio:uio%u", &ret)!=1) {
        errlogPrintf("find_uio_number: Someone changed the match conditions and didn't update me!\n");
        ret=-1;
        goto fail;
    }

    /* ret set by sscanf */
fail:
    free(namelist);
    free(devdir);
    return ret;
}

static
int
open_uio(struct osdPCIDevice* osd)
{
    int uio, ret=1;
    char *devname=NULL;
    if (osd->fd!=-1) return 0;

    uio=find_uio_number(osd);
    if (uio<0) goto fail;

    devname=allocPrintf("/dev/uio%u", uio);
    if (!devname) goto fail;

    /* First try to open /dev/uio# */
    osd->fd=open(devname,O_RDWR);
    if (osd->fd==-1) {
        /* TODO: try to create? */
        perror("Failed to open UIO device file");
        errlogPrintf("Could not open device file %s.\n",devname);
        goto fail;
    }

    ret=0;
fail:
    free(devname);
    return ret;
}

static
void
close_uio(struct osdPCIDevice* osd)
{
    int i;

    for(i=0; i<PCIBARCOUNT; i++) {
        if (!osd->base[i]) continue;

        munmap((void*)osd->base[i], osd->offset[i]+osd->len[i]);
        osd->base[i]=NULL;
    }

    if (osd->fd!=-1) close(osd->fd);
    osd->fd=-1;
}

static
int linuxDevPCIInit(void)
{
    FILE* dlist=NULL;
    int line=0, colnum, i;
    osdPCIDevice *osd=NULL;
    pciLock = epicsMutexMustCreate();

    pagesize=sysconf(_SC_PAGESIZE);
    if (pagesize==-1) {
        perror("Failed to get pagesize");
        goto fail;
    }

    dlist=fopen(DEVLIST,"r");
    if (!dlist) {
        errlogPrintf("Failed to read device list : " DEVLIST " does not exist\n");
        goto fail;
    }

    while(1) {
        unsigned int bdf;
        unsigned int vendor_device;
        unsigned int irq;
        unsigned int bar;
        unsigned int blen;
        int fail=0;
        char dname[101];

        line++;colnum=1;

        osd=calloc(1, sizeof(osdPCIDevice));
        if (!osd) {
            errMessage(S_dev_noMemory, "Out of memory");
            goto fail;
        }
        osd->fd=-1;

        int matched=fscanf(dlist, "%4x %8x %2x",
                           &bdf, &vendor_device, &irq);
        if (matched==EOF || feof(dlist))
            break;
        if (matched!=3 || ferror(dlist)) {
            colnum+=matched;
            goto badline;
        }

        osd->dev.bus=bdf>>8;
        osd->dev.device=(bdf>>3)&0x1f;
        osd->dev.function=bdf&0x7;
        osd->dev.id.vendor=(vendor_device>>16)&0xffff;
        osd->dev.id.device=vendor_device&0xffff;
        osd->dev.irq=irq;
        osd->dev.id.sub_vendor=read_sysfs_hex(&fail, BUSBASE "subsystem_vendor",
                                              osd->dev.bus, osd->dev.device, osd->dev.function);
        osd->dev.id.sub_device=read_sysfs_hex(&fail, BUSBASE "subsystem_device",
                                              osd->dev.bus, osd->dev.device, osd->dev.function);
        osd->dev.id.pci_class= read_sysfs_hex(&fail, BUSBASE "class",
                                              osd->dev.bus, osd->dev.device, osd->dev.function);
        osd->dev.id.revision=0;

        if (fail) {
            errlogPrintf("Warning: Failed to read some attributes of PCI %u:%u.%u\n"
                         "         This may cause some searches to fail\n",
                         osd->dev.bus, osd->dev.device, osd->dev.function);
            fail=0;
        }

        /* Read BAR info */

        /* Base address */
        for (i=0; i<PCIBARCOUNT; i++) {
            colnum++;
            matched=fscanf(dlist,"%8x", &bar);
            if (matched!=1 || fbad(dlist)) goto badline;

            osd->dev.bar[i].ioport=(bar & PCI_BASE_ADDRESS_SPACE)==PCI_BASE_ADDRESS_SPACE_IO;
            if(osd->dev.bar[i].ioport){
                /* This BAR is I/O ports */
                osd->dev.bar[i].below1M=0;
                osd->dev.bar[i].addr64=0;
                osd->displayBAR[i] = bar&PCI_BASE_ADDRESS_IO_MASK;
            }else{
                /* This BAR is memory mapped */
                osd->dev.bar[i].below1M=!!(bar&PCI_BASE_ADDRESS_MEM_TYPE_1M);
                osd->dev.bar[i].addr64=!!(bar&PCI_BASE_ADDRESS_MEM_TYPE_64);
                osd->displayBAR[i] = bar&PCI_BASE_ADDRESS_MEM_MASK;
            }
            /* offset from start of page to start of BAR */
            osd->offset[i]=osd->displayBAR[i]&(pagesize-1);
        }

        colnum++;
        matched=fscanf(dlist,"%8x", &bar);
        if (matched!=1 || fbad(dlist)) goto badline;
        osd->displayErom = bar;

        /* region length */
        for (i=0; i<PCIBARCOUNT; i++) {
            colnum++;
            matched=fscanf(dlist,"%8x", &blen);
            if (matched!=1 || fbad(dlist)) goto badline;
            osd->len[i] = blen;
        }

        colnum++;
        matched=fscanf(dlist,"%8x", &blen);
        if (matched!=1 || fbad(dlist)) goto badline;
        osd->eromlen = blen;

        colnum++;
        if (!fgets(dname, NELEMENTS(dname), dlist)) goto badline;
        /* fgets always adds a null */

        osd->linuxDriver = epicsStrDup(dname);
        if (!osd->linuxDriver)
            errlogPrintf("Warning: Failed to copy driver name\n");

        osd->devLock = epicsMutexMustCreate();

        ellAdd(&devices, &osd->node);
        osd=NULL;

        continue;
    badline:
        errlogPrintf("Failed to parse line %u column %u of "DEVLIST"\n", line, colnum);
        if(osd) free(osd->linuxDriver);
        free(osd);
        goto fail;
    }

    return 0;
fail:
    if (dlist) fclose(dlist);
    epicsMutexDestroy(pciLock);
    return S_dev_badInit;
}

static
int linuxDevFinal(void)
{
    ELLNODE *cur, *next, *isrcur, *isrnext;
    osdPCIDevice *curdev=NULL;
    osdISR *isr;

    epicsMutexMustLock(pciLock);
    for(cur=ellFirst(&devices), next=cur ? ellNext(cur) : NULL;
        cur;
        cur=next, next=next ? ellNext(next) : NULL )
    {
        curdev=CONTAINER(cur,osdPCIDevice,node);

        epicsMutexMustLock(curdev->devLock);

        for(isrcur=ellFirst(&curdev->isrs), isrnext=isrcur ? ellNext(isrcur) : NULL;
            isrcur;
            isrcur=isrnext, isrnext=isrnext ? ellNext(isrnext) : NULL )
        {
            isr=CONTAINER(isrcur,osdISR,node);

            stopIsrThread(isr);

            ellDelete(&curdev->isrs,cur);
            free(isr);

        }

        close_uio(curdev);

        epicsMutexUnlock(curdev->devLock);
        epicsMutexDestroy(curdev->devLock);
        free(curdev->linuxDriver);
        free(curdev);
    }
    epicsMutexUnlock(pciLock);
    epicsMutexDestroy(pciLock);

    return 0;
}

static
int
linuxDevPCIFindCB(
     const epicsPCIID *idlist,
     devPCISearchFn searchfn,
     void *arg,
     unsigned int opt /* always 0 */
)
{
  int err=0, ret=0;
  ELLNODE *cur;
  osdPCIDevice *curdev=NULL;
  const epicsPCIID *search;

  if(!searchfn)
    return S_dev_badArgument;

  epicsMutexMustLock(pciLock);

  cur=ellFirst(&devices);
  for(; cur; cur=ellNext(cur)){
      curdev=CONTAINER(cur,osdPCIDevice,node);
      epicsMutexMustLock(curdev->devLock);

      for(search=idlist; search && !!search->device; search++){

          if(search->device!=DEVPCI_ANY_DEVICE &&
             search->device!=curdev->dev.id.device)
              continue;
          else
              if(search->vendor!=DEVPCI_ANY_DEVICE &&
                 search->vendor!=curdev->dev.id.vendor)
                  continue;
          else
              if( search->sub_device!=DEVPCI_ANY_SUBDEVICE &&
                  search->sub_device!=curdev->dev.id.sub_device
                  )
                  continue;
          else
              if( search->sub_vendor!=DEVPCI_ANY_SUBVENDOR &&
                  search->sub_vendor!=curdev->dev.id.sub_vendor
                  )
                  continue;
          else
              if( search->pci_class!=DEVPCI_ANY_CLASS &&
                  search->pci_class!=curdev->dev.id.pci_class
                  )
                  continue;
          else
              if( search->revision!=DEVPCI_ANY_REVISION &&
                  search->revision!=curdev->dev.id.revision
                  )
                  continue;

          /* Match found */

          err=searchfn(arg,&curdev->dev);
          if(err==0) /* Continue search */
              continue;
          else if(err==1) /* Abort search OK */
              ret=0;
          else /* Abort search Err */
              ret=err;
          epicsMutexUnlock(curdev->devLock);
          goto done;

      }

      epicsMutexUnlock(curdev->devLock);

  }

done:
  epicsMutexUnlock(pciLock);

  return ret;
}

static
int
linuxDevPCIToLocalAddr(
  epicsPCIDevice* dev,
  unsigned int bar,
  volatile void **ppLocalAddr,
  unsigned int opt
)
{
    osdPCIDevice *osd=CONTAINER(dev,osdPCIDevice,dev);

    epicsMutexMustLock(osd->devLock);

    if (open_uio(osd)) {
        epicsMutexUnlock(osd->devLock);
        return S_dev_addrMapFail;
    }

    if (!osd->base[bar]) {
        osd->base[bar] = mmap(NULL, osd->offset[bar]+osd->len[bar],
                              PROT_READ|PROT_WRITE, MAP_SHARED,
                              osd->fd, bar*pagesize);
        if (osd->base[bar]==MAP_FAILED) {
            perror("Failed to map BAR");
            errlogPrintf("Failed to MMAP BAR %u of %u:%u.%u\n", bar,
                         osd->dev.bus, osd->dev.device, osd->dev.function);
            epicsMutexUnlock(osd->devLock);
            return S_dev_addrMapFail;
        }
    }

    *ppLocalAddr=osd->base[bar] + osd->offset[bar];

    epicsMutexUnlock(osd->devLock);
    return 0;
}

static
epicsUInt32
linuxDevPCIBarLen(
  epicsPCIDevice* dev,
  unsigned int bar
)
{
    osdPCIDevice *osd=CONTAINER(dev,osdPCIDevice,dev);

    epicsMutexMustLock(osd->devLock);
    return osd->len[bar];
    epicsMutexUnlock(osd->devLock);
}

static
int linuxDevPCIConnectInterrupt(
  epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter,
  unsigned int opt
)
{
    char name[10]; /* RTEMS native names are only 4 chars long */
    ELLNODE *cur;
    osdPCIDevice *osd=CONTAINER(dev,osdPCIDevice,dev);
    osdISR *other, *isr=calloc(1,sizeof(osdISR));

    if (!isr) return S_dev_noMemory;

    isr->fptr=pFunction;
    isr->param=parameter;
    isr->osd=osd;
    isr->waiter_status=osdISRStarting;

    epicsMutexMustLock(osd->devLock);
    for(cur=ellFirst(&osd->isrs); cur; cur=ellNext(cur))
    {
        other=CONTAINER(cur,osdISR,node);
        if (other->fptr==isr->fptr && other->param==isr->param) {
            epicsMutexUnlock(osd->devLock);
            errlogPrintf("ISR already registered\n");
            free(isr);
            return S_dev_vecInstlFail;
        }
    }
    ellAdd(&osd->isrs,&isr->node);
    epicsMutexUnlock(osd->devLock);

    snprintf(name,NELEMENTS(name),"%02xPCIISR",dev->irq);
    name[NELEMENTS(name)-1]='\0';

    /* Ensure that "IRQ" thread has higher priority
     * then all other EPICS threads.
     */
    isr->waiter = epicsThreadMustCreate(name,
                                        epicsThreadPriorityMax-1,
                                        epicsThreadStackMedium,
                                        isrThread,
                                        isr
                                        );
    if (!isr->waiter) {
        errlogPrintf("Failed to create ISR thread\n");

        epicsMutexMustLock(osd->devLock);
        ellDelete(&osd->isrs,&isr->node);
        epicsMutexUnlock(osd->devLock);

        free(isr);
        return S_dev_vecInstlFail;
    }

    return 0;
}

static
void isrThread(void* arg)
{
    osdISR *isr=arg;
    osdPCIDevice *osd=isr->osd;
    int interrupted=0, ret;
    epicsInt32 event;
    const char* name;
    sigset_t allow;

    sigemptyset(&allow);
    sigaddset(&allow, SIGHUP);

    name=epicsThreadGetNameSelf();

    if (pthread_sigmask(SIG_UNBLOCK, &allow, NULL))
        errlogPrintf("Failed to set mask for thread %s,\n"
                     "ISR disconnect may not work correctly", name);

    epicsMutexMustLock(osd->devLock);

    if (isr->waiter_status!=osdISRStarting) {
        isr->waiter_status = osdISRDone;
        epicsMutexUnlock(osd->devLock);
        return;
    }

    isr->waiter_id = pthread_self();
    isr->waiter_status = osdISRRunning;

    while (isr->waiter_status==osdISRRunning) {
        epicsMutexUnlock(osd->devLock);

        /* The interrupted flag lets us check
         * the status flag (taking devLock)
         * once each iteration
         */
        if (interrupted) {
            interrupted=0;
            (isr->fptr)(isr->param);
        }

        ret=read(osd->fd, &event, sizeof(event));
        if (ret==-1) {
            switch(errno) {
            case EINTR: /* interrupted by a signal */
                break;
            default:
                errlogPrintf("isrThread '%s' read error %d\n",
                             name,errno);
                epicsThreadSleep(0.5);
            }
        } else
            interrupted=1;

        /* the 'event' number isn't used */

        epicsMutexMustLock(osd->devLock);
    }

    isr->waiter_status = osdISRDone;

    epicsMutexUnlock(osd->devLock);
}

/* Caller must take devLock */
static
void
stopIsrThread(osdISR *isr)
{
    if (isr->waiter_status==osdISRDone)
        return;
    else if (isr->waiter_status==osdISRRunning) {
        pthread_kill(isr->waiter_id, SIGHUP);
    }

    isr->waiter_status = osdISRStopping;

    while (isr->waiter_status!=osdISRDone) {
        epicsMutexUnlock(isr->osd->devLock);

        epicsThreadSleep(0.1);

        epicsMutexMustLock(isr->osd->devLock);
    }
}

static
int linuxDevPCIDisconnectInterrupt(
  epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter
)
{
    int ret=S_dev_intDisconnect;
    ELLNODE *cur;
    osdISR *isr;
    osdPCIDevice *osd=CONTAINER(dev,osdPCIDevice,dev);

    epicsMutexMustLock(osd->devLock);
    for(cur=ellFirst(&osd->isrs); cur; cur=ellNext(cur))
    {
        isr=CONTAINER(cur,osdISR,node);

        if (pFunction==isr->fptr && parameter==isr->param) {

            stopIsrThread(isr);

            ellDelete(&osd->isrs,cur);
            free(isr);

            ret=0;
            break;
        }
    }
    epicsMutexUnlock(osd->devLock);

    return ret;
}

devLibPCI plinuxPCI = {
  "native",
  linuxDevPCIInit, linuxDevFinal,
  linuxDevPCIFindCB,
  linuxDevPCIToLocalAddr,
  linuxDevPCIBarLen,
  linuxDevPCIConnectInterrupt,
  linuxDevPCIDisconnectInterrupt
};
#include <epicsExport.h>

void devLibPCIRegisterBaseDefault(void)
{
    devLibPCIRegisterDriver(&plinuxPCI);
}
epicsExportRegistrar(devLibPCIRegisterBaseDefault);
