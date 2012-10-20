/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_TRACING
 * @brief FreeRTOS PIOS application tags list definition
 * @{
 *
 * @file       pios_tracing.h  
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Application tags for tasks, used for tracing.
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#ifndef PIOS_TRACING_H
#define	PIOS_TRACING_H

#define PIOS_TAGS_OUTSIDETAGGEDTASK 0

#ifdef PIOS_TRACING


/* the following function need to be defined on a platform basis and 
 * represent the function that actually outputs the tag code during context 
 * switch.
 * It is recommended this function being an inline function and as short/fast 
 * as possible to prevent great performance impacts */
extern void PIOS_trace_impl(void * tag);

/* Define the traceTASK_SWITCHED_IN() macro to call the actual 
 * board specific tag log implementation */
#define traceTASK_SWITCHED_IN() PIOS_trace_impl(pxCurrentTCB->pxTaskTag)

/*  log the switch out as tag 0 */
#define traceTASK_SWITCHED_OUT() PIOS_trace_impl(0)  

#endif /* PIOS_TRACE_IMPL */

#endif	/* PIOS_TRACING_H */

/**
  * @}
  * @}
  */

