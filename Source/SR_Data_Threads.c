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
 *     Function SR_Data_Threads.c
 *
 *     Date: 2013-09-08
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
#include "Server_Functions_Prt.h"
#include "SR_Data_Threads.h"

#define INTEGMIN(X,Y) ((X) < (Y) ? (X) : (Y)); 

static inline lmssize_t Read(lmint_t, lmchar_t *, lmint_t, lmchar_t);
static inline lmssize_t Write(lmint_t, lmchar_t *, lmsize_t);

static lmint_t R_KAN(SR_thread_args_t *, lmint_t, lmint_t);
static lmint_t S_KAN(SR_thread_args_t *, lmint_t, lmint_t);

static lmint_t R_EOFC(lmint_t);
static lmssize_t S_EOFC(lmint_t, lmint_t);

//      mode 1: ATDTMode == 'D' && KeepAlive_Mode == 'N'  /* Direct transfer, close socket */
//      mode 2: ATDTMode == 'A' && KeepAlive_Mode == 'N'  /* Alternate transfer, close socket */
//      mode 3: ATDTMode == 'D' && KeepAlive_Mode == 'C'  /* Direct transfer, close socket after client request it*/
//      mode 4: ATDTMode == 'A' && KeepAlive_Mode == 'C'  /* Alternate transfer, close socket after client request it*/
//      mode 5: ATDTMode == 'D' && KeepAlive_Mode == 'Y'  /* Direct transfer, do not close socket*/
//      mode 6: ATDTMode == 'A' && KeepAlive_Mode == 'Y'  /* Alternate transfer, do not close socket*/

