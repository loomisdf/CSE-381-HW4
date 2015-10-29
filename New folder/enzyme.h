#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX 100

typedef struct {
        char *string; // keep testing and swap contents if necessary so that string[0] < string[1]
        int swapcount; // used later
} thread_info_t;

extern int please_quit;

extern int workperformed;

void *run_enzyme(void *data);
int make_enzyme_threads(pthread_t * enzymes_tid, char *string, void *(*fp)(void *));
int join_on_enzymes(pthread_t *threads_tid, int n);

int smp2_main(int, char**);
