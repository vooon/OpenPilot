/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup ???
 * @brief Communicate with generic I2C sensor and update @ref GenericI2C "GenericI2C UAV Object"
 * @{ 
 *
 * @file       generici2csensor.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Reads generic i2c sensor
 *
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

/**
 * Output object: generici2csensor
 *
 * This module will periodically update the value of the mcp3424sensor UAVobject.
 *
 */

#include "openpilot.h"
#include "generici2csensor.h"	// UAVobject that will be updated by the module
#include "pios_i2c.h"

// Private constants
#define STACK_SIZE_BYTES 600
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)
#define UPDATE_PERIOD 500
//static double_t vPerC = 0.0000403; //volts per celcius for K-type thermocouple
//static double_t MCP3424_REFVOLTAGE = 2.048; //internal reference voltage for MCP3424 IC

// Private types
#define MCP9804_I2C_ADDRESS 0x1F //Cold junction temperature sensor
#define MCP3424_I2C_ADDRESS 0x68 //Four channel ADC sensor

// Private variables
static xTaskHandle taskHandle;

// down sampling variables
#define MCP3424_ds_size    4

// Private functions
static void GenericI2CSensorTask(void *parameters);

enum I2CPASS_NAMES {
	FIRST_PASS, LOOP_PASS, UAVOBJECT_UPDATE_PASS
};

enum I2CSTEP_NAMES {
	WAIT_STATE_0, WAIT_STATE_1, WAIT_STATE_2, WAIT_STATE_3,
	CONFIG_REG_0, CONFIG_REG_1, CONFIG_REG_2, CONFIG_REG_3,
	WRITE_REG_0, WRITE_REG_1,
	READ_REG_0, READ_REG_1,
	UAVOBJECT_DATA_0, UAVOBJECT_DATA_1, UAVOBJECT_DATA_2, UAVOBJECT_DATA_3,
	UAVOBJECT_UPDATE
	
};

/**
* Start the module, called on startup
*/
int32_t GenericI2CSensorModuleStart()
{
	// Start main task
	xTaskCreate(GenericI2CSensorTask, (signed char *)"GenericI2CSensor", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_GENERICI2CSENSOR, taskHandle);
	return 0;
}

/**
* Initialise the module, called on startup
*/
int32_t GenericI2CSensorModuleInitialize()
{
	GenericI2CSensorInitialize(); //Initialise the UAVObject used for transferring data to GCS

	return 0;
}

MODULE_INITCALL(GenericI2CSensorModuleInitialize, GenericI2CSensorModuleStart)


/*
 * Read arbitrary registers
 *
 * Returns true if successful and false if not.
 */
static bool ReadGenericRegisters(uint8_t i2c_address, uint8_t numRegisters, uint8_t* pBuffer)
{
	
	const struct pios_i2c_txn txn_list_1[] = {
		{
			.addr = i2c_address,
			.rw = PIOS_I2C_TXN_READ,
			.len = numRegisters, 
			.buf = pBuffer,
		},
	};
	
	//Read data bytes
	if(PIOS_I2C_Transfer(PIOS_I2C_MAIN_ADAPTER, txn_list_1, NELEMENTS(txn_list_1))) {
		return true;
	}
	else
		return false;
}

/*
*/
static bool WriteGenericRegisters(uint8_t i2c_address, uint8_t numRegisters, uint8_t* pBuffer)
{
	
	//Set GenericI2CSensor config register via i2c
	const struct pios_i2c_txn txn_list_1[] = {
		{
			.addr = i2c_address,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = numRegisters,
			.buf = pBuffer,
		},
	};
	

	//Write data bytes
	if(PIOS_I2C_Transfer(PIOS_I2C_MAIN_ADAPTER, txn_list_1, NELEMENTS(txn_list_1))) {		
		return true;
	}
	else
		return false;
}

uint8_t jumpTableIndex=0;
static void advance_to_next_I2C_step(enum I2CSTEP_NAMES *p_i2cStep, enum I2CPASS_NAMES *p_pass, uint8_t *jumpTable) {
	
	if (jumpTable[jumpTableIndex]==0xFF) { //Special value which causes jump table to loop
		jumpTableIndex=0;
	}
	else if (jumpTable[jumpTableIndex]==0xFE) { //Special value which causes jump table to change type of pass
		*p_pass=jumpTable[++jumpTableIndex];
		*p_i2cStep=jumpTable[++jumpTableIndex];
	}
	else {
		*p_i2cStep=jumpTable[++jumpTableIndex];
	}
}