void *SR_Data_Threads(void *arg)
{
	SR_thread_args_t *c = (SR_thread_args_t *)arg;

	lmchar_t SR_mode;
	lmint_t sockfd, retval;
	lmsize_t i;
/*
 * sync all SR_Threads and SR_hub so that Data_Thread goes further once they 
 * are all spawned. Without that there were problems with case 200 where Data_Thread 
 * sometimes deleted List before SR_Hub started, upon start SR_hub needs to identify 
 * some values from the list.
 * 
 * The value of processes which are synced on this pt_sync_mod is increased by 2, ie.
 * number of SR_Data_Threads + SR_Hub + Data_Thread
 */	
	pt_sync_mod(c->psync_loc, 0, 2);
/* 
 * get SR_mode and socket number, unlock so that other SR_threads can get ther
  * increase counter so that next job can grab it.
*/
	while(1){
/*
 * wait until all requests (all Receiver + Sender) for a particular data_set arrived 
 * the last invoking of pt_sync_mod is done in SR_hub.c
 * 
 * 
 *  *c->pthr_cntr local counter uniques for all SR_Data_Threads for one SR_Hub, upon start, all
 * SR_Data_Thread will set it to 0 and then wait on pt_sync until all requests arrive.
 * One they arrive, SR_Data_Threads will grab them one by one, getting socket number and SR_mode from an array which is filled in
 * Data_Thread (SR_Threads->sockfd) 
 */
		*c->pthr_cntr = 0;
/*
 * wait until all SR_threads reach pt_sync, then start actual transfer of the data from S to R(s).
 * This counter include all SR_Data_Threads and SR_Hub.
 * becasue the internal counter of synced jobs is set to S+R, we have to add 1 so that SR_Hub is 
 * synced too
 */
		pt_sync_mod(c->psync_loc, 0, 1);
/*
 * get SR_mode and socket number of each connected processes
 * protext by mutex
 */
		Pthread_mutex_lock(c->plock);

			SR_mode =  c->pSR_mode[*c->pthr_cntr];
			sockfd  =  c->psockfd[*c->pthr_cntr];
			(*c->pthr_cntr)++; 

		Pthread_mutex_unlock(c->plock);
/*
 * decide which mode is used; depends on KeepAlive and ATDT option
 * the value of mode set in SR_hub.c
 */
		switch(*c->pSRt_mode){
/*  -------------------------------------------------------------- */
		case 1:
/*
 * do not keep socket allive, ie. open and close secket every time the data transfer occurs
 */
			switch(SR_mode){
				case 'R':
/*
 * R(eceivers)
 */
					if( R_KAN(c, sockfd, 1) == -1) return NULL;
				break;

				case 'S':
/*
 * S(ender)
 */
					if( S_KAN(c, sockfd, 1) == -1) return NULL;
				break;
				
				case 'T':
/*
 * thread is to be terminated sync all S and R SR_Data_Threads
 */
					retval = pt_sync(c->psync_loc);
/*
 * signal the SR_hub and it can do another cycle, it is done by the last process leaving the 
 * pt_sync()
 */
					if(retval == 1)Sem_post(c->psem_g);
					goto END;
					
				break;

				default:
					Error("SR_Data_Threads: Wrong SR_mode");
				break;
			}
		break;
/*  -------------------------------------------------------------- */
		case 2:
/*
 * ATDT mode == A, the Receiver will receive the data and then send 
 * back to Sender, Sender will first send the data and then receive from Receiver
 * works only for 1 R process
 * valid only of one Receiver, do not need to sync or barrier betwen swithich flod direciton
 */
			
			switch(SR_mode){
				case 'R':
/*
 * R(eceivers)
 * when finishing with R, do not signal SR_hub to go to another loop, 
 * the Receiver process will now send the data 
 */
					if( R_KAN(c, sockfd, 0) == -1) return NULL;
/*
 * last Pthread_barrier_wait is done in SR_hub.c
 *
 * wait until all SR_threads reach pt_sync, then start actual transfer of the data from S to R(s)
 * becasue the internal counter of synced jobs is set to S+R, we have to add 1 so that SR_Hub is 
 * synced too
 */
					if( S_KAN(c, sockfd, 2) == -1) return NULL;

				break;

				case 'S':
/*
 * S(ender), after finishing sending, receive the data
 * after that signal SR_hub that SR operation is finished and it can do 
 * another loop
 */
					if( S_KAN(c, sockfd, 0) == -1) return NULL;
/*
 * last Pthread_barrier_wait is done in SR_hub.c
 *
 * wait until all SR_threads reach pt_sync, then start actual transfer of the data from S to R(s)
 * becasue the internal counter of synced jobs is set to S+R, we have to add 1 so that SR_Hub is 
 * synced too
 */
					if( R_KAN(c, sockfd, 2) == -1) return NULL;

				break;

				case 'T':
/*
 * thread is to be terminated sync all S and R SR_Data_Threads
 */
					retval = pt_sync(c->psync_loc);
/*
 * signal the SR_hub and it can do another cycle, it is done by the last process leaving the 
 * pt_sync()
 */
					if(retval == 1)Sem_post(c->psem_g);
					goto END;
					
				break;

				default:
					Error("SR_Data_Threads: Wrong SR_mode");
				break;
			}
		break;

		case 5:  /* same as mode 1, do not close socket and do not signal SR_hub */
/*
 * keep socket alive forever
 */
			switch(SR_mode){
				case 'R':
/*
 * R(eceivers)
 */
					while(1){

						if(R_KAN(c, sockfd, 5) != 1) return NULL;
/* 
 * if connection required to be closed, terminate while loop
 */
						if(*c->pstatus_run != 1) goto END1;
					}

				break;

				case 'S':
/*
 * S(ender)
 */
					while(1){

						if( S_KAN(c, sockfd, 5) != 1) return NULL;
/* 
 * if connection required to be closed, terminate while loop
 */
						if(*c->pstatus_run != 1) goto END1;
					}

				break;

				case 'T':
/*
 * thread is to be terminated
 */
				break;

				default:
					Error("SR_Data_Threads: Wrong SR_mode");
				break;
			}
		break;
/*  -------------------------------------------------------------- */
		case 6:   /* same as mode 2, do not close socket and do not signal SR_hub */
/*
 * ATDT mode == A, the Receiver will receive the data and then send 
 * back to Sender, Sender will first send the data and then receive from Receiver
 * works only for 1 R process
 */
			switch(SR_mode){
				case 'R':
/*
 * R(eceivers)
 * when finishing with R, do not signal SR_hub to go to another loop, 
 * the Receiver process will now send the data 
 */
					while(1){

						if( R_KAN(c, sockfd, 0) == -1) return NULL;
/*
 * last Pthread_barrier_wait is done in SR_hub.c
 *
 * wait until all SR_threads reach pt_sync, then start actual transfer of the data from S to R(s)
 * becasue the internal counter of synced jobs is set to S+R, we have to add 1 so that SR_Hub is 
 * synced too
 */
						if( S_KAN(c, sockfd, 0) == -1) return NULL;
/* 
 * if connection required to be closed, terminate while loop
 */
						if(*c->pstatus_run != 1) goto END1;
					}
				break;

				case 'S':
/*
 * S(ender), after finishing sending, receive the data
 * after that signal SR_hub that SR operation is finished and it can do 
 * another loop
 */
					while(1){

						if( S_KAN(c, sockfd, 0) == -1) return NULL;
/*
 * last Pthread_barrier_wait is done in SR_hub.c
 *
 * wait until all SR_threads reach pt_sync, then start actual transfer of the data from S to R(s)
 * becasue the internal counter of synced jobs is set to S+R, we have to add 1 so that SR_Hub is 
 * synced too
 */
						if( R_KAN(c, sockfd, 0) == -1) return NULL;
/* 
 * if connection required to be closed, terminate while loop
 */
						if(*c->pstatus_run != 1) goto END1;
					}
				break;

				case 'T':
/*
 * thread is to be terminated
 */
				break;

				default:
					Error("SR_Data_Threads: Wrong SR_mode");
				break;
			}
		break;
		
		default:
			Error("SR_Data_Threads: Wrong mode - check KeepAlive and ATDT specifications");
		break;
		}
	}
	
END1:
/*
 * close sockets, used by case 5,6 which do not close socket
 * themselves. Case 1,2 close their own socket, that's why they go directly
 * to END 
 * 
 * to check if the socket is opened, check its value. 
 * if 0, it was already closed
 */
	for(i=0; i<*c->psync_loc->nthreads; i++){
		if(c->psockfd[i] > 0){
			if( close(c->psockfd[i]) == -1)
				Perror("close");
			c->psockfd[i] = 0;
		}
	}
END:
/*
 * case 1,2  ends here, they close their opened sockets
 * themeselves, free meory and Return, thread wil be joined in Data_Thread.c
 */
	free(c);
	return NULL;
}


