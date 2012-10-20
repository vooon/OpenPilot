/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_TRACING
 * @brief FreeRTOS PIOS application tags list definition
 * @{
 *
 * @file       pios_app_tags.h  
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


#ifndef PIOS_APP_TAGS_H
#define	PIOS_APP_TAGS_H
// do not use 0 as APP tag as it is used to signal a task switch out
#define PIOS_TAGS_OUTSIDETAGGEDTASK 0

#define PIOS_TAGS_IDLE 1
#define PIOS_TAGS_SYSTEM 2
#define PIOS_TAGS_TELEMETRY 3
#define PIOS_TAGS_SENSORS 4
#define PIOS_TAGS_STABILIZATION 5
#define PIOS_TAGS_MANUALCONTROL 6
#define PIOS_TAGS_ATTITUDE 7
#define PIOS_TAGS_ACTUATOR 8
#define PIOS_TAGS_GPS 9
#define PIOS_TAGS_RADIO_ST 10
#define PIOS_TAGS_RADIO_RX 11

#ifdef PIOS_TRACING
#define PIOS_TAG(x) vTaskSetApplicationTaskTag( NULL, ( void * ) x );
#else
#define PIOS_TAG(x) 
#endif

#endif	/* PIOS_APP_TAGS_H */

/**
  * @}
  * @}
  */

