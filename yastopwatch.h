/**
 * Yastopwatch
 * =============
 *
 * 1) Declare stopwatchs
 *
 * DEF_SW(name)
 * DEF_TSC_SW(name)
 * DEF_THREADED_SW(name)
 * DEF_THREADED_TSC_SW(name)
 *
 * 2) Control stopwatchs
 *
 * START_SW(name)
 * STOP_SW(name)
 * RESET_SW(name)
 * SYNC_SW(name)
 *
 * 3) Examine stopwatchs
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

#ifndef __YASTOPWATCH_H__
#define __YASTOPWATCH_H__

#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>

#define SYNC_RATE 10

enum __stopwatch__source__ {
    TSC,
    GETTIMEOFDAY
};

enum __stopwatch__type__ {
    NORMAL,
    THREADED
};

typedef struct __stopwatch__ __stopwatch_t__;

struct __stopwatch__ {
   uint64_t last;
   uint64_t count;
   uint64_t sum;

   /* For per-thread timing */
   uint64_t pcount;
   uint64_t psum;
   
   __stopwatch_t__ *opaque;
};

#define __SW(name) __stopwatch__##name
#define __GLOBAL__SW(name) __global__stopwatch__##name
#define __TYPE(name) __type__##name
#define __SOURCE(name) __source__##name

#define DEF_SW(name)                                 \
    struct __stopwatch__ __SW(name);                            \
    enum __stopwatch__source__ __SOURCE(name) = GETTIMEOFDAY;        \
    enum __stopwatch__type__ __TYPE(name) = NORMAL;

#define DEF_TSC_SW(name)                             \
    struct __stopwatch__ __SW(name);                            \
    enum __stopwatch__source__ __SOURCE(name) = TSC;                 \
    enum __stopwatch__type__ __TYPE(name) = NORMAL;

#define DEF_THREADED_SW(name)                        \
    struct __stopwatch__ __GLOBAL__SW(name);             \
    __thread struct __stopwatch__ __SW(name) = {0, 0, 0, 0, 0, &__GLOBAL__SW(name)};\
    enum __stopwatch__source__ __SOURCE(name) = GETTIMEOFDAY;        \
    enum __stopwatch__type__ __TYPE(name) = THREADED;       \

#define DEF_THREADED_TSC_SW(name)                    \
    struct __stopwatch__ __GLOBAL__SW(name);             \
    __thread struct __stopwatch__ __SW(name) = {0, 0, 0, 0, 0, &__GLOBAL__SW(name)};\
    enum __stopwatch__source__ __SOURCE(name) = TSC;        \
    enum __stopwatch__type__ __TYPE(name) = THREADED;       \

#define __GET__TIME(name) (__SOURCE(name) == TSC ? get_tsc() : get_usec())

#define SYNC_SW(name)                                \
{                                                       \
    sync_stopwatch(&__SW(name));                         \
}

#define START_SW(name)                               \
{                                                       \
    __SW(name).last = __GET__TIME(name);             \
}

#define STOP_SW(name)                                \
{                                                       \
    __SW(name).count ++;                             \
    __SW(name).sum += __GET__TIME(name) - __SW(name).last;    \
    if(__TYPE(name) == THREADED) {                      \
        if(__SW(name).count == SYNC_RATE) {          \
            sync_stopwatch(&__SW(name));                 \
        }                                               \
    }                                                   \
}

#define RESET_SW(name)                               \
{                                                       \
    __SW(name).last = 0;                             \
    __SW(name).sum = 0;                              \
    __SW(name).count = 0;                            \
    if(__TYPE(name) == THREADED) {                      \
        __SW(name).pcount = 0;                       \
        __SW(name).psum = 0;                         \
        __GLOBAL__SW(name).sum = 0;                  \
        __GLOBAL__SW(name).count = 0;                \
    }                                                   \
}

#define GET_TIME(name) (__TYPE(name) == THREADED ? (__SW(name).opaque)->sum : __SW(name).sum)
#define GET_SEC(name) (__SOURCE(name) == TSC ? GET_TIME(name) / (double) FREQ : GET_TIME(name) / (double) 1000000)
#define GET_USEC(name) (__SOURCE(name) == TSC ? GET_TIME(name) / ((double) FREQ / 1000000) : (double)GET_TIME(name))
#define GET_COUNT(name) (__TYPE(name) == THREADED ? (__SW(name).opaque)->count : __SW(name).count)

#define GET_THREAD_TIME(name) (__SW(name).psum)
#define GET_THREAD_SEC(name) (__SOURCE(name) == TSC ? GET_THREAD_TIME(name) / (double) FREQ : GET_THREAD_TIME(name) / (double) 1000000)
#define GET_THREAD_USEC(name) (__SOURCE(name) == TSC ? GET_THREAD_TIME(name) / ((double) FREQ / 1000000) : (double)GET_THREAD_TIME(name))
#define GET_THREAD_COUNT(name) (__SW(name).pcount)


static void sync_stopwatch(struct __stopwatch__ *stopwatch) {
    struct __stopwatch__ *gstopwatch = stopwatch->opaque;
    stopwatch->pcount += stopwatch->count;
    stopwatch->psum += stopwatch->sum;
    __sync_fetch_and_add(&gstopwatch->count, stopwatch->count);
    __sync_fetch_and_add(&gstopwatch->sum, stopwatch->sum);
    stopwatch->count = 0;
    stopwatch->sum = 0;
}

inline static uint64_t get_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}

inline static uint64_t get_tsc(void) {
    uint32_t a, d;
    __asm __volatile("rdtsc":"=a"(a), "=d"(d));
    return ((uint64_t)a) | (((uint64_t)d) << 32);
}

#define FREQ (2.4 * 1000 * 1000 * 1000)

#endif
