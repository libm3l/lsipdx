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
 *     Header file client_functions_Prt.h
 *
 *     Date: 2013-10-20
 * 
 *
 *     Modifications:
 *     Date		Version		Patch number		CLA 
 *
 *
 *     Description
 * 
 */




#ifndef  __FCE_CLIENT_FUNCTIONS_PRT_H__
#define  __FCE_CLIENT_FUNCTIONS_PRT_H__

#include "lsipdx_header.h"

#ifndef CPP_C
#ifdef __cplusplus 
#define CPP_C "C"
#else
#define CPP_C
#endif
#endif


extern CPP_C  lmint_t client_sender(void *, lmint_t, client_fce_struct_t *, opts_t *, opts_t *);
extern CPP_C  node_t *client_receiver(lmint_t , client_fce_struct_t *, opts_t *, opts_t *);

#endif
