/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup Generic I2C Virtual Machine Module
 * @brief Generic Programmable I2C Virtual Machine Module
 * @{ 
 *
 * @file       
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Generic Programmable I2C Virtual Machine Module
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

#include <pios.h>
#include "generici2csensor.h"
#include "generici2csensorsettings.h"
#include "../../../shared/lib/gen_i2c_vm.h" //<------ICK, UGLY

#define I2C_VM_RAM_SIZE 8
uint32_t program[GENERICI2CSENSORSETTINGS_PROGRAM_NUMELEM]; //This is the program vector

struct i2c_vm_regs {
	bool     halted;
	bool     fault;
	uint8_t  pc;
	uint32_t ctr;

	uint8_t i2c_dev_addr;
	uint8_t ram[I2C_VM_RAM_SIZE];

	GenericI2CSensorData uavo;

	int32_t  r0;
	int32_t  r1;
	int32_t  r2;
	int32_t  r3;
	float    f0;
	float    f1;
	float    f2;
	float    f3;
};

/* Halt the virtual machine */
static bool i2c_vm_halt (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	vm_state->halted = true;
	return (true);
}

/* Virtual machine no operation instruction */
static bool i2c_vm_nop (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	vm_state->pc++;
	return (true);
}

/* Set virtual machine counter */
static bool i2c_vm_set_ctr (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	vm_state->ctr = op1;
	vm_state->pc++;
	return (true);
}

/* Store virtual machine data in RAM  */
static bool i2c_vm_store (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	if (op2 >= sizeof(vm_state->ram)) {
		return (false);
	}

	vm_state->ram[op2] = op1;
	vm_state->pc++;
	return (true);
}


/* Load data into virtual machine registers */
static bool i2c_vm_load (struct i2c_vm_regs * vm_state, uint32_t val, uint8_t op3)
{
	switch (op3) {
	case VM_R0:
		vm_state->r0 = val;
		break;
	case VM_R1:
		vm_state->r1 = val;
		break;
	case VM_F0:
		vm_state->f0 = val;
		break;
	default:
		return (false);
	}

	vm_state->pc++;
	return (true);
}

/* Load register information in Big Endian format */
static bool i2c_vm_load_be (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	if ((op2 < 1) || (op2 > 4)) {
		return (false);
	}

	uint32_t val=0; //Important that this be =0
	memcpy (&val, &(vm_state->ram[op1]), op2);

	/* Handle byte swapping */
	val = (((val & 0xFF000000) >> 24) |
		((val & 0x00FF0000) >> 8) |
		((val & 0x0000FF00) << 8) |
		((val & 0x000000FF) << 24));

	return (i2c_vm_load (vm_state, val, op3));
}

/* Load register information in Little Endian format */
static bool i2c_vm_load_le (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	if ((op2 < 1) || (op2 > 4)) {
		return (false);
	}

	uint32_t val=0; //Important that this be =0
	memcpy (&val, &(vm_state->ram[op1]), op2);

	return (i2c_vm_load (vm_state, val, op3));
}

/* Counter decrements to zero */
static bool i2c_vm_dec_ctr (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	if (vm_state->ctr > 0) {
		vm_state->ctr--;
	}
	vm_state->pc++;
	return (true);
}

/* Virtual machine jump operation */
static bool i2c_vm_jump (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	vm_state->pc += op1;
	return (true);
}

/* Virtual machine Branch If Not Zero operation */
static bool i2c_vm_bnz (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	if (vm_state->ctr) {
		return (i2c_vm_jump(vm_state, op1, op2, op3));
	} else {
		vm_state->pc++;
	}
	return (true);
}

/* Set I2C device address in virtual machine */
static bool i2c_vm_set_dev_addr (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	vm_state->i2c_dev_addr = op1;
	vm_state->pc++;
	return (true);
}

/* Read I2C data into virtual machine RAM */
static bool i2c_vm_read (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	/* Make sure our read fits in our buffer */
	if (op1 > sizeof(vm_state->ram)) {
		return false;
	}

	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = vm_state->i2c_dev_addr,
			.rw = PIOS_I2C_TXN_READ,
			.len = op1,
			.buf = vm_state->ram,
		},
	};

	vm_state->pc++;
	return (PIOS_I2C_Transfer(PIOS_I2C_MAIN_ADAPTER, txn_list, NELEMENTS(txn_list)));
}

