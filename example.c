#include <time.h>
#include <stdio.h>
#include <pthread.h>

#include "libtimer.h"

#define FREQ (2.4 * 1000 * 1000 * 1000)

DEF_THREADED_TSC_TIMER(t1);
DEF_TSC_TIMER(t2);

void* worker (void* arg) {
    START_TIMER(t1);
    
    sleep(1);
    
    STOP_TIMER(t1);
    SYNC_TIMER(t1);
}

int main() {
    pthread_t thr1,thr2;

    START_TIMER(t2);
    
    pthread_create(&thr1, NULL, worker, NULL);
    pthread_create(&thr2, NULL, worker, NULL);
    
    pthread_join(thr1, NULL);
    pthread_join(thr2, NULL);

    STOP_TIMER(t2);

    printf("[t1] Total time: %lf\n", GET_SEC(t1));
    printf("[t1] Total count: %ld\n", GET_COUNT(t1));
    
    printf("[t2] Total time: %lf\n", GET_SEC(t2));
    printf("[t2] Total count: %ld\n", GET_COUNT(t2));
}