/**
 * Module thread, should not return.
 */
static void GenericI2CSensorTask(void *parameters)
{

	portTickType lastSysTime;

	//UAVObject data structure
	GenericI2CSensorData d1;

	// Main task loop
	lastSysTime = xTaskGetTickCount();

	uint8_t I2C_JUMPTABLE[32]; //Initialize jump table. AT THIS MOMENT, A JUMP TABLE MUST BE SMALLER THAN 32 BYTES

	
	//MOST OF THESE VALUES NEED TO BE ABSTRACT AND READ IN FROM FLASH
	enum I2CSTEP_NAMES i2cStep=WAIT_STATE_0;
	enum I2CPASS_NAMES pass=FIRST_PASS;
	uint32_t tmpLongBuffer = 0;  //buffer to store return data from sensor //THIS IS A FIXED SIZE, SINCE IT HAS TO FIT INTO THE UAVOBJECT
	portTickType xDelay;
	portTickType xLastWakeTime=xTaskGetTickCount();
	
	char I2C_ADDRESS=0;
	int NUM_REGISTERS_WRITE_CONFIG_REG_0=0, NUM_REGISTERS_WRITE_CONFIG_REG_1=0, NUM_REGISTERS_WRITE_CONFIG_REG_2=0, NUM_REGISTERS_WRITE_CONFIG_REG_3=0;
	int NUM_REGISTERS_WRITE_REG_0=0, NUM_REGISTERS_WRITE_REG_1=0;
	int NUM_REGISTERS_READ_REG_0=0, NUM_REGISTERS_READ_REG_1=0;
	int WAIT_DELAY_ms_0=0, WAIT_DELAY_ms_1=0, WAIT_DELAY_ms_2=0, WAIT_DELAY_ms_3=0;
	uint32_t VALUE_WRITE_CONFIG_REG_0=0, VALUE_WRITE_CONFIG_REG_1=0, VALUE_WRITE_CONFIG_REG_2=0, VALUE_WRITE_CONFIG_REG_3=0;
	uint32_t VALUE_WRITE_REG_0=0, VALUE_WRITE_REG_1=0;
	
	/*
	 *Temporary values for SRF08 (BEGIN)
	 * V V V V V V V V V V V V V V V V
	 */
	I2C_ADDRESS=0xE0;
	WAIT_DELAY_ms_0=65;
	VALUE_WRITE_REG_0=(0x00<<24) & (0x82<<16);
	NUM_REGISTERS_WRITE_REG_0=2;
	VALUE_WRITE_REG_1=0x02;
	NUM_REGISTERS_WRITE_REG_1=1;
	NUM_REGISTERS_READ_REG_0=6;
	
	I2C_JUMPTABLE[0]=0xFE;
	I2C_JUMPTABLE[1]=LOOP_PASS;
	I2C_JUMPTABLE[2]=WRITE_REG_0;
	I2C_JUMPTABLE[3]=WAIT_STATE_0;
	I2C_JUMPTABLE[4]=WRITE_REG_1;
	I2C_JUMPTABLE[5]=READ_REG_0;
	I2C_JUMPTABLE[6]=0xFE;
	I2C_JUMPTABLE[7]=UAVOBJECT_UPDATE_PASS;
	I2C_JUMPTABLE[8]=UAVOBJECT_DATA_0;
	I2C_JUMPTABLE[9]=UAVOBJECT_UPDATE;
	I2C_JUMPTABLE[10]=0xFF;
	/* ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ 
	 *Temporary values for SRF08 (END)
	 */
	
	while(1) {

		//Generic I2C addressing
		if(	== WAIT_STATE_0){
				// Block for WAIT_DELAY_ms_0 [ms].
				xDelay = WAIT_DELAY_ms_0 / portTICK_RATE_MS;
				vTaskDelayUntil(&xLastWakeTime, xDelay);
		}
		else if( i2cStep == WAIT_STATE_1 ){
			// Block for WAIT_DELAY_ms_1 [ms].
			xDelay = WAIT_DELAY_ms_1 / portTICK_RATE_MS;
			vTaskDelayUntil(&xLastWakeTime, xDelay);
		}
		else if( i2cStep == WAIT_STATE_2 ){
			// Block for WAIT_DELAY_ms_2 [ms].
			xDelay = WAIT_DELAY_ms_2 / portTICK_RATE_MS;
			vTaskDelayUntil(&xLastWakeTime, xDelay);
		}
		else if ( i2cStep == WAIT_STATE_3 ){
			// Block for WAIT_DELAY_ms_3 [ms].
			xDelay = WAIT_DELAY_ms_3 / portTICK_RATE_MS;
			vTaskDelayUntil(&xLastWakeTime, xDelay);
		}
		else if (pass==FIRST_PASS){
			if ( i2cStep == CONFIG_REG_0 ){
				WriteGenericRegisters(I2C_ADDRESS, NUM_REGISTERS_WRITE_CONFIG_REG_0, (uint8_t *) &VALUE_WRITE_CONFIG_REG_0);
			}
			else if ( i2cStep == CONFIG_REG_1 ){
				WriteGenericRegisters(I2C_ADDRESS, NUM_REGISTERS_WRITE_CONFIG_REG_1, (uint8_t *) &VALUE_WRITE_CONFIG_REG_1);
			}
			else if ( i2cStep == CONFIG_REG_2 ){
				WriteGenericRegisters(I2C_ADDRESS, NUM_REGISTERS_WRITE_CONFIG_REG_2, (uint8_t *) &VALUE_WRITE_CONFIG_REG_2);
			}
			else if ( i2cStep == CONFIG_REG_3 ){
				WriteGenericRegisters(I2C_ADDRESS, NUM_REGISTERS_WRITE_CONFIG_REG_3, (uint8_t *) &VALUE_WRITE_CONFIG_REG_3);
			}
			
			xLastWakeTime=xTaskGetTickCount();
			
		}
		else if (pass==LOOP_PASS){
			if ( i2cStep == WRITE_REG_0 ){
				WriteGenericRegisters(I2C_ADDRESS, NUM_REGISTERS_WRITE_REG_0, (uint8_t *) &VALUE_WRITE_REG_0);
			}
			else if ( i2cStep == WRITE_REG_1 ){
				WriteGenericRegisters(I2C_ADDRESS, NUM_REGISTERS_WRITE_REG_1, (uint8_t *) &VALUE_WRITE_REG_1);
			}
			else if ( i2cStep == READ_REG_0 ){
				ReadGenericRegisters(I2C_ADDRESS, NUM_REGISTERS_READ_REG_0, (uint8_t *) tmpLongBuffer);
//				decipherGenericI2CSensorresponse((uint8_t *) tmpLongBuffer, 0, 0, 0, 0);
			}
			else if ( i2cStep == READ_REG_1 ){
				ReadGenericRegisters(I2C_ADDRESS, NUM_REGISTERS_READ_REG_1, (uint8_t *) tmpLongBuffer);
//				decipherGenericI2CSensorresponse((uint8_t *) tmpLongBuffer, 0, 0, 0, 0);
			}
			
			xLastWakeTime=xTaskGetTickCount();

		}
		else if (pass==UAVOBJECT_UPDATE_PASS){
			if (i2cStep == UAVOBJECT_DATA_0){
				d1.SensorOutput1= tmpLongBuffer;
//				memcpy(&(d1.SensorOutput1), tmpLongBuffer, 4);
			}
			else if (i2cStep == UAVOBJECT_DATA_1){
				d1.SensorOutput2= tmpLongBuffer;
//				memcpy(&(d1.SensorOutput2), tmpLongBuffer, 4);
			}
			else if (i2cStep == UAVOBJECT_DATA_2){
				d1.SensorOutput3= tmpLongBuffer;
//				memcpy(&(d1.SensorOutput3), tmpLongBuffer, 4);
			}
			else if (i2cStep == UAVOBJECT_DATA_3){
				d1.SensorOutput4= tmpLongBuffer;
//				memcpy(&(d1.SensorOutput4), tmpLongBuffer, 4);
			}
			else if (i2cStep == UAVOBJECT_UPDATE){
				/*
				 * Update UAVObject data
				 */
				GenericI2CSensorSet(&d1);
			}			
			
		}
		
		advance_to_next_I2C_step(&i2cStep, &pass, I2C_JUMPTABLE);
		
		

	}
}



/**
  * @}
 * @}
 */
