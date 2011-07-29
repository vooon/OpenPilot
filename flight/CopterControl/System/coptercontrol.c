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
#define PRIORITY_TASK_HOOKS             (tskIDLE_PRIORITY + 3)

/* Global Variables */

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);

/**
* OpenPilot Main function:
*
* Initialize PiOS<BR>
* Create the "System" task (SystemModInitializein Modules/System/systemmod.c) <BR>
* Start FreeRTOS Scheduler (vTaskStartScheduler) (Now handled by caller)
* If something goes wrong, blink LED1 and LED2 every 100ms
*
*/
/* test functions */
void blink(int led, int times)
{
	for(int i=0; i<times; i++)
	{
		PIOS_LED_Toggle(led);
		PIOS_DELAY_WaitmS(250);
		PIOS_LED_Toggle(led);
		PIOS_DELAY_WaitmS(250);
	}
}


#if defined (PIOS_INCLUDE_HMC5883)
void test_mag()
{
	int16_t mags[3];
	if(PIOS_HMC5883_Test())	{
		PIOS_HMC5883_ReadMag(mags);
		blink(LED1, 2);
	} else {
		blink(LED1, 4);
	}
}
#endif

#if defined (PIOS_INCLUDE_BMP085)
void test_pressure()
{
	if(PIOS_BMP085_Test())
		blink(LED1, 3);
	else
		blink(LED1, 6);
}
#endif

int main()
{
	/* NOTE: Do NOT modify the following start-up sequence */
	/* Any new initialization functions should be added in OpenPilotInit() */

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();

	/* Architecture dependant Hardware and
	 * core subsystem initialisation
	 * (see pios_board.c for your arch)
	 * */
	PIOS_Board_Init();
	for(;;) {
	test_mag();
	PIOS_DELAY_WaitmS(100);
	test_pressure();
	PIOS_DELAY_WaitmS(100);
}
	/* Initialize modules */
	MODULE_INITIALISE_ALL

	/* swap the stack to use the IRQ stack */
	Stack_Change();

	/* Start the FreeRTOS scheduler which should never returns.*/
	vTaskStartScheduler();

	/* If all is well we will never reach here as the scheduler will now be running. */

	/* Do some indication to user that something bad just happened */
	PIOS_LED_Off(LED1); \
	for(;;) { \
		PIOS_LED_Toggle(LED1); \
		PIOS_DELAY_WaitmS(100); \
	};

    return 0;
}

/**
 * @}
 * @}
 */

