/* -*- Mode: c; c-basic-offset: 2; tab-width: 2; indent-tabs-mode: t -*- */
/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup TextStarModule TextStar Module
 * @brief Interface to TextStar serial LCD module
 * This module displays status on the TextStar serial LCD module, which can
 * be found here:
 *
 *  http://cats-whisker.com/web/node/7
 *
 * This module also supports changing system paramters (PIDs, etc) using the
 * input buttons on the TextStar
 *
 * @{ 
 *
 * @file       textstar.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      Include file of the textstar module.
 * 	       As with all modules only the initialize function is exposed all other
 * 	       interactions with the module take place through the event queue and
 *             objects.
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

#ifndef TEXTSTAR_H
#define TEXTSTAR_H

int32_t TextStarInitialize(void);

#endif // TEXTSTAR_H

/**
  * @}
  * @}
  */
