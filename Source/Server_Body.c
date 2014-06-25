/*
 *     Copyright (C) 2012  Adam Jirasek
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 * 
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *     
 *     contact: libm3l@gmail.com
 * 
 */




/*
 *     Function Server_Body.c
 *
 *     Date: 2013-07-16
 * 
 * 
 *     Description:
 * 
 *
 *     Input parameters:
 * 
 *
 *     Return value:
 * 
 * 
 *
 *     Modifications:
 *     Date		Version		Patch number		CLA 
 *
 *
 *     Description
 * 
 */



#include "libm3l.h"
#include "lsipdx_header.h"
#include "Start_Data_Thread.h"
#include "Server_Functions_Prt.h"
#include "Server_Body.h"
#include "Allocate_DataBuffer.h"
#include "ACK.h"
#include "Allocate_Data_Thread_DataSet.h"
#include "Ident_Sys_Comm_Channel.h"
#include "Add_Data_Thread.h"


lmint_t Server_Body(node_t *Gnode, lmint_t portno){
	
	lmsize_t i;
	lmint_t sockfd, newsockfd, cycle;
	struct sockaddr_in cli_addr;
	data_thread_str_t *Data_Threads;
	lmchar_t name_of_required_data_set[MAX_NAME_LENGTH], SR_mode;
	
	socklen_t clilen;
	find_t *Tqst_SFounds;
	node_t *RecNode, *DataBuffer;
	lsipdx_answer_t *Answers;

	opts_t *Popts, opts;
	Popts = &opts;
	
	m3l_set_Send_receive_tcpipsocket(&Popts);
	
	cycle=0;
/*
 * create standard answers and queue
 */
	if (  (Answers = MakePredefinedAnswers()) == NULL)
		Error("Server_Body: Server_Body: error while makeing answers");
/*
 * create buffer structure for buffering recevied data requests if needed
 */
	if( (DataBuffer = Allocate_DataBuffer(Gnode)) == NULL)
		Error("Server_Body: Buffering problem");
/*
 * allocate Data_Thread used by Data_Thread.c and Start_Data_Thread.c
 */
	if(  (Data_Threads = Allocate_Data_Thread_DataSet()) == NULL)
		Perror("Server_Body: Allocate_Data_Thread_DataSet error");
/*
 * spawn all threads
 */
	if(  Start_Data_Thread(DataBuffer, Data_Threads) == -1)
		Perror("Server_Body: Data_Threads error");
/*
 * fill the initial data to data_thread_str before threads start
 */
	Pthread_mutex_lock(&Data_Threads->lock);
/*
 * set the counter to number of available threads
 */
	*Data_Threads->data_threads_availth_counter  =  Data_Threads->n_data_threads;
/*
 * at the beginning the coutner of remainign threads is equal to 
 * number of available threads
 * this coutner is used to synchronize all threads at the end when they went on each other
 * it is reset every iterational step
 */
	*Data_Threads->data_threads_remainth_counter  = *Data_Threads->data_threads_availth_counter;
/*
 * set return value to 0
 */
	*Data_Threads->retval = 0;
/*
 * create, bind and listen socket
 */
	if ( (sockfd = m3l_server_openbindlistensocket(portno, (char *)NULL) ) < 0 )
		Perror("Open_Bind_Listen");
	
	
	printf(" Unique ID is %ld\n", Make_ID_Number(sockfd));

	while(1){
/*
 * if already in cycle, you need to lock mutex here
 */
		if(cycle > 0){
			Pthread_mutex_lock(&Data_Threads->lock);
		}
/* 
 * if number of available threads is 0, wait until some of the Data_Threads become available. 
 * without this condition, the server would be looping on accepting connection and then refusing it 
 * at the Thread_Status of all threads is 1 and Check_Request return value is 1
 *
 * this condition is signaled by SR_hub
 */
		if(*Data_Threads->data_threads_availth_counter == 0){
			while(*Data_Threads->data_threads_availth_counter == 0)
				Pthread_cond_wait(&Data_Threads->cond, &Data_Threads->lock);
		}

		Pthread_mutex_unlock(&Data_Threads->lock);
	
		clilen = sizeof(cli_addr);
		
		if ( (newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0){
			if(errno == EINTR) /* If Interrupted system call, restart - back to while ()  UNP V1 p124  */
				continue;
			else
				Perror("accept()");
		}
/*
 * number of socket is saved locally in  *Data_Threads->socket  = newsockfd;
 * through that it is passed to all Data_Threads. Once the thread is identified as a 
 * positive match - ie. the thread takes care of this data channel it will save the 
 * value in its own local array (SR_Threads->sockfd[local_cntr]	= *c->psocket in Data_Thread)
 */
// 		inet_ntop(AF_INET, &(cli_addr.sin_addr), str, INET_ADDRSTRLEN);
//    		printf("	CONNECTION --------------------   : %s:%d\n",str, ntohs(cli_addr.sin_port)); 
/*
 * receive header with solver and data set information
 */
// 		if( (RecNode = m3l_Receive_tcpipsocket((const char *)NULL, newsockfd, "--encoding" , "IEEE-754", (char *)NULL)) == NULL)
// 			Error("Server_Body: Error during reading data from socket");

		opts.opt_REOBseq = '\0'; // send EOFbuff sequence only
		if( (RecNode = m3l_receive_tcpipsocket((const char *)NULL, newsockfd, Popts)) == NULL)
			Error("Server_Body: Error during reading data from socket");
		
		
// 		if(m3l_Cat(RecNode, "--all", "-P", "-L",  "*",   (char *)NULL) != 0)
// 			Error("Server_Body: CatData");
/*
 * identify type of request and get back with name of required connection and SR_mode
 */
 		Pthread_mutex_lock(&Data_Threads->lock);
/*
 * loop over - identify thread correspoding to required data thread.
 * 
 * the first type of request is system request ie. opening new, closing connection or
 * or changing ATDT, KA or number of R clients
 * 
 * the second is data transfer request for Data_Thread
 * 
 * Data_Thread thread spawns n SR threads (1 Sending thread and n-1 Reading threads) which take care of data transfer,
 * so once the data_thread is identified n-times, the thread is taken away from 
 * pool of available data threads (ie. decrement  (*Data_Threads->data_threads_availth_counter)--)
 * Once the data transfer is finished, add the data thread to the pool of available data threads
 * (ie. increment  (*Data_Threads->data_threads_availth_counter)++)
 */
		switch(Ident_Sys_Comm_Channel(RecNode, &DataBuffer, Data_Threads, 
				name_of_required_data_set, &SR_mode, Answers,newsockfd)){
			case 0:
/* 
 * Legal request, not in buffer, data_thread available 
 * set checkdata to 0 to notify Data_Thread that this is a usual arriving request
 */
				*Data_Threads->checkdata = 0;
				if(SR_mode == 'S'){
/*
 * if process is sender, indicate Sender that header was received before receiving payload
 * if process is Receiver send acknowledgment and get back REOB
 */
					opts.opt_EOBseq = '\0'; // send EOFbuff sequence only
					if( m3l_send_to_tcpipsocket(Answers->RR_POS, (const char *)NULL, newsockfd, Popts) < 1)
						Error("Server_Body: Error during sending data to socket");
				}
				else if(SR_mode == 'R'){

					opts.opt_REOBseq = 'G'; // receive EOFbuff sequence only
					opts.opt_EOBseq = '\0'; // send EOFbuff sequence only	
					if( m3l_send_receive_tcpipsocket(Answers->RR_POS, (const lmchar_t *)NULL, newsockfd, Popts) < 0){
						Error("Server_Body:  Error during receving data from socket \n");
						return -1;
					}
				}
				else
					Error("Server_Body: Wrong SR mode\n");
/*
 * at least one data thread is available:
 *  -  set number of remainign data threads equalt to available data threads
 *  (this values is used for syncig, ie. once the data thread is checked the coutner is decremented
 * -  set number of syncing threads to number of available threads + 1 (this is used to sync all processes - both this process and 
 * data threads are synced so that they all start at one point, Server_Body waits until all data threads arraive at syncing
 * point before signaling them to analyze the data (this is disabled, set in Start_Data_Thread and potentially
 * modified in pt_sync_mod()
 * - set data_set name, SR_Mode and socket number so that data_thread processes can start identification
 * - set the return value to 0, once the thread is identified, the value is set to 1
 */
				*Data_Threads->data_threads_remainth_counter 	= *Data_Threads->data_threads_availth_counter;	
				*Data_Threads->retval = 0;
				
				if( snprintf(Data_Threads->name_of_data_set, MAX_NAME_LENGTH,"%s",name_of_required_data_set) < 0)
					Perror("snprintf");
				*Data_Threads->SR_mode = SR_mode;
				*Data_Threads->socket  = newsockfd;

				Pthread_mutex_unlock(&Data_Threads->lock);
/*
 * once all necessary data are set, send signal to all threads to start unloc mutex
 * and release borrowed memory. The following syncing point is the same as the syncing point in Data_Thread.c
 */
				pt_sync(Data_Threads->sync);
/* 
 * when all Data_Thread are finished, - the identification part, the threads are waiting on each other. 
 * the last thread unlock the semaphore so that the next loop can start
 */		
				pt_sync(Data_Threads->sync);
/*
 * when data set is identified in Data_Thread the retval is set to 1
 * If all threads went attempted to evaluate the incoming request and 
 * none of them identifed the thread, give error message
 */
				if(*Data_Threads->retval == 1){
/*
 * data set was identified
 */
					if( m3l_Umount(&RecNode) != 1)
						Perror("m3l_Umount");
				}
				else{
/*
 * none of the data set was able to identify request, issue warnign and close socket
 */
					printf(" Case 0 retval (%d)  --- %s   %c\n", *Data_Threads->retval, name_of_required_data_set, SR_mode);
					Warning("Server_Body: Not valid data set");
					if( m3l_Umount(&RecNode) != 1)
						Perror("m3l_Umount");
					if( close(newsockfd) == -1)
						Perror("close");
					continue;
				}
			break;

			case 1:
/*
 * conenction is already occupied, all requests already arrived
 */
				if(SR_mode == 'S'){
/*
 * if process is sender, indicate Sender that header was received before receiving payload
 * if process is Receiver send acknowledgment and get back REOB
 */
// 				if( m3l_Send_to_tcpipsocket(RR_NEG, (const char *)NULL, newsockfd, "--encoding" , "IEEE-754",  (char *)NULL) < 1)
// 					Error("Server_Body: Error during sending data from socket");
					opts.opt_EOBseq = '\0'; // send EOFbuff sequence only
					if( m3l_send_to_tcpipsocket(Answers->RR_NEG, (const char *)NULL, newsockfd, Popts) < 1)
						Error("Server_Body: Error during sending data from socket");
					
				}
				else if(SR_mode == 'R'){
// 					m3l_Send_to_tcpipsocket(RR_NEG, (const char *)NULL, newsockfd, "--encoding" , "IEEE-754", (char *)NULL);
					opts.opt_EOBseq = '\0'; // send EOFbuff sequence only
					if( m3l_send_to_tcpipsocket(Answers->RR_NEG, (const char *)NULL, newsockfd, Popts) < 1)
						Error("Server_Body: Error during sending data from socket");
				}
				else{
					Error("Server_Body: Wrong SR mode\n");
				}
				
				
				if( close(newsockfd) == -1)
					Perror("close");
				if( m3l_Umount(&RecNode) != 1)
					Perror("m3l_Umount");
/*
 * data_thread is occupied let the process know it and close socket
 * process will attempt to establish connection later
 */
				Pthread_mutex_unlock(&Data_Threads->lock);
			
			break;
			
			case 100:
/*
 * system request is always done as a sender, indicate Sender that header was received before receiving payload
 * if process is Receiver send acknowledgment and get back REOB
 */
// 				opts.opt_EOBseq = '\0'; // send EOFbuff sequence only
// 				if( m3l_send_to_tcpipsocket(Answers->RR_POS, (const char *)NULL, newsockfd, Popts) < 1)
// 					Error("Server_Body: Error during sending data to socket");
/*
 * notify Data_Thread that this is a "system" request, ie. 
 * request which changes status of existing channels or adds a new one
 */
				*Data_Threads->checkdata = 100;

				Pthread_mutex_unlock(&Data_Threads->lock);
/*
 * let all Data_Threads waiting on pt_sync to make a step and enter the second pt_sync at the end 
 * of Data_Thread identification process. Before that set Data_Thread counter increment to 1
 */
				*Data_Threads->sync->incrm = 1;
				if( Add_Data_Thread(RecNode, Data_Threads, &DataBuffer) < 0){
					opts.opt_EOBseq = '\0'; // send EOFbuff sequence only
					if( m3l_send_to_tcpipsocket(Answers->RR_NEG, (const char *)NULL, newsockfd, Popts) < 1)
					Error("Server_Body: Error during sending data to socket");
				}
				else{
					opts.opt_EOBseq = '\0'; // send EOFbuff sequence only
					if( m3l_send_to_tcpipsocket(Answers->RR_POS, (const char *)NULL, newsockfd, Popts) < 1)
					Error("Server_Body: Error during sending data to socket");
				}
				printf(" Server added channel \n");
				if( close(newsockfd) == -1)
					Perror("close");
/*
 * This semaphore signalizes Server_Body that it can enter
 * the pt_sync. For case of adding thread, the Server body has just one pt_sync instead of two.
 * The first missing pt_sync is then provided by newly spawned Data_Thread. 
 * This semaphore makes sure that the pt_sync in the newly spawned Data_Thread is executed before pt_sync
 * in Server_Body so that the Server_Body pt_sync_mod is used in conunction with 
 * second pt_sync in Data_Threads
 */				
				Sem_wait(&Data_Threads->sem);
/* 
 * having mutex lock here caused dead-lock, needed to move it before Add_Data_Thread */
/*
 * delte borrowed memory, at this stage the 
 * node does not contain Channel subset, it was 
 * dettached from the node in Add_Data_Thread
 */
				if( m3l_Umount(&RecNode) != 1)
					Perror("m3l_Umount");
/*
 * this is the second pt_sync. The last thread will increase number of synced 
 * jobs by 1. Because there is already additional thread spawned by Add_Data_Thread, increase temporarily
 * the number of synced jobs
 */
				pt_sync_mod(Data_Threads->sync, 1, 1);
			break;
			
			case 200:
/*
 * notify Data_Thread that this is a "system" request, ie. 
 * request which changes status of existing channels or adds a new one
 */
				*Data_Threads->checkdata = 200;			
			break;

			case -1:
/*
 * wrong data set, possibly the name of connection does not exist
 */
				Pthread_mutex_unlock(&Data_Threads->lock);
				Warning("Server_Body: wrong connection request");
				
				if( close(newsockfd) == -1)
					Perror("close");
				if( m3l_Umount(&RecNode) != 1)
					Perror("m3l_Umount");
			break;
		}
/*
 * initial stage was completed, server is running in while(1) loop, set cycle to 1
 * needed for propper locking of mutex
 */
		cycle = 1;

	}      /* end of while(1) */
	
	if(Tqst_SFounds != NULL) m3l_DestroyFound(&Tqst_SFounds);
/*
 * join threads and release memmory
 */
	for(i=0; i< Data_Threads->n_data_threads; i++){
		if( Data_Threads->Data_Str[i]->data_threadPID != NULL){
			if( pthread_join(*Data_Threads->Data_Str[i]->data_threadPID, NULL) != 0)
				Error("Server_Body:  Joining thread failed");
		}
	
		free(Data_Threads->Data_Str[i]->data_threadPID);
		free(Data_Threads->Data_Str[i]->name_of_channel);
		free(Data_Threads->Data_Str[i]->status_run);
	}

	Pthread_mutex_destroy(&Data_Threads->lock);
	Pthread_cond_destroy(&Data_Threads->cond);
	Sem_destroy(&Data_Threads->sem);

	free(Data_Threads->name_of_data_set);
	free(Data_Threads->SR_mode);
	free(Data_Threads->data_threads_availth_counter);
	free(Data_Threads->data_threads_remainth_counter);
	free(Data_Threads->socket);
	free(Data_Threads->retval);
	free(Data_Threads->checkdata);
	
	free(Data_Threads->sync->nsync);
	free(Data_Threads->sync->nthreads);
	Pthread_mutex_destroy(&Data_Threads->sync->mutex);
	Pthread_mutex_destroy(&Data_Threads->sync->block);
	Pthread_cond_destroy(&Data_Threads->sync->condvar);
	Pthread_cond_destroy(&Data_Threads->sync->last);

	free(Data_Threads->sync);

	free(Data_Threads);
	
	if(m3l_Umount(&DataBuffer) != 1)
		Perror("m3l_Umount DataBuffer");

	DestroyPredefinedAnswers(&Answers);
	
	return 1;
}
