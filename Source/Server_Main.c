#include "libm3l.h"
#include "Server_Header.h"



#include "SignalC.h"
#include "Data_Fork.h"
#include "Data_Thread.h"


// extern lmint_t optind;
// static lmint_t verbose_flag;

lmint_t main (int argc, char **argv){
/*
 * function is a main server function
 * it reads data in libm3l format defining
 * inter-process communication
 * spawns a child for each data set.
 */

	lmint_t c, portnum, status, j;
	lmint_t digit_optind = 0;
	lmchar_t *Filename=NULL;
	lmint_t rcbarr;
	
	node_t *Gnode = NULL;
	data_thread_str_t *Data_Threads;
	
	lmsize_t i;
	find_t *SFounds;
	node_t *LocNode;
	
	char opt_s='\0';
	
	portnum = -1;
/*
 * get options 
 */
	while (1) {
		lmint_t this_option_optind = optind ? optind : 1;
		lmint_t option_index = 0;
		static struct option long_options[] = {
			{"port",    		required_argument, 	0, 'p' },
			{"help",     		no_argument,       	0, 'h' },
			{"input_file",     	required_argument,	0, 'i' },
			{"show_file",     	no_argument,       	0, 's' },
			{0,         0,                 0,  0 }
		};

		c = getopt_long(argc, argv, "hi:p:s:012?",long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				printf("option %s", long_options[option_index].name);
			if (optarg)
				printf(" with arg %s", optarg);
				printf("\n");
			break;

			case '0':
			case '1':
			case '2':
				if (digit_optind != 0 && digit_optind != this_option_optind)
					printf("digits occur in two different argv-elements.\n");
				digit_optind = this_option_optind;
				printf("option %c\n", c);
			break;
			
			case 'i':
				printf("option i with value '%s'\n", optarg);
				if ( (Filename = strdup(optarg)) == NULL)
					Perror("strdup");
			break;
			
			case 'p':
				printf("option p with value '%s'\n", optarg);
				portnum = atoi(optarg);
			break;
			case 's':
				opt_s = 'y';
			break;
			
			case '?':
			case 'h':
				printf("Help : \n");
			break;

			default:
				printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
	}
/*
 * check that file name is given
 */
	if(Filename == NULL){
		free(Filename);
		Error("Server: no definition file specified\n");
	}
/*
 * check that port number is given
 */	
	if(portnum < 1){
		free(Filename);
		Error("Server: no port specified\n");
	}	
/*
 * open definition file
 */


	for(j=0; j<10; j++){

printf("   \n\n  CYCLE       %d\n\n ", j);

	if( (Gnode = m3l_Fread(Filename, (lmchar_t *)NULL))  == NULL){
		free(Filename);
		Error("Server: m3l_Fread");
	}
// 	free(Filename);
/*
 * if specified, write the file on screen
 */	
	if(opt_s == 'y'){
 		if(m3l_Cat(Gnode, "--all", "-L", "-P", "*",   (lmchar_t *)NULL) != 0)
 			Warning("CatData");
	}
/*
 * Ctrl C signal handler
 */
// 	signal(SIGINT, catch_int);
/*
 * SIGCHLD signal handler
 */    
// 	signal(SIGCHLD,sig_chld); 
// 	Data_Fork(Gnode);
	
	Data_Threads = Data_Thread(Gnode);		
/*
 * loop over and send variable
 */
	 Pthread_mutex_lock(&Data_Threads->Data_Glob_Args->lock);
/*
 * set the counter to number of available threads
 */
		*Data_Threads->data_threads_status_counter  =  Data_Threads->n_data_threads;
 /*
  * at the beginning the coutner of remainign threads is equal to 
  * number of available threads
  * this coutner is used to synchronize all threads at the end when they went on each other
  * it is reset every iterational step
  */
		*Data_Threads->data_threads_remain_counter  = *Data_Threads->data_threads_status_counter;
/* 
 * this is identification iof the thread
 */
		Data_Threads->Data_Glob_Args->VARIABLE     =  Data_Threads->data_threads[0];
/*
 * wait for barrier, indicating all threads in Data_Thread were created
 * the _wait on this barrier is the second_wait call in Data_Thread for each thread and this is the last one
 * makes sure we do not start Data_Thread before some of the data which are needed are filled abd mutex is locked - see 
 * fours lines above
 */
	Pthread_barrier_wait(&Data_Threads->Data_Glob_Args->barr);
/*
 * give condition signal to Data_Thread that they can start - see part of code in Data_Thread
 *              -   while (*c->prcounter == 0)
 *		- 	Pthread_cond_wait(c->pcond, c->plock);
 */
	Pthread_cond_broadcast(&Data_Threads->Data_Glob_Args->cond);


	for(i=0; i< Data_Threads->n_data_threads; i++){
		
		if(i > 0) {
			Pthread_mutex_lock(&Data_Threads->Data_Glob_Args->lock);
/* 
 * this is identification iof the thread
 */			 Data_Threads->Data_Glob_Args->VARIABLE    =  Data_Threads->data_threads[i];
/*
  * at the beginning the coutner of remainign threads is equal to 
  * number of available threads
  * this coutner is used to synchronize all threads at the end when they went on each other
  * it is reset every iterational step
  */
			*Data_Threads->data_threads_remain_counter = *Data_Threads->data_threads_status_counter;
/*
 * once all necessary data are set, send signal to all threads to start
 */
			Pthread_cond_broadcast(&Data_Threads->Data_Glob_Args->cond);
		}
			
		Pthread_mutex_unlock(&Data_Threads->Data_Glob_Args->lock);
/* 
 * when all Data_Thread are finished, - the identification part, the threads are waiting on each other. 
 * the last thread unlock the semaphore so that the next loop can start
 */
		Sem_wait(&Data_Threads->Data_Glob_Args->sem);		
		
	}
/*
 * join threads and release memmory
 */
	for(i=0; i< Data_Threads->n_data_threads; i++)
		pthread_join(Data_Threads->data_threads[i], NULL);
	
	
	Pthread_mutex_destroy(&Data_Threads->Data_Glob_Args->lock);
	Pthread_barrier_destroy(&Data_Threads->Data_Glob_Args->barr);
	Pthread_cond_destroy(&Data_Threads->Data_Glob_Args->cond);
 	Pthread_cond_destroy(&Data_Threads->Data_Glob_Args->dcond);
 	Sem_destroy(&Data_Threads->Data_Glob_Args->sem);
	
	free(Data_Threads->data_threads);
	free(Data_Threads->data_threads_status_counter);
	free(Data_Threads->data_threads_remain_counter);
	free(Data_Threads->Data_Glob_Args);
	free(Data_Threads);
	
	
/*
 * free borrowed memory
 */
	if( (c = m3l_Umount(&Gnode)) != 1)
		Perror("m3l_Umount");


	}

	
// 	printf(" ------------------------------   Waiting for children \n");
// 	wait(&status);
// 	printf(" ------------------------------   DONE \n");

	printf(" ------------------------------   Exiting main function () \n");

	exit(EXIT_SUCCESS);
	
}