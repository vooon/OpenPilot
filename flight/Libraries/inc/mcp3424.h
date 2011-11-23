/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup GasEngine Module
 * @brief Communicate with MCP3424 and update @ref mcp3424_sensor "mcp3424_sensor UAV Object"
 * @{ 
 *
 * @file       mcp3424.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      mcp3424 module, reads 4 analog channels from mcp3424 via I2C
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
#ifndef MCP3424_H
#define MCP3424_H

uint8_t MCP3424_GetGain(uint8_t x);
uint8_t MCP3424_GetResolution(uint8_t x);
bool MCP3424_GetAnalogValue(uint16_t I2CAddress, uint8_t channel, uint8_t* pBuffer, uint8_t resolution, uint8_t gain, double_t* analogValue);

#endif // MCP3424_H

/**
 * @}
 * @}
 */
