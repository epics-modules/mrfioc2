/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <dbDefs.h>
#include <epicsTypes.h>
#include <epicsString.h>
#include <macLib.h>

#define epicsExportSharedSymbols
#include "linkoptions.h"

#ifndef HUGE_VALF
#  define HUGE_VALF HUGE_VAL
#endif
#ifndef HUGE_VALL
#  define HUGE_VALL (-(HUGE_VAL))
#endif

/*
 * Parse the value 'val' according to the type named in 'opt->optType'
 * and store the result at 'user + opt->offset'.
 */
static
int
store_value(const linkOptionDef* opt, void* user, const char* val, int options)
{
    epicsUInt32 *ival;
    unsigned long int lival;
    int *eval;
    const linkOptionEnumType *emap;
    double *dval;
    char *sval;
    char *end;

    (void)options; /* unused */

    switch(opt->optType) {
    case linkOptionInt32:
        if (opt->size<sizeof(epicsUInt32)) {
              fprintf(stderr, "Provide storage (%u bytes) is too small for Int32 (%lu)\n",
                              opt->size, (unsigned long)sizeof(epicsUInt32));
              return -1;
        }
        ival=(epicsUInt32*)( (char*)user + opt->offset );

        lival = strtoul(val, &end, 0);
        /* test for the myriad error conditions which strtol may use */
        if ( lival==ULONG_MAX || end==val )
        {
            fprintf(stderr,"value %s can't be converted for integer key %s\n",val,opt->name);
            return -1;
        }

        *ival = (epicsUInt32)lival;

        break;
    case linkOptionDouble:
        if (opt->size<sizeof(double)) {
              fprintf(stderr, "Provide storage (%u bytes) is too small for double (%lu)\n",
                              opt->size, (unsigned long)sizeof(double));
              return -1;
        }
        dval=(double*)( (char*)user + opt->offset );

        *dval = strtod(val, &end);
        /* Indicates errors in the same manner as strtol */
        if ( *dval==HUGE_VALF || *dval==HUGE_VALL || end==val )
        {
            fprintf(stderr,"value %s can't be converted for double key %s\n",val,opt->name);
            return -1;
        }
        break;
    case linkOptionEnum:
        if (opt->size<sizeof(int)) {
              fprintf(stderr, "Provide storage (%u bytes) is too small for enum (%lu)\n",
                              opt->size, (unsigned long)sizeof(int));
              return -1;
        }
        eval=(int*)( (char*)user + opt->offset );

        for(emap=opt->Enums; emap && emap->name; emap++) {
            if ( strcmp(emap->name, val)==0 ) {
                *eval=emap->value;
                break;
            }
        }

        if( !emap || !emap->name ) {
            fprintf(stderr,"%s is not a valid value for the Enum %s\n",
                   val, opt->name);
            return -1;
        }

        break;
    case linkOptionString:
        if (opt->size<sizeof(char*)) {
              /* Catch if someone has given us a char* instead of a char[]
               * Also means that char buffers must be >4.
               */
              fprintf(stderr, "Provide storage (%u bytes) is too small for string (>= %lu)\n",
                              opt->size, (unsigned long)sizeof(char*));
              return -1;
        }
        sval=( (char*)user + opt->offset );

        strncpy(sval, val, opt->size-1);
        sval[opt->size-1]='\0';
        break;
    case linkOptionInvalid:
        fprintf(stderr,"Can't store \'%s\' for %s as the storage type is not defined\n",
               val,opt->name);
        return -1;
        break;
    }

    return 0;
}

int
epicsShareAPI
linkOptionsStore(const linkOptionDef* opts, void* user, const char* str, int options)
{
    MAC_HANDLE handle; /* only .debug is used */

    int status=0;
    size_t i;
    epicsUInt32 *found;
    const linkOptionDef *cur;
    char **pairs=NULL, **arg;

    for(i=0, cur=opts; cur && cur->name; i++, cur++) {}

    /* Bit array to find missing required keys */
    found=calloc( (i/32)+1, sizeof(epicsUInt32) );
    if (!found) {
        fprintf(stderr,"store_options: calloc failed\n");
        status=-1;
        goto errbitarray;
    }

    memset((void*)&handle, 0, sizeof(handle));

    if (options&LINKOPTIONDEBUG)
        handle.debug=0xff;

    /* Parses 'str' and stores the result in the array pairs.
     * The length of pairs is twice the number of pairs found.
     * Each even element is a key, each odd element is a value.
     *
     * Pair N is (pairs[2*N], pairs[2*N+1])
     *
     * After the last pair is a (NULL, NULL)
     */
    if (macParseDefns(&handle, str, &pairs)<0) {
        status=-1;
        goto errparse;
    }

    for (i=0, cur=opts; cur && cur->name; i++, cur++) {

        if (options&LINKOPTIONDEBUG)
            fprintf(stderr,"For option: %s\n",cur->name);

        for (arg=pairs; arg && arg[0]; arg+=2) {

            if (options&LINKOPTIONDEBUG)
                printf("key %s\n",arg[0]);

            if( strcmp(arg[0], cur->name)!=0 )
                continue;

            if (found[i/32]&(1<<(i%32)) && !cur->overwrite) {
                fprintf(stderr,"Option %s was already given\n",cur->name);
                status=-1;
                goto errsemantix;
            }

            found[i/32] |= 1<<(i%32);

            status=store_value(cur, user, arg[1], options);
            if (status)
                goto errsemantix;
        }

        if ( !(found[i/32]&(1<<(i%32))) && cur->required ) {
            fprintf(stderr,"Missing required option %s\n",cur->name);
            status=-1;
            goto errsemantix;
        }
    }

errsemantix:
    free((void*)pairs);
errparse:
    free(found);
errbitarray:
    return status;
}

epicsShareFunc
const char*
epicsShareAPI
linkOptionsEnumString(const linkOptionEnumType *emap, int i, const char* def)
{
    for(; emap && emap->name; emap++) {
        if ( i == emap->value ) {
            return emap->name;
        }
    }
    return def;
}
