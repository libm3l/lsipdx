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

	int sockfd, portno, n, status, ch_stat, *tmpint;

        socklen_t clilen;
        struct sockaddr_in cli_addr;
	char *name="Pressure";

	int nmax;
	double *tmpdf;

	nmax = 5;
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
// 	for(i=0; i<nmax; i++){

// 		printf("\n\n--------------------------------    i = %ld\n\n", i);
/*
 * open socket, IP address of server is in argv[1], port number is in portno
 */
		Gnode = Header("Pressure", 'R');
		
		if(m3l_Cat(Gnode, "--all", "-P", "-L",  "*",   (char *)NULL) != 0)
			Error("CatData");
		
		if ( (sockfd =  m3l_cli_open_socket(argv[1], portno, (char *)NULL)) < 0)
			Error("Could not open socket");

		m3l_Send_receive_tcpipsocket(Gnode,(char *)NULL, sockfd, "--encoding" , "IEEE-754",  "--REOB", (char *)NULL);

		if(m3l_Umount(&Gnode) != 1)
			Perror("m3l_Umount");
		
		if( close(sockfd) == -1)
			Perror("close");
		
		
		
		
		
		Gnode = Header("Density", 'S');
		
		if(m3l_Cat(Gnode, "--all", "-P", "-L",  "*",   (char *)NULL) != 0)
			Error("CatData");
		
		if ( (sockfd =  m3l_cli_open_socket(argv[1], portno, (char *)NULL)) < 0)
			Error("Could not open socket");

		m3l_Send_receive_tcpipsocket(Gnode,(char *)NULL, sockfd, "--encoding" , "IEEE-754",  "--REOB", (char *)NULL);

		if(m3l_Umount(&Gnode) != 1)
			Perror("m3l_Umount");
		
		if( close(sockfd) == -1)
			Perror("close");
		
		
		
		Gnode = Header("Temperature", 'R');
		
		if(m3l_Cat(Gnode, "--all", "-P", "-L",  "*",   (char *)NULL) != 0)
			Error("CatData");
		
		if ( (sockfd =  m3l_cli_open_socket(argv[1], portno, (char *)NULL)) < 0)
			Error("Could not open socket");

		m3l_Send_receive_tcpipsocket(Gnode,(char *)NULL, sockfd, "--encoding" , "IEEE-754",  "--REOB", (char *)NULL);

		if(m3l_Umount(&Gnode) != 1)
			Perror("m3l_Umount");
		
		if( close(sockfd) == -1)
			Perror("close");
		
		
// 	}


     return 0; 
}