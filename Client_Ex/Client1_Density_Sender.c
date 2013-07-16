/*
 *     Copyright (C) 2012  Adam Jirasek
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *     
 *     contact: libm3l@gmail.com
 * 
 */



/*
 *     Function Client1.c
 *
 *     Date: 2013-02-23
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
#include "ACK.h"

int main(int argc, char *argv[])
{
	node_t *Gnode=NULL, *RecNode=NULL, *TmpNode = NULL;
	pid_t  childpid;
	size_t *dim, i, j;

	lmint_t sockfd, portno, n, status, ch_stat, *tmpint, *tmpi;

        socklen_t clilen;
        struct sockaddr_in cli_addr;
	lmchar_t *name="Pressure";

	lmint_t nmax, retval;
	lmdouble_t *tmpdf;
	
	struct timespec tim, tim2;
// 	tim.tv_sec = 1;
	tim.tv_sec = 0;
// 	tim.tv_nsec = 300000000L;    /* 0.1 secs */
	tim.tv_nsec = 20000000L;    /* 0.1 secs */

	nmax = 100000;
/*
 * get port number
 */
     if (argc < 3) {
       fprintf(stderr,"ERROR, no IPaddress and port number provided\n");
       exit(1);
     }
 	portno = atoi(argv[2]);
/*
 * open socket - because we use more then just send - receive scenario
 * we need to open socket manualy and used Send_receive function with hostname = NULL, ie. as server
 * portno is then replaced by socket number
 */
	for(i=0; i<nmax; i++){

 		printf("\n\n--------------------------------    i = %ld\n\n", i);
/*
 * open socket, IP address of server is in argv[1], port number is in portno
 */
		Gnode = Header("Density", 'S');

		dim = (size_t *) malloc( 1* sizeof(size_t));
		dim[0] = 1;
		if(  (TmpNode = m3l_Mklist("Iteration", "I", 1, dim, &Gnode, "/Header", "./", (char *)NULL)) == 0)
				Error("m3l_Mklist");
		tmpi = (lmint_t *)m3l_get_data_pointer(TmpNode);
		tmpi[0] = i;
		free(dim);
		
		if(m3l_Cat(Gnode, "--all", "-P", "-L",  "*",   (char *)NULL) != 0)
			Error("CatData");
again:		
		if ( (sockfd =  m3l_cli_open_socket(argv[1], portno, (char *)NULL)) < 0)
			Error("Could not open socket");

// 		m3l_Send_receive_tcpipsocket(Gnode,(char *)NULL, sockfd, "--encoding" , "IEEE-754",  "--REOB", (char *)NULL);
		
		if(  (TmpNode = m3l_Send_receive_tcpipsocket(Gnode,(char *)NULL, sockfd, "--encoding" , "IEEE-754", (char *)NULL)) == NULL)
			Error("Receiving data");
/*
 * get the value of the /RR/val
 */
		retval = TmpNode->child->data.i[0];
/*
 * if retval == 1 the data_thread is prepared to transmit the data, 
 * if retval == 0 the data_thread is busy, close socket and try again
 */
		printf(" Sending header %d\n", retval);
		
		if(retval == 0){
	
			if(m3l_Umount(&TmpNode) != 1)
			Perror("m3l_Umount");
			
			if( close(sockfd) == -1)
				Perror("close");			
			if(nanosleep(&tim , &tim2) < 0 )
				Error("Nano sleep system call failed \n");
			
			printf("Sender -- Attemtping to send Header data again\n");

			
			goto again;
		}
		
		if(m3l_Umount(&Gnode) != 1)
			Perror("m3l_Umount");
		
		
		
		
		
		
		
		
		



		printf(" Creating payload \n");
		
		Gnode = client_name("Text from Client1");
	
		dim = (size_t *) malloc( 1* sizeof(size_t));
		dim[0] = 1;
/*
 * add iteraztion number
 */
		if(  (TmpNode = m3l_Mklist("Iteration_Number", "I", 1, dim, &Gnode, "/Client_Data", "./", (char *)NULL)) == 0)
				Error("m3l_Mklist");
		TmpNode->data.i[0] = i;
/*
 * add pressure array, array has 10 pressure with some values
 */	
		dim[0] = 5;
		if(  (TmpNode = m3l_Mklist("numbers", "D", 1, dim, &Gnode, "/Client_Data", "./", (char *)NULL)) == 0)
				Error("m3l_Mklist");
		tmpdf = (double *)m3l_get_data_pointer(TmpNode);
		for(j=0; j<5; j++)
			tmpdf[j] = (i+1)*j*3.1415926;
		free(dim);

		if(m3l_Cat(Gnode, "--all", "-P", "-L",  "*",   (char *)NULL) != 0)
			Error("CatData");

		m3l_Send_receive_tcpipsocket(Gnode,(char *)NULL, sockfd, "--encoding" , "IEEE-754",  "--REOB", (char *)NULL);
// 		printf(" after sending payload \n");

// 		m3l_Send_to_tcpipsocket(Gnode,(char *)NULL, sockfd, "--encoding" , "IEEE-754", (char *)NULL);
// 		printf(" after sending payload \n");
// 		m3l_Receive_tcpipsocket((char *)NULL, sockfd, "--encoding" , "IEEE-754",  "--REOB", (char *)NULL);
// 		printf(" after --REOB \n");
		
		if(m3l_Umount(&Gnode) != 1)
			Perror("m3l_Umount");
		
		if( close(sockfd) == -1)
			Perror("close");
		
		if(nanosleep(&tim , &tim2) < 0 )
			Error("Nano sleep system call failed \n");
 	}


     return 0; 
}