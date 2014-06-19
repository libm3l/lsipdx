/*
 *     Copyright (C) 2014  Adam Jirasek
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
 *     Function ACK.c
 *
 *     Date: 2014-06-18
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
#include "ACK.h"


node_t *ackn(void)
{
	node_t *Gnode, *TmpNode;
	char *answer="ACKN";
	size_t *dim;
	
	if(  (Gnode = m3l_Mklist("Answer", "DIR", 0, 0, (node_t **)NULL, (const char *)NULL, (const char *)NULL, (char *)NULL)) == 0)
		Perror("m3l_Mklist");
	
	dim = (size_t *) malloc( 1* sizeof(size_t));
	dim[0] = strlen(answer)+1;
	
	if(  (TmpNode = m3l_Mklist("ANSWER", "C", 1, dim, &Gnode, "/Answer", "./", "--no_malloc", (char *)NULL)) == 0)
		Error("m3l_Mklist");
	TmpNode->data.c = answer;
	TmpNode->data.c[dim[0]] = '\0';
	free(dim);
	
 	return Gnode;
}


node_t *ret_receipt(lmint_t val)
{
	node_t *Gnode, *TmpNode;
	size_t *dim;
	
	if(  (Gnode = m3l_Mklist("RR", "DIR", 0, 0, (node_t **)NULL, (const char *)NULL, (const char *)NULL, (char *)NULL)) == 0)
		Perror("m3l_Mklist");
	
	dim = (size_t *) malloc( sizeof(size_t));
	dim[0] = 1;
	
	if(  (TmpNode = m3l_Mklist("val", "I", 1, dim, &Gnode, "/RR", "./", (char *)NULL)) == 0)
		Error("m3l_Mklist");
	TmpNode->data.i[0] = val;
	
	free(dim);
		
 	return Gnode;
}

node_t *client_name(char *name)
{
	node_t *Gnode, *TmpNode;
	size_t *dim;
	
	if(  (Gnode = m3l_Mklist("Client_Data", "DIR", 0, 0, (node_t **)NULL, (const char *)NULL, (const char *)NULL, (char *)NULL)) == 0)
		Perror("m3l_Mklist");
	
	dim = (size_t *) malloc( 1* sizeof(size_t));
	dim[0] = strlen(name)+1;
	
	if(  (TmpNode = m3l_Mklist("Name", "C", 1, dim, &Gnode, "/Client_Data", "./", "--no_malloc", (char *)NULL)) == 0)
		Error("m3l_Mklist");
	TmpNode->data.c = name;
	
	free(dim);
	
 	return Gnode;
}


node_t *Header(lmchar_t *name, lmchar_t RWmode){

	node_t *Gnode, *TmpNode;
	lmsize_t dim[1];
	
	if(RWmode == 'S' || RWmode == 'R'){
/*
 * sender/receiver request
 */

		if(  (Gnode = m3l_Mklist("Header", "DIR", 0, 0, (node_t **)NULL, (const char *)NULL, (const char *)NULL, (char *)NULL)) == 0)
			Perror("m3l_Mklist");
		
// 		dim = (size_t *) malloc( 1* sizeof(size_t));
		dim[0] = strlen(name)+1;
		
		if(  (TmpNode = m3l_Mklist("Name_of_Channel", "C", 1, dim, &Gnode, "/Header", "./", "--no_malloc", (char *)NULL)) == 0)
			Error("m3l_Mklist");
		TmpNode->data.c = name;
		
		
		dim[0] = 2;
		
		if(  (TmpNode = m3l_Mklist("SR_mode", "C", 1, dim, &Gnode, "/Header", "./", (char *)NULL)) == 0)
			Error("m3l_Mklist");
		TmpNode->data.c[0] = RWmode;
		TmpNode->data.c[1] = '\0';
// 		free(dim);
		
		return Gnode;
	}
	else if(RWmode == 'X'){
/*
 * system request
 */
		if(  (Gnode = m3l_Mklist("_sys_comm_", "DIR", 0, 0, (node_t **)NULL, (const char *)NULL, (const char *)NULL, (char *)NULL)) == 0)
			Perror("m3l_Mklist");
		dim[0] = 1;
		if(  (TmpNode = m3l_Mklist("request_type", "I", 1, dim, &Gnode, "/_sys_comm_", "./", (char *)NULL)) == 0)
			Error("m3l_Mklist");
		TmpNode->data.i[0] = 100;
		return Gnode;
	}
	else
		return NULL;
}


