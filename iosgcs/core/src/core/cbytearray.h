/**
 ******************************************************************************
 *
 * @file       cbytearray.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
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

#ifndef __OpenPilotGCS__cbytearray__
#define __OpenPilotGCS__cbytearray__

#include "../optypes.h"
#include "../system/sharedptr.h"

typedef struct tagRange
{
    tagRange()
    {
        location = -1;
        length   = 0;
    }
    tagRange(int loc, int len)
    {
        location = loc;
        length   = len;
    }
	
    int location;
    int length;

} RANGE, *PRANGE;

class CString;

class CByteArrayPrivate;

class CByteArray
{
public:
	CByteArray();
	CByteArray(const char* data, opuint32 size);
	CByteArray(const unsigned char* data, opuint32 size);
	CByteArray(const CByteArray& data);
    CByteArray(const CString& string);            // construct byte array from text dump
    ~CByteArray();
    
	const CByteArray& operator=(const CByteArray& other);
	CByteArray& operator+=(const CByteArray& other);
	char& operator[]( int pos );
	char operator[]( int pos ) const;
    
	CByteArray& append(const char* data, opuint32 size);
	CByteArray& append(const char* data, const RANGE& range);
	CByteArray& append(const CByteArray& other);
	CByteArray& insert(opuint32 start, const char* data, opuint32 size);
	CByteArray& remove(int start, int len);
    
	void fromRawData(const char* data, opuint32 size);
    
	bool isEmpty() const;
	// specified byte array is equal our array?
	bool isEqual( const CByteArray& other ) const;
	// data in the buffer as a text?
	// 0 must meet  only at the and, or do not meet
	bool isString() const;
	bool setSize(opuint32 newSize );
	opuint32 size() const;
    
	const char* data() const;
    
	// string with a dump of the object
	void generateDump(CString& dump, bool addSpacesBetweenBytes = false) const;

private:
    CByteArrayPrivate* d;
    
    void makeNewDataIfNeed();
};

#endif /* defined(__OpenPilotGCS__cbytearray__) */
