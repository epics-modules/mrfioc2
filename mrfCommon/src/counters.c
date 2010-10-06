
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <epicsInterrupt.h>
#include <ellLib.h>
#include <cantProceed.h>

#include "counters.h"

void counter_start(struct counter* c)
{
	int iflags;
	struct timespec tmp;

        if (!c) return;

	clock_gettime(CLOCK_MONOTONIC, &tmp);

	iflags=epicsInterruptLock();
	if (c->start.tv_sec || c->start.tv_nsec) {
		/* already running */
		epicsInterruptUnlock(iflags);
		return;
	}
	c->start = tmp;
	epicsInterruptUnlock(iflags);
	return;
}

void counter_stop(struct counter* c, double normalize)
{
	double run;
	struct timespec end;
	int iflags;

        if (!c) return;

	clock_gettime(CLOCK_MONOTONIC, &end);

        if (normalize==0) normalize=1.0;

	iflags=epicsInterruptLock();
	if (!c->start.tv_sec && !c->start.tv_nsec) {
		/* not running */
		epicsInterruptUnlock(iflags);
		return;
	}

	run = end.tv_nsec - c->start.tv_nsec;
	run *= 1e-9;
	run += end.tv_sec - c->start.tv_sec;
        run /= normalize;
	c->runs++;
        c->sum += run;
        c->sum2 += run*run;
	c->start.tv_sec=0;
	c->start.tv_nsec=0;
        if ( c->low==0 || run < c->low)  c->low =run;
        if ( c->high==0|| run > c->high) c->high=run;

	epicsInterruptUnlock(iflags);
}

void counter_reset(struct counter* c)
{
        int iflags;
        if (!c) return;
        iflags=epicsInterruptLock();
	c->start.tv_sec=0;
	c->start.tv_nsec=0;
	c->runs=0;
        c->sum=0.0;
        c->sum2=0.0;
        c->low=0.0;
        c->high=0.0;
	epicsInterruptUnlock(iflags);
}

void counter_report(struct counter* c, FILE* f, const char* s)
{
        if (!f || !c) f=stdout;
        double mean = c->sum / c->runs;
        double sigma = c->sum2 / c->runs - mean*mean;
        sigma = sqrt(sigma);
        fprintf(f,"counter %s ran %d times in %g or %g +- %g s [%g, %g]\n",
                s, c->runs, c->sum, mean, sigma, c->low, c->high);
}

typedef struct {
    ELLNODE node;
    struct counter* count;
    char name[1]; /* Actually longer */
} report_priv;

static ELLLIST counter_priv = {{0,0},0};

void counter_report_add(struct counter* c, const char* label)
{
    ELLNODE *node;
    report_priv *p;

    for(node=ellFirst(&counter_priv); node; node=ellNext(node)) {
        p=(report_priv*)node;
        if (p->count==c)
            return; /* skip duplicates */
    }

    p=mallocMustSucceed(sizeof(report_priv)+strlen(label), label);

    strcpy(p->name, label);

    p->count = c;

    ellAdd(&counter_priv, &p->node);
}

void counter_report_del(struct counter* c)
{
    ELLNODE *node;
    report_priv *p;

    for(node=ellFirst(&counter_priv); node; node=ellNext(node)) {
        p=(report_priv*)node;
        if (p->count!=c)
            continue;

        ellDelete(&counter_priv, node);
        free(p);
        break;
    }
}

struct counter*
counter_get(const char* label)
{
    ELLNODE *node;
    report_priv *p;

    for(node=ellFirst(&counter_priv); node; node=ellNext(node)) {
        p=(report_priv*)node;
        if (strcmp(p->name, label)==0)
            return p->count;
    }
    return NULL;
}

void counter_reset_all(void)
{
    ELLNODE *node;
    report_priv *p;

    for(node=ellFirst(&counter_priv); node; node=ellNext(node)) {
        p=(report_priv*)node;

        counter_reset(p->count);
    }
}

void counter_report_all(FILE* f)
{
    ELLNODE *node;
    report_priv *p;

    for(node=ellFirst(&counter_priv); node; node=ellNext(node)) {
        p=(report_priv*)node;

        counter_report(p->count, f, p->name);
    }
}