/* Write I2C data from virtual machine RAM */
static bool i2c_vm_write (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	if (op1 > sizeof(vm_state->ram)) {
		return (false);
	}

	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = vm_state->i2c_dev_addr,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = op1,
			.buf = vm_state->ram,
		},
	};

	vm_state->pc++;
	return (PIOS_I2C_Transfer(PIOS_I2C_MAIN_ADAPTER, txn_list, NELEMENTS(txn_list)));
}

/* Send UAVObject from virtual machine registers */
static bool i2c_vm_send_uavo (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	/* Copy our exportable state into the uavo */
	vm_state->uavo.r0 = vm_state->r0;
	vm_state->uavo.r1 = vm_state->r1;
	vm_state->uavo.f0 = vm_state->f0;

	GenericI2CSensorSet(&vm_state->uavo);

	vm_state->pc++;
	return (true);
}

/* Make virtual machine wait op1 [ms] */
static bool i2c_vm_delay (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3)
{
	vTaskDelay(op1);
	vm_state->pc++;
	return (true);
}

/* Reboot virtual machine */
static bool i2c_vm_reboot (struct i2c_vm_regs * vm_state)
{
	vm_state->halted = false;
	vm_state->fault  = false;
	vm_state->pc     = 0;
	vm_state->ctr    = 0;

	vm_state->r0     = 0;
	vm_state->r1     = 0;
	vm_state->r2     = 0;
	vm_state->r3     = 0;
	vm_state->f0     = (float) 0.0;
	vm_state->f1     = (float) 0.0;
	vm_state->f2     = (float) 0.0;
	vm_state->f3     = (float) 0.0;

	vm_state->i2c_dev_addr = 0;
	memset(vm_state->ram, 0, sizeof(vm_state->ram));

	return true;
}


typedef bool (*i2c_vm_inst_handler) (struct i2c_vm_regs * vm_state, uint8_t op1, uint8_t op2, uint8_t op3);

const i2c_vm_inst_handler i2c_vm_handlers[] = {
	[I2C_VM_OP_HALT]         = i2c_vm_halt,         //Halt
	[I2C_VM_OP_NOP]          = i2c_vm_nop,          //No operation
	[I2C_VM_OP_STORE]        = i2c_vm_store,        //Store value
	[I2C_VM_OP_LOAD_BE]      = i2c_vm_load_be,      //Load big endian
	[I2C_VM_OP_LOAD_LE]      = i2c_vm_load_le,      //Load little endian
	[I2C_VM_OP_SET_CTR]      = i2c_vm_set_ctr,      //Set counter
	[I2C_VM_OP_DEC_CTR]      = i2c_vm_dec_ctr,      //Decrement counter
	[I2C_VM_OP_BNZ]          = i2c_vm_bnz,          //Branch if not zero
	[I2C_VM_OP_JUMP]         = i2c_vm_jump,         //Jump
	[I2C_VM_OP_SET_DEV_ADDR] = i2c_vm_set_dev_addr, //Set I2C device address
	[I2C_VM_OP_READ]         = i2c_vm_read,         //Read from I2C bus
	[I2C_VM_OP_WRITE]        = i2c_vm_write,        //Write to I2C bus
	[I2C_VM_OP_SEND_UAVO]    = i2c_vm_send_uavo,    //Send UAVObject
	[I2C_VM_OP_DELAY]        = i2c_vm_delay,        //Wait (ms)
};

/* Run virtual machine. This is the code that loops through and interprets all the instructions */
static bool i2c_vm_run (uint32_t * code, uint8_t code_len)
{
	static struct i2c_vm_regs vm;

	i2c_vm_reboot (&vm);

	while (!vm.halted) {
		if (vm.pc > code_len) {
			vm.fault  = true;
			vm.halted = true;
			continue;
		}
		/* Fetch */
		uint32_t instruction = code[vm.pc];

		/* Decode */
		uint8_t operator = (instruction & 0xFF000000) >> 24;
		uint8_t op1      = (instruction & 0x00FF0000) >> 16;
		uint8_t op2      = (instruction & 0x0000FF00) >>  8;
		uint8_t op3      = (instruction & 0x000000FF);

		if (operator > NELEMENTS(i2c_vm_handlers)) {
			vm.fault = true;
			vm.halted = true;
			continue;
		}
		i2c_vm_inst_handler f = i2c_vm_handlers[operator];

		/* Execute + Writeback */
		if (!f || !f(&vm, op1, op2, op3)) {
			vm.fault  = true;
			vm.halted = true;
			continue;
		}
	}

	return (!vm.fault);
}


