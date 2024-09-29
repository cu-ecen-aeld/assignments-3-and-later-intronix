#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    usleep(thread_func_args->wait_to_obtain_ms * 1000); // usleep uses microseconds
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    usleep(thread_func_args->wait_to_release_ms * 1000); // Wait before releasing
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    thread_func_args->thread_complete_success = true;
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *thread_func_args = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (thread_func_args == NULL) {
    }
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;
    thread_func_args->thread_complete_success = false;
    if (pthread_create(thread, NULL, threadfunc, (void *)thread_func_args) != 0) {
        free(thread_func_args);
        return false;
    }

    return true; 
}

