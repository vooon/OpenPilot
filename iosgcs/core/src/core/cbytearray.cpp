/**
 ******************************************************************************
 *
 * @file       cbytearray.cpp
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

#include "cbytearray.h"
#include "../thread/cmutex.h"
#include "../text/cstring.h"
#include "../system/cexception.h"
#include "../op_errors.h"

#include <stdlib.h>
#include <assert.h>

class CByteArrayPrivate
{
public:
	CByteArrayPrivate(opuint32 size, const char* data = 0);
	CByteArrayPrivate(const CByteArrayPrivate& other);
	~CByteArrayPrivate();

	void ref();
	void deref();
	opuint32 refs() const;

	const CByteArrayPrivate& operator=(const CByteArrayPrivate& other);
    
	bool setSize(opuint32 newSize );
	opuint32 size() const;
	char*& data();
    
public:
    opuint32 m_refCount;
	opuint32 m_size;
	char*    m_data;
    mutable CMutex   m_mutex;
};


CByteArrayPrivate::CByteArrayPrivate(opuint32 size, const char *data /*= 0*/)
{
	m_data = (char*)malloc( size );
	if( m_data )
		m_size = size;
	else
		return;
    
	if( data )
		memcpy( m_data, data, size );
}

CByteArrayPrivate::CByteArrayPrivate(const CByteArrayPrivate& other)
{
	m_data     = 0;
	m_size     = 0;
    
	operator=( other );
}

CByteArrayPrivate::~CByteArrayPrivate()
{
	if( m_data )
	{
		free(m_data);
		m_data = 0;
	}
	m_size = 0;
}

void CByteArrayPrivate::ref()
{
	CMutexSection locker( &m_mutex );
	++m_refCount;
}

void CByteArrayPrivate::deref()
{
	CMutexSection locker( &m_mutex );
	if( !m_refCount )
		return;
	if( m_refCount > 1 )
		--m_refCount;
	else
	{
		locker.unlock();
		delete this;
	}
}

opuint32 CByteArrayPrivate::refs() const
{
	CMutexSection locker( &m_mutex );
	return m_refCount;
}

const CByteArrayPrivate& CByteArrayPrivate::operator=(const CByteArrayPrivate &other)
{
	if(m_data == other.m_data)
		return *this;
	if (m_data)
	{
		free(m_data);
		m_data = 0;
	}
	m_size = 0;
	if(other.m_size)
	{
		m_data = (char*)malloc(other.m_size);
		if (m_data)
		{
			m_size = other.m_size;
			memcpy(m_data, other.m_data, m_size);
		}
	}
	return *this;
}

bool CByteArrayPrivate::setSize(opuint32 newSize)
{
	if( !newSize )
	{
		m_size = 0;
		if( m_data )
		{
			free( m_data );
			m_data = 0;
		}
	}
	else
	{
		char* data = (char*)realloc( m_data, newSize );
		if( !data )
			return false;
		m_data = data;
		m_size = newSize;
	}
	return true;
}

opuint32 CByteArrayPrivate::size() const
{
	return m_size;
}

char*& CByteArrayPrivate::data()
{
	return m_data;
}


CByteArray::CByteArray()
{
    d = 0;
}

CByteArray::CByteArray(const char* data, opuint32 size)
{
    d = new CByteArrayPrivate(size, data);
    d->ref();
}

CByteArray::CByteArray(const unsigned char* data, opuint32 size)
{
    d = new CByteArrayPrivate(size, (const char*)data);
    d->ref();
}

CByteArray::CByteArray(const CByteArray& data)
{
	d = data.d;
	if( d )
		d->ref();
}

CByteArray::CByteArray(const CString& string)            // construct byte array from text dump
{
	d = 0;
	//Каждый байт кодируется двумя 16-ричными разрядами (символами)
	if (string.length() % 2)
	{
		assert(false);
		return;
	}
    
	for (int i = 0;i<string.length();i+=2)
	{
		char highDigit = string.at(i), lowDigit = string.at(i + 1);
		unsigned char byte = highDigit * 16 + lowDigit;
		append((char*)&byte, 1);
	}
}

CByteArray::~CByteArray()
{
	if( d )
		d->deref();
}

const CByteArray& CByteArray::operator=(const CByteArray& other)
{
	if (d)
	{
		d->deref();
		d = 0;
	}
	if( other.d )
	{
		other.d->ref();
		d = other.d;
	}
	return *this;
}

CByteArray& CByteArray::operator+=(const CByteArray& other)
{
    return append(other.data(), other.size());
}

char& CByteArray::operator[](int pos)
{
	if( !d || pos >= int(d->m_size) )
		throw CException(OPERR_INVALIDPARAM, "invalid position in CByteArray::operator[]");
	return d->m_data[pos];
}

char CByteArray::operator[]( int pos ) const
{
	if( !d || pos >= int(d->m_size) )
		throw CException(OPERR_INVALIDPARAM, "invalid position in CByteArray::operator[]");
	return d->m_data[pos];
}

