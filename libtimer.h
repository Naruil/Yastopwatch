/**
 * Timer library
 * =============
 *
 * 1) Declare Timers
 *
 * DEF_TIMER(name)
 * DEF_TSC_TIMER(name)
 * DEF_THREADED_TIMER(name)
 * DEF_THREADED_TSC_TIMER(name)
 *
 * 2) Control Timers
 *
 * START_TIMER(name)
 * STOP_TIMER(name)
 * RESET_TIMER(name)
 * SYNC_TIMER(name)
 *
 * 3) Examine Timers
 * 
 * GET_TIME(name)
 * GET_SEC(name)
 * GET_USEC(name)
 * GET_COUNT(name)
 * GET_THREAD_TIME(name)
 * GET_THREAD_SEC(name)
 * GET_THREAD_USEC(name)
 * GET_THREAD_COUNT(name)
 *
 */

#ifndef __LIBTIMER_H__
#define __LIBTIMER_H__

#include <sys/time.h>
#include <pthread.h>

#define SYNC_RATE 10

enum __timer__source__ {
    TSC,
    GETTIMEOFDAY
};

enum __timer__type__ {
    NORMAL,
    THREADED
};

typedef struct __timer__ __timer_t__;

struct __timer__ {
   unsigned long long last;
   unsigned long long count;
   unsigned long long sum;

   /* For per-thread timing */
   unsigned long long pcount;
   unsigned long long psum;
   
   __timer_t__ *opaque;
};

#define __TIMER(name) __timer__##name
#define __GLOBAL__TIMER(name) __global__timer__##name
#define __TYPE(name) __type__##name
#define __SOURCE(name) __source__##name

#define DEF_TIMER(name)                                 \
    struct __timer__ __TIMER(name);                            \
    enum __timer__source__ __SOURCE(name) = GETTIMEOFDAY;        \
    enum __timer__type__ __TYPE(name) = NORMAL;

#define DEF_TSC_TIMER(name)                             \
    struct __timer__ __TIMER(name);                            \
    enum __timer__source__ __SOURCE(name) = TSC;                 \
    enum __timer__type__ __TYPE(name) = NORMAL;

#define DEF_THREADED_TIMER(name)                        \
    struct __timer__ __GLOBAL__TIMER(name);             \
    __thread struct __timer__ __TIMER(name) = {0, 0, 0, 0, 0, &__GLOBAL__TIMER(name)};\
    enum __timer__source__ __SOURCE(name) = GETTIMEOFDAY;        \
    enum __timer__type__ __TYPE(name) = THREADED;       \

#define DEF_THREADED_TSC_TIMER(name)                    \
    struct __timer__ __GLOBAL__TIMER(name);             \
    __thread struct __timer__ __TIMER(name) = {0, 0, 0, 0, 0, &__GLOBAL__TIMER(name)};\
    enum __timer__source__ __SOURCE(name) = TSC;        \
    enum __timer__type__ __TYPE(name) = THREADED;       \

#define __GET__TIME(name) (__SOURCE(name) == TSC ? get_tsc() : get_usec())

#define SYNC_TIMER(name)                                \
{                                                       \
    sync_timer(&__TIMER(name));                         \
}

#define START_TIMER(name)                               \
{                                                       \
    __TIMER(name).last = __GET__TIME(name);             \
}

#define STOP_TIMER(name)                                \
{                                                       \
    __TIMER(name).count ++;                             \
    __TIMER(name).sum += __GET__TIME(name) - __TIMER(name).last;    \
    if(__TYPE(name) == THREADED) {                      \
        if(__TIMER(name).count == SYNC_RATE) {          \
            sync_timer(&__TIMER(name));                 \
        }                                               \
    }                                                   \
}

#define RESET_TIMER(name)                               \
{                                                       \
    __TIMER(name).last = 0;                             \
    __TIMER(name).sum = 0;                              \
    __TIMER(name).count = 0;                            \
    if(__TYPE(name) == THREADED) {                      \
        __TIMER(name).pcount = 0;                       \
        __TIMER(name).psum = 0;                         \
        __GLOBAL__TIMER(name).sum = 0;                  \
        __GLOBAL__TIMER(name).count = 0;                \
    }                                                   \
}

#define GET_TIME(name) (__TYPE(name) == THREADED ? (__TIMER(name).opaque)->sum : __TIMER(name).sum)
#define GET_SEC(name) (__SOURCE(name) == TSC ? GET_TIME(name) / (double) FREQ : GET_TIME(name) / (double) 1000000)
#define GET_USEC(name) (__SOURCE(name) == TSC ? GET_TIME(name) / ((double) FREQ / 1000000) : (double)GET_TIME(name))
#define GET_COUNT(name) (__TYPE(name) == THREADED ? (__TIMER(name).opaque)->count : __TIMER(name).count)

#define GET_THREAD_TIME(name) (__TIMER(name).psum)
#define GET_THREAD_SEC(name) (__SOURCE(name) == TSC ? GET_THREAD_TIME(name) / (double) FREQ : GET_THREAD_TIME(name) / (double) 1000000)
#define GET_THREAD_USEC(name) (__SOURCE(name) == TSC ? GET_THREAD_TIME(name) / ((double) FREQ / 1000000) : (double)GET_THREAD_TIME(name))
#define GET_THREAD_COUNT(name) (__TIMER(name).pcount)


static void sync_timer(struct __timer__ *timer) {
    struct __timer__ *gtimer = timer->opaque;
    timer->pcount += timer->count;
    timer->psum += timer->sum;
    __sync_fetch_and_add(&gtimer->count, timer->count);
    __sync_fetch_and_add(&gtimer->sum, timer->sum);
    timer->count = 0;
    timer->sum = 0;
}

inline static unsigned long long get_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}

inline static unsigned long long get_tsc(void) {
    unsigned a, d;
    __asm __volatile("rdtsc":"=a"(a), "=d"(d));
    return ((unsigned long long)a) | (((unsigned long long)d) << 32);
}

#define FREQ (2.4 * 1000 * 1000 * 1000)

#endif
