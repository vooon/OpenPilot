/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_RCTX RCTX Input Functions
 * @brief Code to measure RCTX input and seperate into channels
 * @{
 *
 * @file       pios_rctx.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      RCTX Input functions (STM32 dependent)
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

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <libudev.h>
#include <locale.h>

#include <sys/time.h>
#include <time.h>

/* Project Includes */
#include "pios.h"
#include "pios_rctx_priv.h"

#if defined(PIOS_INCLUDE_RCTX)

#ifdef RCTX_DEBUG
#define rctx_printf(data, arg...) printf(data, ## arg)
#define rctx_puts(data) puts(data)
#else
#define rctx_printf(data, arg...)
#define rctx_puts(data)
#endif

/* function call supported for a particular USB adapter */
static int PIOS_RCTX_Phoenix_Usb_Adapter_Buf_Set(void);
static int PIOS_RCTX_Phoenix_Usb_Adapter_Status_Get(void);

/* Local Variables */
static int rctx_fd = 0;
static uint32_t current_adapter_id = 0;
static uint32_t CaptureValue[RCTX_PIOS_NUM_INPUTS];
static char buf[256];
static int32_t rctx_trasmitter_connected = 0;

/* list of all supported adapters */
struct pios_rctx_device pios_rctx_device_supported[] =
	{  /* vendor id , product id, periode_ms, callback input, callback status */
		/* PhoenixRC USB Adapter from Horizon */
		{ 1781 , 898, 200, PIOS_RCTX_Phoenix_Usb_Adapter_Buf_Set, PIOS_RCTX_Phoenix_Usb_Adapter_Status_Get },
		/* Add new r/c transmitter USB adapter here */
		{ 0, 0, 0, NULL, NULL }
	};

/* size of the list of supported adapters */
#define pios_rctx_device_supported_nitems (sizeof(pios_rctx_device_supported)/sizeof(pios_rctx_device_supported[0]))

/**
* Look for the hidraw devive the r/c Tx is plugged to
* \param[in] device node string
* \param[in] indice in supported device table
* \output pios_rctx_err errors
* \output 0 successful
*/
static int32_t PIOS_RCTX_probe_tx(char *device_node_path_string, uint32_t *dev_tab_i)
{
	int32_t rc = pios_rctx_transmitter_not_found;
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;

	// Create the udev object
	udev = udev_new();
	if (!udev) {
		printf("RCTX: Can't create udev\n");
		return pios_rctx_udev_create_err;
	}

	// Create a list of the devices in the 'hidraw' subsystem
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	// For each item enumerated, get the device's path in /sys
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;

		// Get the filename of the /sys entry for the device
		path = udev_list_entry_get_name(dev_list_entry);

		// Create a udev_device object (dev) representing it
		dev = udev_device_new_from_syspath(udev, path);

		// Get the path to the device node itself in /dev
		strcpy(device_node_path_string, udev_device_get_devnode(dev));

		// Get the parent device to get device's USB infos
		dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
		if (!dev)
		{
			printf("RCTX: Unable to find parent usb device.");
			return pios_rctx_udev_parent_err;
		}

		// Get info from the /sys entry files
		int32_t vendor_id = atoi(udev_device_get_sysattr_value(dev,"idVendor"));
		int32_t product_id = atoi(udev_device_get_sysattr_value(dev, "idProduct"));
		int32_t i;

		for (i=0; i<pios_rctx_device_supported_nitems; i++)
		{
			if ((pios_rctx_device_supported[i].vendor_id == vendor_id) &&
				(pios_rctx_device_supported[i].product_id == product_id))
			{
				printf("RCTX: Found transmitter:\n");
				printf("RCTX:   VID/PID: 0x%d 0x%d\n", vendor_id, product_id );

				printf("RCTX:   %s\nRCTX:   %s\n",
					udev_device_get_sysattr_value(dev,"manufacturer"),
					udev_device_get_sysattr_value(dev,"product"));
				printf("RCTX:   Connected to device Node: %s\n", device_node_path_string);
				udev_device_unref(dev);
				*dev_tab_i = i;
				rc = 0;
				goto leave;
			}
		}
		udev_device_unref(dev);
	}

leave:
	// Free the enumerator object
	udev_enumerate_unref(enumerate);

	udev_unref(udev);

	return rc;
}

/**
* Get the transmitter status
* \output -1 transmitter disconnected
* \output 0 transmitter connected
*/
static int PIOS_RCTX_Phoenix_Usb_Adapter_Status_Get(void)
{
	return rctx_trasmitter_connected;
}

/**
* Set the value of the channels
* \output pios_rctx_err errors
* \output 0 successful
*/
static int PIOS_RCTX_Phoenix_Usb_Adapter_Buf_Set(void)
{
	int i = 0;
	int res;
	int rc = 0;
	static uint32_t rctx_lost_connection_counter = 0;

	// Send a Report to the Device
	buf[0] = 0x0;
	buf[1] = 0x0;
	res = write(rctx_fd, buf, 2);
	if (res < 0) {
			perror("RCTX: write");
			rc = pios_rctx_wressource_unavailable;
	} else {

		// Get a report from the device
		res = read(rctx_fd, buf, RCTX_PIOS_NUM_INPUTS);
		if (res < 0) {
			perror("RCTX: read");
			rc = pios_rctx_rressource_unavailable;
		} else {

			// reset lost connection counter since we can read the handle
			rctx_lost_connection_counter = 0;
			rctx_printf("RCTX: read %d bytes from transmitter: ", res);
			for (i = 0; i < res; i++) {
				rctx_printf("%hhx ", buf[i]);
				CaptureValue[i] = 0xFF & buf[i];
			}
			rctx_printf("\n");

			// check for sign of connection lost other than usb disconnected
			// this is transmitter/adapter specific.
			if (buf[0] == 0xFF) {
				// transmitter unplugged from the adapter
				rc = pios_rctx_transmitter_unplugged_from_adapter;
			}
		}
	}

	// process all connection or technical difficulties here
	// don't declare connection lost right away since it could happen from time to time.
	// The impact is that during this failing period the new input is not update
	// and will be similar to previous input
	// this could be dangerous too! (Depending on RCTX_LOST_CONNECTION_TIME_MS_MAX)
	if (rc) {

		// wait before declaring connection lost
		if (rctx_lost_connection_counter++ >
			(RCTX_LOST_CONNECTION_TIME_MS_MAX / pios_rctx_device_supported[current_adapter_id].periode_ms)) {

			// give up and declare the transmitter disconnected
			rctx_trasmitter_connected = -1;
		}
	}

	return rc;
}