/* Simple test program to demonstrate that the generic I2C VM works. However, 
 * there is currently no logic to configure the VM from the GCS.
 */
void gen_i2c_vm_run (void)
{
	uint32_t test_program[18];
	test_program[0]=I2C_VM_ASM_NOP();
	test_program[1]=I2C_VM_ASM_SET_CTR(10); //Set counter to 10
	test_program[2]=I2C_VM_ASM_SET_DEV_ADDR(0x53); //Set I2C device address (in 7-bit)
	test_program[3]=I2C_VM_ASM_STORE(0x2D, 0); //Store register address
	test_program[4]=I2C_VM_ASM_STORE(0x08, 1); //Store register value
	test_program[5]=I2C_VM_ASM_WRITE_I2C(2); //Write two bytes
	test_program[6]=I2C_VM_ASM_DEC_CTR(); //Decrement counter
	test_program[7]=I2C_VM_ASM_BNZ(-4); //If the counter is not zero, go back four steps
	
	test_program[8]=I2C_VM_ASM_SET_DEV_ADDR(0x53); //Set I2C device address (in 7-bit)
	test_program[9]=I2C_VM_ASM_STORE(0x32, 0); //Store register address
	test_program[10]=I2C_VM_ASM_WRITE_I2C(1);  //Write one bytes
	test_program[11]=I2C_VM_ASM_DELAY(50); //Delay 50ms
	test_program[12]=I2C_VM_ASM_READ_I2C(6);   //Read six bytes
	test_program[13]=I2C_VM_ASM_LOAD_LE(0, 2, VM_R0); //Load 16-bit formatted bytes into first output register
	test_program[14]=I2C_VM_ASM_LOAD_LE(2, 2, VM_R1); //Load 16-bit formatted bytes into second output register
	
	test_program[15]=I2C_VM_ASM_SET_DEV_ADDR(0x27); //Set I2C device address (in 7-bit)
	
	test_program[16]=I2C_VM_ASM_SEND_UAVO(); //Set the UAVObjects
	test_program[17]=I2C_VM_ASM_JUMP(-9);    //Jump back 9 steps
//	/* Generate test program as an array of integers. This would normally be done by the GCS */
//	uint32_t test_program = {
//	I2C_VM_ASM_NOP(), //Do nothing
//	
//	I2C_VM_ASM_SET_CTR(10), //Set counter to 10
//	I2C_VM_ASM_SET_DEV_ADDR(0x53), //Set I2C device address (in 7-bit)
//	I2C_VM_ASM_STORE(0x2D, 0), //Store register address
//	I2C_VM_ASM_STORE(0x08, 1), //Store register value
//	I2C_VM_ASM_WRITE_I2C(2), //Write two bytes
//	I2C_VM_ASM_DEC_CTR(), //Decrement counter
//	I2C_VM_ASM_BNZ(-4), //If the counter is not zero, go back four steps
//	
//	I2C_VM_ASM_SET_DEV_ADDR(0x53), //Set I2C device address (in 7-bit)
//	I2C_VM_ASM_STORE(0x32, 0), //Store register address
//	I2C_VM_ASM_WRITE_I2C(1),  //Write one bytes
//	I2C_VM_ASM_DELAY(50), //Delay 50ms
//	I2C_VM_ASM_READ_I2C(6),   //Read six bytes
//	I2C_VM_ASM_LOAD_LE(0, 2, VM_R0), //Load 16-bit formatted bytes into first output register
//	I2C_VM_ASM_LOAD_LE(2, 2, VM_R1), //Load 16-bit formatted bytes into second output register
//	
//	I2C_VM_ASM_SET_DEV_ADDR(0x27), //Set I2C device address (in 7-bit)
//	
//	I2C_VM_ASM_SEND_UAVO(), //Set the UAVObjects
//	I2C_VM_ASM_JUMP(-9),    //Jump back 9 steps
//	};
	
	for (int i=0; i<18; i++){
		program[i]=test_program[i];
	}

	if (i2c_vm_run(program, NELEMENTS(program))) {
		/* Program ran to completion */
	} else {
		/* Code faulted at some point */
	}
}

/**
 * @}
 * @}
 */
