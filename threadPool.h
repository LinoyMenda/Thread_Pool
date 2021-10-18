#ifndef __THREAD_POOL__
#define __THREAD_POOL__
#include "osqueue.h"
#include <pthread.h>

typedef struct thread_pool
{
 // number of threads
 int n_threads;
 // array with id of each thread from n threads
 pthread_t* arr_ntid;
 // Queue with assignments
 OSQueue* q;
 int flag_tpDestroy; // if 0 so false - do not destroy, if 1 so true - destroy (stop get assignments)
 int shouldWaitForTasks; // if 0 so do not do not start task that in queue, if 1 run all tasks that in queue
 pthread_mutex_t mutex;
 pthread_cond_t cond;
}ThreadPool;

typedef struct function_data
{
// function
void (*func) (void *);
// parameters
void* param;
}FunctionData;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
