//
// Linoy Menda 313302317
//

#include <stdlib.h>
#include <pthread.h>
#include "osqueue.h"
#include "threadPool.h"
#include <stdio.h>


// declare on function which tpCreate create and the implement come later
// this function is unvisible for user in h file
void* funcForAllThreads(ThreadPool* tp);

// this function create ThreadPool and return pointer to tp struct
ThreadPool* tpCreate(int numOfThreads) {
    // define memory for tp struct
    ThreadPool *tp = (ThreadPool *) malloc(sizeof(ThreadPool));
    // define number of threads in struct
    tp->n_threads = numOfThreads;
    // when create tp so tpDestroy is zero. when we will call tpInsertTask function the flag will change to 1
    tp->flag_tpDestroy = 0;
    // relevant only when flag_tpDestroy is equal to 1
    tp->shouldWaitForTasks = -1;
    // define pointer to array threads , the array size is numOfThreads
    pthread_t *p_arr_threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
    // define the pointer arr into the struct
    tp->arr_ntid = p_arr_threads;
    // initialize mutex lock and cond
     if (pthread_mutex_init(&tp->mutex,NULL)!=0) {
         perror("Error in system call");
         free(p_arr_threads);
         free(tp);
         exit(-1);
     }
    if (pthread_cond_init(&tp->cond,NULL)!=0) {
        perror("Error in system call");
        free(p_arr_threads);
        free(tp);
        exit(-1);
    }
    // create queue
    tp->q = osCreateQueue();
    // create treads and save their id
    int err;
    int i;
    for (i = 0;  i<numOfThreads ; i++) {
        err = pthread_create(p_arr_threads, NULL, (void *)funcForAllThreads, tp);
        p_arr_threads++;
        if (err != 0) {
            perror("Error in system call");
            free(p_arr_threads);
            free(tp);
        }
    }
    return tp;
}

// this function insert assignment to queue
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param){
    // create FunctionData from given data
    FunctionData* func_data = (FunctionData*) malloc (sizeof(FunctionData));
    // initialize Function Data
    func_data->func = computeFunc;
    func_data->param = param;
    // only if flag_tpDestroy is false(0) we can insert new assignments
    if (threadPool->flag_tpDestroy!=1) {
        // insert new assignment to queue
        osEnqueue(threadPool->q, func_data);
        // send signal - queue is not empty
        pthread_cond_broadcast(&threadPool->cond);
    }
    return 0;
}
// This function is responsible handle threads synchronization
void* funcForAllThreads(ThreadPool* tp){
    while (1) {
        // lock mutex
        if (pthread_mutex_lock(&tp->mutex)!=0){
            perror("Error in system call");
            free(tp->arr_ntid);
            free(tp);
            exit(-1);
        }
        // while queue is empty and we do not want to destroy queue
        while (osIsQueueEmpty(tp->q)==1 && tp->flag_tpDestroy==0){
            if (pthread_cond_wait(&tp->cond,&tp->mutex)!=0){
                free(tp->arr_ntid);
                free(tp);
                exit(-1);
            }
        }
        // destroy queue when no more assignments because we wait for all of them
        // or just leave if we do not wait for other assignments.
        if ((osIsQueueEmpty(tp->q)==1 && tp->flag_tpDestroy==1 && tp->shouldWaitForTasks!=0) ||
        (tp->flag_tpDestroy==1 && tp->shouldWaitForTasks==0)){
            // unlock mutex before ent threads - avoid deadlock
            if (pthread_mutex_unlock(&tp->mutex)!=0){
                free(tp->arr_ntid);
                free(tp);
                exit(-1);
            }
            pthread_exit(NULL);
        }
        // if queue is not empty and we do not want to destroy queue
        // or we want destroy but before finish all assignments
        FunctionData *func_data = osDequeue(tp->q);
        // realise lock before function
        if(pthread_mutex_unlock(&tp->mutex)!=0){
            free(tp->arr_ntid);
            free(tp);
            exit(-1);
        }
        // implement function
        func_data->func(func_data->param);
        // in order to avoid "definitely lost" - dequeue use and free
        free(func_data);
    }

}
// stop Thread pool
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){
    threadPool->flag_tpDestroy = 1;
    threadPool->shouldWaitForTasks = shouldWaitForTasks;
    // send alert to all threads that we are in destroy status
    pthread_cond_broadcast(&threadPool->cond);
    if (threadPool->flag_tpDestroy==1){
        int i;
        for (i = 0; i < threadPool->n_threads; i++) {
            pthread_join(threadPool->arr_ntid[i],NULL);
        }
    }
    while (osIsQueueEmpty(threadPool->q)!=1) {
        free(osDequeue(threadPool->q));
    }
    osDestroyQueue(threadPool->q);
    pthread_mutex_destroy(&threadPool->mutex);
    pthread_cond_destroy(&threadPool->cond);
    free(threadPool->arr_ntid);
    free(threadPool);
}
