
#ifndef COUNTERS_H_INC
#define COUNTERS_H_INC

/**@brief Basic runtime statistics
 *
 * Implements a cumulative runtime counter.
 * When counter_stop() is called the value of the second argument
 * is used to normalize the most recent interval before
 * it is added to the total.
 */

#include <time.h>
#include <stdio.h>

#include "nan.h"

#ifdef __cplusplus
extern "C" {
#endif

struct counter
{
	struct timespec start;
	unsigned int runs;
        double sum, sum2;
        double high, low;
};

#define COUNTER_INIT {{0,0}, 0, 0.0, 0.0, NaN, NaN}

void counter_start(struct counter*);
void counter_stop(struct counter*, double);
void counter_reset(struct counter*);
void counter_report(struct counter*, FILE* f, const char*);

void counter_report_add(struct counter*, const char*);
void counter_report_del(struct counter*);

struct counter* counter_get(const char*);
void counter_reset_all(void);

void counter_report_all(FILE*);

#ifdef __cplusplus
}

class Counter {
    counter c;
public:
    Counter(const char* l) : c()
    {
        counter_report_add(&c, l);
    };
    ~Counter()
    {
        counter_report_del(&c);
    }

    operator counter* () {return &c;}
};

class scope_counter
{
    counter* cnt;
    double norm;
public:
        scope_counter(counter* x, double n=0.0) : cnt(x), norm(n)
        { counter_start(cnt); }

        scope_counter(Counter& c, double n=0.0) : cnt(c), norm(n)
        { counter_start(cnt); }

        ~scope_counter()
        { counter_stop(cnt, norm); }
};

#endif

#endif /* COUNTERS_H_INC */
