/**
 ******************************************************************************
 *
 * @file       op_errors.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @see        The GNU Public License (GPL) Version 3
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

#ifndef OpenPilotGCS_osx_op_errors_h
#define OpenPilotGCS_osx_op_errors_h

enum OpenPilotErrors
{
    OP_OK                = 0,
    OPERR_FAIL           = -1,
    OPERR_INVALIDCALL    = -2,
    OPERR_INVALIDDATA    = -3,
    OPER_OUTOFMEMORY     = -4,
    OPERR_NOTIMPLEMENTED = -5,
    OPERR_INVALIDPARAM   = -6,
    OPERR_OUTOFMEMORY    = -7
};

#endif
