/*************** YOU SHOULD NOT MODIFY ANYTHING IN THIS FILE ***************/

#define _GNU_SOURCE
#include <stdio.h>
#undef _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "testrunner.h"
#include "enzyme.h"

#define quit_if(cond) do {if (cond) exit(EXIT_FAILURE);} while(0)

/* Prepare input, reroute file descriptors, and run the program. */
void run_test(const char *input, int argc, char **argv)
{
	/* Prepare input */
	FILE *in = fopen("smp2.in", "w");
	fprintf(in, input);
	fclose(in);
	/* Run the program */
	int child_pid=fork();
	if(child_pid == 0) {
	/* Reroute standard file descriptors */
	freopen("smp2.in",  "r", stdin );
	freopen("smp2.out", "w", stdout);
	freopen("smp2.err", "w", stderr);
		quit_if(smp2_main(argc, argv) != EXIT_SUCCESS);
		exit(0);
	} else {		
		int status;
		waitpid(child_pid,&status,0);
	}
}

/*
 * Helper functions for testing components
 */


static void *dummy_enzyme(void *data) {
	thread_info_t *info = (thread_info_t *)data;
	info->swapcount = info->string[0]-'a';
	return (void *)info;
}


/* Test normal functionality */
int test_sort(int argc, char **argv)
{
	char *args[] = { "./enzyme", "ddcba", NULL };
	FILE  *err;
	int rv,itmp1,itmp2,totalswap;
	/* Run the test */
	run_test("", 2, args);
	/* Check output */
	err = fopen("smp2.out", "r");
	fscanf(err,"Creating threads...\nDone creating %d threads.\nJoining threads...\n",&itmp1);
	quit_if(itmp1 != 4);
	int found=1, lines=0;
	totalswap = 0;
	while(found<5) {
		lines++;
		quit_if(lines > 100);
	  rv = fscanf(err, "Thread %d exited normally: %d swaps.\n",&itmp1,&itmp2);
		if(rv == 2) {totalswap += itmp2; found ++;}
	  quit_if(rv<0 || feof(err));
	}
	rv = fscanf(err, "Total: %d swaps\n" 
		    "Sorted string: abcdd\n",&itmp1);
	quit_if(rv != 1 || itmp1 != totalswap);
	quit_if(!feof(err));
	fclose(err);
	return EXIT_SUCCESS;
}

/* Test make_threads functionality */
int test_make_enzyme_threads(int argc, char **argv) {
	char string[] = "enzymatic";
	int i;
	workperformed = 0;
	please_quit = 0;
	pthread_t enzymes[8];
	int n = make_enzyme_threads(enzymes,string,dummy_enzyme);
	quit_if(n != 8);
	int c = 0;
    void *status;
	for(i=0;i<n;i++) {
	    int rv = pthread_join(enzymes[i],&status);
	    if (rv != 0)   return -1;
	    else if (status != NULL) {
	      c += ((thread_info_t *)status)->swapcount;
	    }
	}	
	quit_if(c != 105);
	return EXIT_SUCCESS;
}

int test_pleasequit1(int argc, char **argv) {
	please_quit=1;
	
	thread_info_t data = {strdup("ab"),0};
	run_enzyme(&data);
	return  EXIT_SUCCESS;
}

int test_pleasequit0(int argc, char **argv) {
	please_quit=0;
	thread_info_t data = {strdup("abc"),0};
	int child_pid=fork();
	if(child_pid ==0) {
		run_enzyme(&data);
		return EXIT_FAILURE; // Should never get to here	
	}
	sleep(1);
	int status;
	int r=waitpid(child_pid,&status,WNOHANG);
	kill(child_pid,9);
	return r==child_pid && WIFEXITED(status) ? EXIT_FAILURE:EXIT_SUCCESS;
}

// Checks that the enzymes performs no work on 'ab'
int test_swap_count_workperformed3(int argc, char **argv) {
	please_quit=0;
	workperformed=0;
	thread_info_t data = {strdup("ab"),0};
	pthread_t tid;
	pthread_create(&tid, NULL,run_enzyme,&data );
	void * result;
	sched_yield();
	please_quit=1;
	int r=pthread_join(tid, &result);
	quit_if(r != 0 || result !=  &data) ;
	quit_if(data.swapcount != 0);
	quit_if(workperformed != 0);
	return EXIT_SUCCESS;
}