lsipdx_answer_t *MakePredefinedAnswers(){
/*
 * functions creates predefined answers
 */
	lsipdx_answer_t *Answers;
	
	if(  (Answers = (lsipdx_answer_t *)malloc(sizeof(lsipdx_answer_t))) == NULL)
		Error("Server_Body: error while malloced Answers");
	
	Answers->RR_POS = ret_receipt(1);
	Answers->RR_NEG = ret_receipt(0);
	Answers->RR_WRREQ = ret_receipt(-1);
	Answers->RR_WRCONREQ = ret_receipt(2);
	
	return Answers;
}

void DestroyPredefinedAnswers(lsipdx_answer_t **Answers){
/*
 * free memory in predefined answers
 */
	if(m3l_Umount(&(*Answers)->RR_NEG) != 1)
		Perror("m3l_Umount RR_NEG");
	if(m3l_Umount(&(*Answers)->RR_POS) != 1)
		Perror("m3l_Umount RR_POS");	
	if(m3l_Umount(&(*Answers)->RR_WRREQ) != 1)
		Perror("m3l_Umount RR_WRREQ");
	if(m3l_Umount(&(*Answers)->RR_WRCONREQ) != 1)
		Perror("m3l_Umount RR_WRREQ");

	free(*Answers);
	*Answers = NULL;
}

node_t * ChannelList(lmchar_t *name, lmsize_t Rproc, lmchar_t ATDT_mode, lmchar_t KA_mode){
/*
 * function makes a node with details about Channel
 * example is below
 *
 *
 *  -DIR     Channel                 7
 *  -C                Name_of_Channel 1 ...
 *                            Density
 *  -I                Sending_Process  1 1
 *                                1 
 *  -ST               Receiving_Processes  1 1
 *                                Rproc 
 *   -DIR             CONNECTION   
 *   -C                            ATDT_Mode 1  2
 *                                       ATDT_mode
 *   -C                            KEEP_CONN_ALIVE_Mode 1  2
 *                                       KA_mode
 */                                       
	node_t *RetNode = NULL, *TmpNode = NULL, *TmpNode1=NULL;
	lmsize_t dim[1], *tmpsize;
	lmchar_t *tmpchar;
	lmint_t *tmpint;
/*
 * start this request with _sys_comm_ name
 * once it arrives and is identified as a _sys_comm_
 * it will be renamed to Channel
 */
	if(  (RetNode = m3l_Mklist("_sys_comm_", "DIR", 0, 0, (node_t **)NULL, (const char *)NULL, (const char *)NULL, (char *)NULL)) == 0)
		Perror("ChannelList: Mklist");
	
	if( (dim[0] = strlen(name)+1) < 1)
		Error("ChannelList: wrong name of channel");
	
	if(  (TmpNode = m3l_Mklist("Name_of_Channel", "C", 1, dim, &RetNode, "./Channel", "./", (char *)NULL)) == 0)
		Error("Allocate_DataBuffer: m3l_Mklist");
	tmpchar = (lmchar_t *)m3l_get_data_pointer(TmpNode);
	if( snprintf(tmpchar, dim[0],"%s",name) < 0)
		Perror("snprintf");
	tmpchar[dim[0]] = '\0';
	
	dim[0] = 1;
	if(  (TmpNode = m3l_Mklist("Sending_Process", "I", 1, dim, &RetNode, "./Channel", "./", (char *)NULL)) == 0)
		Error("Allocate_DataBuffer: m3l_Mklist");
	tmpint = (lmint_t *)m3l_get_data_pointer(TmpNode);
	tmpint[0] = 1;

	if(  (TmpNode = m3l_Mklist("Receiving_Processes", "ST", 1, dim, &RetNode, "./Channel", "./", (char *)NULL)) == 0)
		Error("Allocate_DataBuffer: m3l_Mklist");
	tmpsize = (lmsize_t *)m3l_get_data_pointer(TmpNode);
	tmpsize[0] = Rproc;
	
	if(  (TmpNode = m3l_Mklist("CONNECTION", "DIR", 0, 0, &RetNode, "./Channel", "./", (char *)NULL)) == 0)
		Perror("ChannelList: Mklist");
	
	dim[0] = 2;
	if(  (TmpNode1 = m3l_Mklist("ATDT_Mode", "C", 1, dim, &TmpNode, "./CONNECTION", "./", (char *)NULL)) == 0)
		Error("m3l_Mklist");
	TmpNode1->data.c[0] = ATDT_mode;
	TmpNode1->data.c[1] = '\0';

	if(  (TmpNode1 = m3l_Mklist("KEEP_CONN_ALIVE_Mode", "C", 1, dim, &TmpNode, "./CONNECTION", "./", (char *)NULL)) == 0)
		Error("m3l_Mklist");
	TmpNode1->data.c[0] = ATDT_mode;
	TmpNode1->data.c[1] = '\0';


	return RetNode;
	
}