/**
* read periodically the value of input transmitter
* \param[in] pointer to file descriptor and indice to supported device
*/
static void TaskRCTX(void *pvParameters)
{
	uint16_t second_delay_ctr = 0;
	portTickType xLastExecutionTime;
	int32_t rc;

	// Initialise the xLastExecutionTime variable on task entry
	xLastExecutionTime = xTaskGetTickCount();

	for(;;) {

		vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

		// Periodically read the r/c transmitter input
		// the periode here depend on transmitter/USB adapter communication speed
		// increasing the speed may not be necessary and may endup taking more cpu cycle than needed
		// reducing will prevent io access to kernel usb stack and file system, giving more cycle to GCS and x-plane which they can use
		if(++second_delay_ctr >= pios_rctx_device_supported[current_adapter_id].periode_ms) {

			// test the r/c transmitter USB connection
			if (pios_rctx_device_supported[current_adapter_id].TX_Status_Get() == 0) {

				// get the transmitter input
				rc = pios_rctx_device_supported[current_adapter_id].TX_Input_Get();

				// handle special cases that are not connection lost or technical defect related
				if (rc == pios_rctx_trainer_enabled) {
					// User push the trainer button should we process this as an emergency?
					// should we report a connection failure still so that GCS takes control?
					// TODO: implement use case for trainer switch pressed
				}

			} else {

				// TODO: recover from lost of connection
				break;
			}

			second_delay_ctr = 0;
		}
	}

	printf("RCTX: Error: The connection with the transmitter has been lost.\n");
	printf("RCTX: Recovery not supported yet.\n");
	printf("RCTX: please restart OpenPilot to regain connection to the transmitter again.\n");
	close(rctx_fd);
	while(1);
}

/**
* get the transmitter connection status (i.e. used to swap between GCS and real transmitter)
* \output -1 transmitter not connected
* \output >0 transmitter connected
*/
int32_t PIOS_RCTX_Transmitter_Status_Get(void)
{
	return pios_rctx_device_supported[current_adapter_id].TX_Status_Get();
}

/**
* Initialises the r/c HID interface
*/
void PIOS_RCTX_Init(void)
{
	int32_t res, i = 0;
	uint32_t dev_tab_i = 0;
	char hid_raw_string[RCTX_HIDRAW_STRING_MAX];

	// grab the hidraw device
	memset(hid_raw_string, 0, sizeof(hid_raw_string));
	res = PIOS_RCTX_probe_tx(hid_raw_string, &dev_tab_i);
	if (res == 0) {
		rctx_fd = open(hid_raw_string, O_RDWR|O_NONBLOCK);

		if (rctx_fd > 0) {
			// clear temporary buffer used to grab input from transmitter
			memset(buf, 0x0, sizeof(buf));

			// populate current adapter discovered
			current_adapter_id = dev_tab_i;

			// Create task
			xTaskCreate(TaskRCTX, (signed portCHAR *)"RCTX", configMINIMAL_STACK_SIZE , NULL, RCTX_TASK_PRIORITY, NULL);

		} else {
			perror("RCTX: Unable to open device");
			if (errno == 13) {
				printf("RCTX: make sure the read/write permissions are set (i.e. sudo chmod a+rw %s)\n", hid_raw_string);
				printf("RCTX: You also can create a udev rule for this interface because\n");
				printf("RCTX: each time the transmitter will be plugged, the permission will need to be updated.\n");
			}

		}
	}

	for (i = 0; i < RCTX_PIOS_NUM_INPUTS; i++) {
		CaptureValue[i] = 0;
	}
}

/**
* Get the value of an input channel
* \param[in] Channel Number of the channel desired
* \output -1 Channel not available
* \output >0 Channel value
*/
int32_t PIOS_RCTX_Get(int8_t Channel)
{
	// Return error if channel not available
	if (Channel >= RCTX_PIOS_NUM_INPUTS) {
		return -1;
	}

#ifdef RCTX_DEBUG

	int total_usecs;
	gettimeofday(&end_time, 0);
	total_usecs = (end_time.tv_sec-start_time.tv_sec) * 1000000 +
				  (end_time.tv_usec-start_time.tv_usec);

	printf("RCTX: Total time was %d uSec.\n", total_usecs);
	gettimeofday(&start_time, 0);

	rctx_printf ("RCTX: Channel #%d: %d (0x%hhx)\n",
					Channel,
					CaptureValue[Channel],
					CaptureValue[Channel]);
#endif

	return ((rctx_trasmitter_connected == 0 ) ? CaptureValue[Channel] : -1);
}


#endif

/**
  * @}
  * @}
  */