/*
 * socket read and write function prototypes
 */
lmssize_t Write(lmint_t sockfd,  lmchar_t *buffer, lmsize_t size){
/*
 * write buffer to socket
 */
	lmssize_t total, n;	
	total = 0;
	lmchar_t *buff;

	buff = buffer;
	
	while(size > 0) {
		
		if ( (n = write(sockfd,buff,size)) < 0){
			if (errno == EINTR) continue;
			return (total == 0) ? -1 : total;
		}
 		buff  += n;
		total += n;
		size  -= n;
	}

	return total;
}


lmssize_t Read(lmint_t descrpt , lmchar_t *buff, lmint_t n, lmchar_t SR)
{
	lmsize_t ngotten;

	if (  (ngotten = read(descrpt,buff,n)) == -1){
		
		Perror("SR_Data_Threads - Read");
		return -1;
	}
	buff[ngotten] = '\0';

	return ngotten;
}


/*
 * Recevier function, ATDT A,D  KeepAllive N
 */
lmint_t R_KAN(SR_thread_args_t *c, lmint_t sockfd, lmint_t mode){

	lmint_t  R_done, last, retval;
	opts_t *Popts, opts;
	lmssize_t n;
	Popts = &opts;
	
	m3l_set_Send_receive_tcpipsocket(&Popts);
/*
 * Receiver threads, set R_done = 0, once the 
 * transfer of entire message is done (ie. Sender sends EOMB sequence
 * set R_done = 1
 * 
 */
	R_done = 1;
/*
 * thread reads the data from buffer and send over TCP/IP to client
 */
	while(R_done == 1){
/*
 * last thread will set last = 1 
 */
		last = 0;
/*
 * gate syncing all threads, syncing for Sender is done after reading from socket;
 * after sycning, check that the Sender received EOFbuff (ie the last sequence of the 
 * entirte transmitted message), if yes, set R_done = 1
 */
		pt_sync(c->psync_loc);
				
		if(*c->pEofBuff != 0){
			R_done = 1;}
		else{
			R_done = 0;}
/*
 * the mutex was locked here to protect writing to each individual sockets
 * but I think it is  not needed, moved lock after 
 */
		if ( (n = Write(sockfd,c->pbuffer, *c->pngotten)) < *c->pngotten){
			Perror("write()");
			return -1;
		}
/* prcounter is counter of R_threads which still have not 
 * read wrote the buffer to TCP/IP socket
 * The values is reset in S_KAN each time S_KAn reads from socket
 */ 
		Pthread_mutex_lock(c->plock);
			(*c->prcounter)--;
			*c->psync = 0;
					
		if(*c->prcounter == 0){
/*
 * if this is the last Receiver thread, set last to 1
 */
			last = 1;
/* 	
 * the last thread, broadcast
 * set number of remaining threads equal to number of reading threads (only if reading will be repeated, otherwise keep it 0)
 * indicate this is the last thread
 */
			*c->psync = 1;
			Pthread_cond_broadcast(c->pdcond);
/*
 * clean buffer
 */
			bzero(c->pbuffer, MAXLINE+1);
			*c->pngotten = 0;
/*
 * signal Sender that all Receivers are ready for next 
 * round of transmition
 */
			Sem_post(c->psem);
/* 
 * unlock semaphore in the main program so that another loop can start
 */
		}
		else{
/*
 * still some threads working, wait for them
 * indicate this is waiting thread
 */
			while (*c->psync != 0)
				Pthread_cond_wait(c->pdcond, c->plock);
		}

		Pthread_mutex_unlock(c->plock);
	}
/*
 * EOFbuff received, transmition is finished
 * 
 * Reading process sends signal that the it received all data (ie. 
 * 	----- m3l_Send_to_tcpipsocket(NULL,(char *)NULL, sockfd, "--encoding" , "IEEE-754", "--SEOB", (char *)NULL);
 * it is just to make sure all processes are done with transfer
 * do in only ff ATDT mode is D
 */
	retval = 1;

	switch(mode){
		
		case 0:

		break;

		case 1:
		case 2:
			opts.opt_REOBseq = 'G'; // receive EOFbuff sequence only
			if( m3l_receive_tcpipsocket((const lmchar_t *)NULL, sockfd, Popts) < 0){
				Error("SR_Data_Threads: Error when receiving  REOB\n");
				return -1;
			}
/*
 * close the socket 
 */
			if( close(sockfd) == -1)
				Perror("close");
		break;

			
		case 5:
		case 6:
/*
 * same as case 1 and 2, just do not close the socket
 */
			opts.opt_REOBseq = 'G'; // receive EOFbuff sequence only
			if( m3l_receive_tcpipsocket((const lmchar_t *)NULL, sockfd, Popts) < 0){
				Error("SR_Data_Threads: Error when receiving  REOB\n");
				return -1;
			}

		break;
		
		default:
			Error("R_KAN: Wrong mode");
		break;
	}
/*
 * syncing all R and S threads, all sockets are now closed (if required them to be closed ) 
 */
	switch(mode){
	case 0:
		pt_sync(c->psync_loc); 
	break;

	case 1:
	case 2:
		pt_sync(c->psync_loc); 
/*
 * signal the SR_hub and it can do another cycle
 */
		if(last == 1)Sem_post(c->psem_g);
	break;
	}
	return retval;
}


