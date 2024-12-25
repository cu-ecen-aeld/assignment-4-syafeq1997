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

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    //Start a thread which sleeps @param wait_to_obtain_ms number of milliseconds
    usleep(thread_func_args->wait_to_obtain_ms);
    //obtains the mutex in @param mutex
    pthread_mutex_lock(thread_func_args->mutex);
    //holds for @param wait_to_release_ms milliseconds
    usleep(thread_func_args->wait_to_release_ms);
    //releases
    pthread_mutex_unlock(thread_func_args->mutex);
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
     
     //create struct
     struct thread_data* thread_data = malloc(sizeof(struct thread_data));
     if (thread_data == NULL) {
        ERROR_LOG("Memory allocation failed for struct thread_data\n");
        return false;
     }
     thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
     thread_data->wait_to_release_ms = wait_to_release_ms;
     thread_data->mutex = mutex;
     int ret = pthread_create(thread, NULL, threadfunc, thread_data);
     if(ret != 0){
	 ERROR_LOG("Creating new thread failed\n");
         thread_data->thread_complete_success = false;
	 free(thread_data);
	 return false;

     }
     thread_data->thread_complete_success = true;
     return true;
}
