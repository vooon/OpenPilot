
/**
 ******************************************************************************
 *
 * @file       main.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Main modem functions
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


// *****************************************************************************

//#define USE_WATCHDOG                    // comment this out if you don't want to use the watchdog

// *****************************************************************************
// OpenPilot Includes

#include <string.h>

#include "oposd.h"
#include "watchdog.h"
#include "videoconf.h"
#include "linedriver.h"
#include "textdriver.h"
#include "graphicdriver.h"

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);

// *****************************************************************************
// Global Variables

// *****************************************************************************
// Local Variables

#if defined(USE_WATCHDOG)
  volatile uint16_t     watchdog_timer;
  uint16_t              watchdog_delay;
#endif

// *****************************************************************************

#if defined(USE_WATCHDOG)

  void processWatchdog(void)
  {
      // random32 = UpdateCRC32(random32, IWDG->SR);

      if (watchdog_timer < watchdog_delay)
          return;

      // the watchdog needs resetting

      watchdog_timer = 0;

      watchdog_Clear();
  }

  void enableWatchdog(void)
  {	// enable a watchdog
      watchdog_timer = 0;
      watchdog_delay = watchdog_Init(1000);	// 1 second watchdog timeout
  }

#endif

// *****************************************************************************

void sequenceLEDs(void)
{
    for (int i = 0; i < 2; i++)
    {
        USB_LED_ON;
        PIOS_DELAY_WaitmS(100);
        USB_LED_OFF;
        PIOS_DELAY_WaitmS(100);

         #if defined(USE_WATCHDOG)
            processWatchdog();		// process the watchdog
        #endif
    }
}

// *****************************************************************************
// find out what caused our reset and act on it

void processReset(void)
{
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
    {	// Independant Watchdog Reset

        #if defined(PIOS_COM_DEBUG)
            DEBUG_PRINTF("\r\nINDEPENDANT WATCHDOG CAUSED A RESET\r\n");
        #endif

        // all led's ON
        USB_LED_ON;


        PIOS_DELAY_WaitmS(500);	// delay a bit

        // all led's OFF
        USB_LED_OFF;


    }
/*
	if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET)
	{	// Window Watchdog Reset

		DEBUG_PRINTF("\r\nWINDOW WATCHDOG CAUSED A REBOOT\r\n");

		// all led's ON
		USB_LED_ON;
		LINK_LED_ON;
		RX_LED_ON;
		TX_LED_ON;

		PIOS_DELAY_WaitmS(500);	// delay a bit

		// all led's OFF
		USB_LED_OFF;
		LINK_LED_OFF;
		RX_LED_OFF;
		TX_LED_OFF;
	}
*/
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
    {	// Power-On Reset
        #if defined(PIOS_COM_DEBUG)
            DEBUG_PRINTF("\r\nPOWER-ON-RESET\r\n");
        #endif
    }

    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET)
    {	// Software Reset
        #if defined(PIOS_COM_DEBUG)
            DEBUG_PRINTF("\r\nSOFTWARE RESET\r\n");
        #endif
    }

    if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET)
    {	// Low-Power Reset
        #if defined(PIOS_COM_DEBUG)
            DEBUG_PRINTF("\r\nLOW POWER RESET\r\n");
        #endif
    }

    if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
    {	// Pin Reset
        #if defined(PIOS_COM_DEBUG)
            DEBUG_PRINTF("\r\nPIN RESET\r\n");
        #endif
    }

    // Clear reset flags
    RCC_ClearFlag();
}

static void updateOnceEveryFrame() {
#ifdef TEXT_ENABLED
	clearText();
	for (uint8_t i = 0; i < TEXT_LINES; ++i) {
	  updateText(i);
	}
#endif //TEXT_ENABLED

#ifdef GRAPICSENABLED
	clearGraphics();
	updateGrapics();
#endif //GRAPICSENABLED
}


int main()
{
    // *************
    // init various variables
    // *************

    PIOS_Board_Init();

    initLine();
    sequenceLEDs();

    // turn all the leds off
    USB_LED_OFF;


	#if defined(PIOS_COM_DEBUG)
    	DEBUG_PRINTF("\r\n");
	#endif

    // *************
    // Main executive loop


    for (;;)
    {
#ifdef TEXT_ENABLED
    if (update == 2) {
		  update = 0;
#ifdef TEXT_INVERTED_ENABLED
	    clearTextInverted();
#endif //TEXT_INVERTED_ENABLED
	    updateTextPixmap(activeTextId);
	  }
		else if (update == 1) {
#else
			if (update == 1) {
#endif //TEXTENABLED
				update = 0;
				updateOnceEveryFrame();
			}

		#if defined(USE_WATCHDOG)
			  processWatchdog();		// process the watchdog
		#endif
		}

    // *************
    // we should never arrive here

    // disable all interrupts
    PIOS_IRQ_Disable();

    // turn off all leds
    USB_LED_OFF;


    PIOS_SYS_Reset();

    while (1);

    return 0;
}


// *****************************************************************************