/*
 * Sender function, ATDT A,D  KeepAllive N
 */
lmint_t S_KAN(SR_thread_args_t *c, lmint_t sockfd, lmint_t mode){

	lmchar_t prevbuff[EOBlen+1];
	lmint_t eofbuffcond, retval;
	opts_t *Popts, opts;
	Popts = &opts;

	m3l_set_Send_receive_tcpipsocket(&Popts);

	bzero(prevbuff, EOBlen+1);
/*
 * thread reads data from TCP/IP socket sent by client and 
 * write them to buffer
 * 
 */
	eofbuffcond = 0;

	while(eofbuffcond != 1){
/*
 * set counter of Receiving threads to number of R_threads (used in synchronizaiton of R_Threads)
 */
		*c->prcounter = *c->pcounter-1;
		*c->pEofBuff = 1;

		bzero(c->pbuffer,MAXLINE+1);
		if (  (*c->pngotten = Read(sockfd, c->pbuffer, MAXLINE, 'R')) == -1){
			Perror("read");
			free(c);
			return -1;
		}

		eofbuffcond = Check_EOFbuff(c->pbuffer,prevbuff, strlen(c->pbuffer), EOBlen, EOFbuff);
/*
 * The buffer has been red from socket, send broadcast signal to all R_threads to go on
 * then unlock mutex and wait for semaphore
 * if end of entire transfer, set pEofBuff = 0, ie. sigal R_Threads that this is the 
 * very last write to socket
 */
		if(eofbuffcond == 1)
			*c->pEofBuff = 0;
/*
 * wait on synchronization point, the syncing for Receivers is done before writing the 
 * buffer to socket
 */
		pt_sync(c->psync_loc);
/*
 * wait until all Receivers sent the data to the socket
 */
		Sem_wait(c->psem);
/*
 * if end of buffer reached, leave do cycle
 */
	}
/*
 * sender sent payload, before closign socket send back acknowledgement --SEOB, Sender receives --REOB
 * do it only if ATDT mode == D
 */
	retval = 1;

	switch(mode){
		
		case 0:

		break;
		
		case 1:
		case 2:
			opts.opt_EOBseq = 'E'; // send EOFbuff sequence only	
			if( m3l_send_to_tcpipsocket((node_t *)NULL, (const lmchar_t *)NULL, sockfd, Popts) < 0){
				Error("SR_Data_Threads: Error when sending  SEOB\n");
				return -1;
			}
/*
 * close the socket 
 */
			if( close(sockfd) == -1)
				Perror("close");
		break;

		case 5:  
		case 6:
/*
 * same as case 1 and 2, just do not close the socket
 */
			opts.opt_EOBseq = 'E'; // send EOFbuff sequence only	
			if( m3l_send_to_tcpipsocket((node_t *)NULL, (const lmchar_t *)NULL, sockfd, Popts) < 0){
				Error("SR_Data_Threads: Error when sending  SEOB\n");
				return -1;
			}
		break;
		
		default:
			Error("S_KAN: Wrong mode");
		break;
	}
/*
 * synck before letting SR_hub to close sockets
 */
	switch(mode){
	case 0:
	case 1:
	case 2:
		pt_sync(c->psync_loc);
	break;
	}
	
	return retval;
}


