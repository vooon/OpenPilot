/**
 ******************************************************************************
 *
 * @file       ciodevice.h
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

#ifndef __OpenPilotGCS_osx__ciodevice__
#define __OpenPilotGCS_osx__ciodevice__

#include "../optypes.h"

class CIODevice
{
public:
	enum DeviceType
	{
		DeviceTypeUnknown,
		DeviceTypeFile,
		DeviceTypeMemoryDevice,
		DeviceTypeResourceReader
	};
	enum FileOpenMode
	{
		ReadOnly = 1,
		WriteOnly
	};
    
public:
	CIODevice();
	virtual ~CIODevice();
    
	// return the type of the device
	virtual DeviceType deviceType() const;
    
// open/close
	virtual bool open( opuint32 mode );
	virtual void close();
	virtual bool isOpen() const;
    
// io
	virtual int write( const void* data, int len );
	virtual int read( void* data, int len ) const;
    
//
	virtual int seek( opint64 pos );
	virtual opuint64 pos() const;
	virtual opuint64 size() const;
};

#endif /* defined(__OpenPilotGCS_osx__ciodevice__) */
