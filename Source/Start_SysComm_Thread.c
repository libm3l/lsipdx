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
 *     Function Start_SysComm_Thread.c
 *
 *     Date: 2014-03-08
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
#include "Start_SysComm_Thread.h"
#include "Sys_Comm_Channel.h"
#include "Server_Functions_Prt.h"
#include "Associate_Data_Thread.h"

lmsize_t Start_SysComm_Thread(data_thread_str_t *Data_Thread){
/*
 */
	lmsize_t i, retval;
	lmint_t pth_err;
	Server_Comm_DataStr_t *SysCommDatSet;
/*
 * spawn threads
 */	
// 	if( (SysComData = (Server_Comm_DataStr_t *)malloc(sizeof(Server_Comm_DataStr_t))) == NULL)
// 		Perror("Start_SysComm_Thread: SysComData malloc");
// 
// 	if( (SysComData->data_threads = (pthread_t *)malloc(sizeof(pthread_t))) == NULL)
// 		Perror("Start_SysComm_Thread: SysComData->data_threads malloc");
// 	
// 	if( (SysComData->Data_Thread_Pointer = (data_thread_args_t *)malloc(sizeof(data_thread_args_t))) == NULL)
// 		Perror("Start_SysComm_Thread: SysComData->Data_Thread_Pointer malloc");
// 
// 	if( (SysComData->SR_Data_Thread_Pointer = (SR_thread_str_t *)malloc(sizeof(SR_thread_str_t))) == NULL)
// 		Perror("Start_SysComm_Thread: SysComData->SR_Data_Thread_Pointer malloc");

// 		while ( ( pth_err = pthread_create(Data_Thread->Data_Str->data_threadPID, NULL, &Data_Threads,  DataArgs)) != 0 && errno == EAGAIN);
// 		if(pth_err != 0)
// 			Perror("pthread_create()");
/*
 * create a node
 */	
	return retval;
}