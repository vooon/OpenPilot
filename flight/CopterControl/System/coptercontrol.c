/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @brief These files are the core system files of OpenPilot.
 * They are the ground layer just above PiOS. In practice, OpenPilot actually starts
 * in the main() function of openpilot.c
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @brief This is where the OP firmware starts. Those files also define the compile-time
 * options of the firmware.
 * @{
 * @file       openpilot.c 
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Sets up and runs main OpenPilot tasks.
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


/* OpenPilot Includes */
#include "openpilot.h"
#include "uavobjectsinit.h"
#include "systemmod.h"

/* Task Priorities */
#define PRIORITY_TASK_HOOKS     (tskIDLE_PRIORITY + 3)
#define INIT_TASK_PRIORITY		(tskIDLE_PRIORITY + configMAX_PRIORITIES - 1) // max priority

/* Global Variables */

/* bounds of the init stack courtesy of the linker script */
extern char	_init_stack_top, _init_stack_end;

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);

static void mainTask(void *parameters);

/**
* OpenPilot Main function:
*
* Initialize PiOS<BR>
* Create the "System" task (SystemModInitializein Modules/System/systemmod.c) <BR>
* Start FreeRTOS Scheduler (vTaskStartScheduler) (Now handled by caller)
* If something goes wrong, blink LED1 and LED2 every 100ms
*
*/
int main()
{
	int result;
	xTaskHandle mainTaskHandle;
	
	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();
	
	/* initialise the heap */
	vPortInitialiseBlocks();
	
	/*
	 * Swap to the interrupt stack so that when xTaskGenericCreate clears the init stack
	 * we aren't clobbered.
	 */
	 Stack_Change();
	 
	/* create the init task */
	result = xTaskGenericCreate(mainTask,
								(const signed char *)"main",
								(&_init_stack_top - &_init_stack_end) / sizeof(portSTACK_TYPE),
								NULL,
								INIT_TASK_PRIORITY,
								&mainTaskHandle,
								(void *)&_init_stack_end,
								NULL);
	PIOS_Assert(result == pdPASS);
	
	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	/* If all is well we will never reach here as the scheduler will now be running. */

	/* Do some indication to user that something bad just happened */
	PIOS_LED_Off(LED1);
	for(;;) {
		PIOS_LED_Toggle(LED1);
		PIOS_DELAY_WaitmS(100);
	};

    return 0;
}
	

static void mainTask(void *parameters)
{
	/* Architecture dependant Hardware and
	 * core subsystem initialisation
	 * (see pios_board.c for your arch)
	 * */
	PIOS_Board_Init();

	/* Initialize modules */
	MODULE_INITIALISE_ALL

	/* terminate this task & free its resources */
	vTaskDelete(NULL);
}

/**
 * @}
 * @}
 */

