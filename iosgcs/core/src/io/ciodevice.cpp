/**
******************************************************************************
*
* @file       ciodevice.cpp
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

#include "ciodevice.h"
#include "../op_errors.h"

CIODevice::CIODevice()
{
}

CIODevice::~CIODevice()
{
}

// return the type of the device
CIODevice::DeviceType CIODevice::deviceType() const
{
	return DeviceTypeUnknown;
}

/////////////////////////////////////////////////////////////////////
//                             open/close                          //
/////////////////////////////////////////////////////////////////////

bool CIODevice::open( opuint32 /*mode*/ )
{
	return false;
}

void CIODevice::close()
{
}

bool CIODevice::isOpen() const
{
	return false;
}

bool CIODevice::isSequential() const
{
    return false;
}

/////////////////////////////////////////////////////////////////////
//                               io                                //
/////////////////////////////////////////////////////////////////////

int CIODevice::write( const void* /*data*/, int /*len*/ )
{
	return OPERR_INVALIDCALL;
}

int CIODevice::read( void* /*data*/, int /*len*/ ) const
{
	return OPERR_INVALIDCALL;
}

opint64 CIODevice::bytesAvailable() const
{
    return 0;
}

/////////////////////////////////////////////////////////////////////
//                                                                 //
/////////////////////////////////////////////////////////////////////

int CIODevice::seek( opint64 /*pos*/ )
{
	return OPERR_INVALIDCALL;
}

opuint64 CIODevice::pos() const
{
	return 0;
}

opuint64 CIODevice::size() const
{
	return 0;
}

/////////////////////////////////////////////////////////////////////
//                             signals                             //
/////////////////////////////////////////////////////////////////////

CL_Signal_v1<CIODevice*>& CIODevice::signal_readyRead()
{
    return m_signalReadyRead;
}

CL_Signal_v1<CIODevice*>& CIODevice::signal_deviceClosed()
{
    return m_signalDeviceClosed;
}