CByteArray& CByteArray::append(const char* data, opuint32 size)
{
	if (!data)
		return *this;
	if( d )
	{
		if( d->refs() == 1 )
		{
			opuint32 oldSize = d->size();
			if( !d->setSize(oldSize+size) )
				return *this;
            if (d->m_data && data)
                memcpy(d->data() + oldSize, data, size);
			return *this;
		}
		else
		{
			d->deref();
			d = new CByteArrayPrivate(d->size(), d->data());
            d->ref();
			return append(data, size);
		}
	}
	else
	{
		d = new CByteArrayPrivate(size, data);
        d->ref();
		return *this;
	}
}

CByteArray& CByteArray::append(const char* data, const RANGE& range)
{
	if( !data || -1 == range.location || 0 == range.length )
		return *this;
	makeNewDataIfNeed();
	int oldSize = d->size();
	if( !d->setSize(oldSize+range.length) )
	{
		assert(0 == "not enough memory");
		return *this;
	}
	memcpy( d->m_data+oldSize, data+range.location, range.length );
	return *this;
}

CByteArray& CByteArray::append(const CByteArray& other)
{
    return append(other.data(), other.size());
}

CByteArray& CByteArray::insert(opuint32 start, const char* data, opuint32 size)
{
	if( !size || !data )
		return *this;
	if( start > d->m_size )
		return *this;
	opuint32 oldSize = this->size();
	setSize(this->size()+size );
	char* dst = d->m_data+d->m_size-1;
	const char* src = d->m_data+oldSize-1;
	for( int i = 0;i<(int)(oldSize-start);i++ )
	{
		if (dst && src)
			*dst = *src;
		dst--;
		src--;
	}
	memcpy( d->m_data+start, data, size );
	return *this;
}

CByteArray& CByteArray::remove(int start, int len)
{
	// проверим входящие параметры
	if( start >= int(d->m_size) )
		return *this;
	if( len > int(d->m_size-start) )
		len = d->m_size-start;
	// теперь скопируем данные
	char* dst = d->m_data+start;
	const char* src = d->m_data+start+len;
	int size = d->m_size - (start+len);
	for( int i = 0;i<size;i++ )
	{
		*dst = *src;
		dst++;
		src++;
	}
	// а теперь изменим размер памяти
	setSize( d->m_size -len );
	return *this;
}

void CByteArray::fromRawData(const char* data, opuint32 size)
{
	if( d )
		d->deref();
	d = new CByteArrayPrivate(size, data);
    d->ref();
}

bool CByteArray::isEmpty() const
{
	if (!data() || !size())
		return true;
	return false;
}

// specified byte array is equal our array?
bool CByteArray::isEqual( const CByteArray& other ) const
{
	if (d == other.d)
		return true;
	if (size() != other.size())
		return false;
	int size = int(this->size());
	const char* ptr1 = data();
	const char* ptr2 = other.data();
	for( int i = 0;i<size;i++ )
	{
		if( *ptr1 != *ptr2 )
			return false;
		ptr1++; ptr2++;
	}
	return true;
}

// data in the buffer as a text?
// 0 must meet  only at the and, or do not meet
bool CByteArray::isString() const
{
	if( !d )
		return false;
	const char* ptr = d->data();
	for (opuint32 i = 0;i<d->m_size;i++, ptr++)
	{
		if (*ptr == 0)
		{
			if( i != d->m_size-1 )
				return false;
			else
				return true;
		}
	}
	return true;
}

bool CByteArray::setSize(opuint32 newSize )
{
	if( !d )
    {
		d = new CByteArrayPrivate(newSize, 0);
        d->ref();
    }
	else
	{
		if( 1 != d->refs() )
		{
			CByteArrayPrivate* old_d = d;
			d->deref();
			d = new CByteArrayPrivate(newSize);
            d->ref();
			if( newSize && old_d->size() )
			{
				opuint32 size = newSize;
				if (size > old_d->size())
					size = old_d->size();
				if (d->data())
					memcpy((char*)d->data(), old_d->data(), size);
			}
		}
		else
			d->setSize( newSize );
	}
	return true;
}

opuint32 CByteArray::size() const
{
	if( d )
		return d->size();
	else
		return 0;
}

const char* CByteArray::data() const
{
	if( d )
		return d->data();
	else
		return 0;
}

// string with a dump of the object
void CByteArray::generateDump(CString& dump, bool addSpacesBetweenBytes /*= false*/) const
{
	static const char hexVal[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	assert(dump.isEmpty());
    
	for (opuint32 i = 0;i<size();++i)
	{
		char byteTextHex [3] = {0};
		unsigned char byte = *reinterpret_cast<const unsigned char*>(data() + i);
		unsigned char hexDigitHigh = byte / 16, hexDigitLow = byte & 0x0F;
        
		byteTextHex[0] = hexVal[hexDigitHigh];
		byteTextHex[1] = hexVal[hexDigitLow];
        
		dump += byteTextHex;
		if (addSpacesBetweenBytes)
			dump += ' ';
	}
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

void CByteArray::makeNewDataIfNeed()
{
	if( !d )
    {
		d = new CByteArrayPrivate((int)0);
        d->ref();
    }
	else if( 1 != d->refs() )
	{
		CByteArrayPrivate* new_d = new CByteArrayPrivate(*d);
		assert(new_d);
		d->deref();
		d = new_d;
        d->ref();
	}
}
