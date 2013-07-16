
#include "libm3l.h"
#include "Server_Header.h"
#include "Server_Functions_Prt.h"

#include "SR_hub.h"

void *SR_hub(void *arg)
{

	lmsize_t i;
	node_t *List;
	find_t *SFounds;
	lmchar_t *ATDTMode, *KeepAllive_Mode;
/*
 * function signals SR_Threads that all requests (one Sender and n_rec_proc Receivers) arrived and 
 * that it can start data transfer
 */

/*
 * NOTE in previuous version, this function was run after the while(n_avail_loc_theads != 0);  // all connecting thread arrivied, ie. one Sender and n_rec_proc Receivers 
 * in Thread_Prt ended. It did not work when more then one data set arrived and pt_sync in Thread_Prt in do loop was not behaving properly
 * due to the fact that the number os synced threads was changing. To avoid that problem the process of signaling SR_threads are run in parallel 
 * with Thread_Prt
 */
	SR_hub_thread_str_t *c = (SR_hub_thread_str_t *)arg;

// 		if(m3l_Cat(c->pList, "--all", "-L", "-P", "*",   (lmchar_t *)NULL) != 0)
// 			Warning("CatData");
/*
 * find AT-DT mode
 * find KEEP_CONN_ALIVE
 */
		Pthread_mutex_lock(c->plock);
		if( (SFounds = m3l_Locate(c->pList, "./Data_Set/CONNECTION/ATDT_Mode", "./*/*/*",  (lmchar_t *)NULL)) != NULL){
			
			if( m3l_get_Found_number(SFounds) != 1)
				Error("SR_hub: Only one CONNECTION/ATDT_Mode per Data_Set allowed");
/* 
 * pointer to list of found nodes
 */
				if( (List = m3l_get_Found_node(SFounds, 0)) == NULL)
					Error("Thread_Prt: NULL ATDT_Mode");
			
				ATDTMode = (lmchar_t *)m3l_get_data_pointer(List);
/* 
 * free memory allocated in m3l_Locate
 */
			m3l_DestroyFound(&SFounds);
		}
		else
		{
			Error("SR_hub: CONNECTION/ATDT_Mode not found\n");
		}
		
		
		if( (SFounds = m3l_Locate(c->pList, "./Data_Set/CONNECTION/KEEP_CONN_ALIVE_Mode", "./*/*/*",  (lmchar_t *)NULL)) != NULL){
			
			if( m3l_get_Found_number(SFounds) != 1)
				Error("SR_hub: Only one CONNECTION/KEEP_CONN_ALIVE_Mode per Data_Set allowed");
/* 
 * pointer to list of found nodes
 */
				if( (List = m3l_get_Found_node(SFounds, 0)) == NULL)
					Error("SR_hub: NULL KEEP_CONN_ALIVE_Mode");
			
				KeepAllive_Mode = (lmchar_t *)m3l_get_data_pointer(List);
/* 
 * free memory allocated in m3l_Locate
 */
			m3l_DestroyFound(&SFounds);
		}
		else
		{
			Error("SR_hub: CONNECTION/KEEP_CONN_ALIVE_Mode not found\n");
		}
		Pthread_mutex_unlock(c->plock);
		
		
		
		printf(" Modes are %c  %c \n", *ATDTMode, *KeepAllive_Mode);


	while(1){
/*
 * wait for semaphore from Thread_Prt that 
 * all requests arrived
 */
		Sem_wait(c->psem);
/*
 * wait until all SR_threads reach barrier, then start actual transfer of the data from S to R(s)
 */
		Pthread_barrier_wait(c->pbarr);
/*
 * once the data transfer is finished wait until all data is tranferred and S and R threads close their socket
*/
		Sem_wait(c->psem_g);

		Pthread_mutex_lock(c->plock);
/*
 * set the number of available threads for SR transfer to S + R(s) number of threads
 * counter used in Thread_Prt function to determine if all threads arrived
 */
			*c->pn_avail_loc_theads = *c->pn_rec_proc + 1;
/*
 * close sockets
 */
			for(i=0; i<*c->pn_avail_loc_theads; i++){
				if( close(c->psockfd[i]) == -1)
					Perror("close");
			}
/*
 * increase ncount of available Data_Threads
 */
			(*c->prcounter)++;
/*
 * release thread, ie. set Thread_Status = 0
 */
			*c->pThread_Status = 0;
/*
 * if all threads were occupied, ie *Data_Threads->data_threads_availth_counter == *c->pcounter == 0
 * the server is waiting for signal before the continuing with data process identification. 
 * This is done in Server_Body before syncing with data threads
 * If this happens, signal Server_Body that at least one data_thread is avaiable 
	*/
			if(*c->pcounter == 1)
				Pthread_cond_signal(c->pcond);
		
		Pthread_mutex_unlock(c->plock);
	}
/*
 * release borrowed memory, malloced before starting thread in Data_Thread()
 */
	free(c);

	return NULL;

}