/**
 ******************************************************************************
 *
 * @file       debugmod_priv.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      The debug module adds a debug queue and a thread to print the
 *             debug queue to the debug serial port.
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
#ifndef DEBUGMOD_PRIV_H
#define DEBUGMOD_PRIV_H

// Types
typedef struct {
  xQueueHandle queue;
  xTaskHandle taskHandle;
  char buf[DEBUG_QUEUE_BUF_SIZE];
} DebugModData;

#endif // DEBUGMOD_H
