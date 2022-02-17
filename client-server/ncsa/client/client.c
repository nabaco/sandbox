#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <ncsa/common/common.h>
/* TODO: Add log4c */

#define MAX_THREADS 10

typedef struct test_s {
    char name[32];
    int id;
    int ret;
} test_t;

void *thread_routine(void *routine_args) {
    test_t *args = (test_t *)routine_args;

    printf("Thread name: %s, ID: %d\n", args->name, args->id);
    sleep(args->id);
    args->ret = RETURN_OK;

    pthread_exit(NULL);
}

int main(void) {
    pthread_t thread_id[MAX_THREADS];
    test_t test_arg[MAX_THREADS];
    int ret = 0;

    printf("Starting Client\n");

    for(int i=0; i<MAX_THREADS;i++) {
        test_arg[i].id=i;
        ret = sprintf(test_arg[i].name,"Test%d",i);
        test_arg[i].ret=RETURN_ERR;

        printf("Starting thread #%d\n",i);
        ret = pthread_create(&thread_id[i], NULL, thread_routine, (void *)&test_arg[i]);
    }

    for(int i=0; i<MAX_THREADS;i++) {
        ret = pthread_join(thread_id[i], NULL);

        printf("Thread return value: %d\n", test_arg[i].ret);
    }

    return ret;
}