// Check that the enzyme performs some work on the string and updates workperformed.
int test_swap_count_workperformed1(int argc, char**argv) {
	please_quit=0;
	workperformed=0;
	thread_info_t data = {strdup("ba"),0};
	pthread_t tid;
	pthread_create(&tid, NULL,run_enzyme,&data );
	sched_yield();
	sleep(1);
	please_quit=1;
	void * result;
	int r=pthread_join(tid, &result);
	quit_if(r != 0 );
	quit_if( workperformed != 1);
	quit_if(data.string[0] != 'a' && data.string[1] != 'b');
	return EXIT_SUCCESS;
}

// Checks the swapcount value
int test_swap_count_workperformed2(int argc, char**argv) {
	please_quit=0;
	thread_info_t data = {strdup("ba"),0};
	pthread_t tid;
	pthread_create(&tid, NULL,run_enzyme,&data );
	sched_yield();
	sleep(1);
	please_quit=1;
	void * result;
	int r=pthread_join(tid, &result);
	quit_if(r!= 0 || result !=  &data) ;
	quit_if(data.swapcount != 1) ;
	return EXIT_SUCCESS;
}


int test_cancelenzyme(int argc, char**argv) {
	please_quit=0;
	thread_info_t data = {strdup("C"),0};
	pthread_t tid;
	pthread_create(&tid, NULL,run_enzyme,&data );
		sched_yield();please_quit=1;
	void* result;
	int r=pthread_join(tid, &result);
	if(r!= 0 ) return EXIT_FAILURE;
	return result == PTHREAD_CANCELED ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Test run_enzyme functionality */
int test_run_enzyme(int argc, char **argv) {
	char* string = strdup("dcba");

	workperformed = 0;
	please_quit = 0;
	pthread_t tid[3];
	thread_info_t data1 = {string,0};
	thread_info_t data2 = {string+1,0};
	thread_info_t data3 = {string+2,0};
	pthread_create(&tid[0], NULL,run_enzyme,&data1 );
	pthread_create(&tid[1], NULL,run_enzyme,&data2 );
	pthread_create(&tid[2], NULL,run_enzyme,&data3 );
	sched_yield();
	sleep(1);
	sched_yield();
	sleep(1);
	sched_yield();
	please_quit = 1;
	void* results[3];
	pthread_join(tid[0], &results[0]);
	pthread_join(tid[1], &results[1]);
	pthread_join(tid[2], &results[2]);

	quit_if((thread_info_t*)results[0] != &data1) ;
	quit_if((thread_info_t*)results[1] != &data2) ;
	quit_if((thread_info_t*)results[2] != &data3)  ;
	
	quit_if(data1.swapcount + data2.swapcount+ data3.swapcount != 6);
	return EXIT_SUCCESS;
}

/* Test join_on_enzymes functionality */
int test_join(int argc, char **argv) {
	char* string = strdup("dcba");
	
	workperformed = 0;
	please_quit = 0;
	pthread_t tid[3];
	thread_info_t data1 = {string,0};
	thread_info_t data2 = {string+1,0};
	thread_info_t data3 = {string+2,0};

	pthread_create(&tid[0], NULL,dummy_enzyme,&data1 );
	pthread_create(&tid[1], NULL,dummy_enzyme,&data2 );
	pthread_create(&tid[2], NULL,dummy_enzyme,&data3 );

	
    freopen("smp2.out","w",stdout);
    freopen("smp2.err","w",stderr);
	int tot = join_on_enzymes(&tid[0],3);
	printf("tot=%d\n",tot);
	quit_if(tot != 6);
	return EXIT_SUCCESS;
}

/*
 * Main entry point for SMP2 test harness
 */
int run_smp2_tests(int argc, char **argv)
{
	/* Tests can be invoked by matching their name or their suite name 
	   or 'all' */
	testentry_t tests[]={
	{ "make" , "make", test_make_enzyme_threads },
	{ "sort" , "enzyme", test_sort },
	{ "pleasequit1" , "enzyme", test_pleasequit1 },
	{ "pleasequit0" , "enzyme", test_pleasequit0 },
	{ "swap1" , "enzyme", test_swap_count_workperformed1 },
	{ "swap2" , "enzyme", test_swap_count_workperformed2 },
	{ "swap3" , "enzyme", test_swap_count_workperformed3 },
	{ "run_enzyme" , "enzyme", test_run_enzyme },
	{ "join" , "join", test_join },
	{ "cancel" , "join", test_cancelenzyme }

	};
	int result = run_testrunner(argc, argv, tests, sizeof(tests) / sizeof (testentry_t));
	unlink("smp2.in");
	unlink("smp2.out");
	unlink("smp2.err");
	return result;
}

/* The real main function.  */
int main(int argc, char **argv)
{
	if (argc > 1 && !strcmp(argv[1], "-test")) {
		return run_smp2_tests(argc - 1, argv + 1);
	} else {
		return smp2_main(argc, argv);
	}
}