lmint_t R_EOFC(lmint_t sockfd){
/*
 * receive the EOFC sequence, look at the first byte
 * value and return back; the values signals if the client request
 * closing scoket or keep it opened
 */
	lmchar_t buff[EOFClen+1], allbuff[EOFClen+1], *pc;
	lmssize_t ngotten, nreceived;
	lmsize_t i;
	
	lmint_t retval;

	nreceived = 0;
	pc = &allbuff[0];

	do{
/*
 * bzero buffer
 */		bzero(buff,sizeof(buff));
		if (  (ngotten = Read(sockfd, buff, EOFClen, 'A')) == -1)
 			Perror("read");
		
		nreceived = nreceived + ngotten;
		
		for(i=0; i< ngotten; i++)
			*pc++ = *(buff+i);
		
	}while(nreceived < EOFClen);
/* 
 * allbuff contains entire segment, which consits of a number and EOFbuff sequence
 * check the first byte and get the value
 */ 
	retval = allbuff[0] - '0';
	return retval;
}


lmssize_t S_EOFC(lmint_t sockfd, lmint_t val){
/*
 * send the value of the EOFC signaling whether
 * client requests or does not request to close the socket
 */
	lmssize_t total, n;
	total = 0;
	lmchar_t Echo[EOFClen+1], *buff;
	lmsize_t size;
	
	if(val == 1){
		if( snprintf(Echo, EOFClen+1 ,"%s", EOFCY) < 0)
			Perror("snprintf");
	}
	else if(val == 1){
		if( snprintf(Echo, EOFClen+1 ,"%s", EOFCN) < 0)
			Perror("snprintf");
	}
	else
		Error("S_EOFC: Wrong value of val parameter");

	
	Echo[EOFClen] = '\0';
	size = EOFClen + 1;
	
	buff = Echo;
	
	while(size > 0) {
		
		if ( (n = write(sockfd,buff,size)) < 0){
			if (errno == EINTR) continue;
			return (total == 0) ? -1 : total;
		}
 		buff += n;
		total += n;
		size -= n;
	}
/*
 * buffer was sent
 */
	return total;
}
