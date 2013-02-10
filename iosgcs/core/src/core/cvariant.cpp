//
//  cvariant.cpp
//  gcs_osx
//
//  Created by Vova Starikh on 2/10/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#include "cvariant.h"

#include <memory.h>
#include <assert.h>

#ifdef __ANDROID__
#include "string"
#endif

CVariant::CVariant()
{
	m_type = Invalid;
	m_builtins.ui64 = 0;
}

CVariant::CVariant(const CVariant& other)
{
	*this = other;
}

CVariant::CVariant(opuint32 ui)
{
	*this = ui;
	m_type = UInt;
}

CVariant::CVariant(opint32 i)
{
	*this = i;
	m_type = Int;
}

CVariant::CVariant(opint64 i)
{
	*this = i;
	m_type = Int;
}

CVariant::CVariant(double d)
{
	*this = d;
	m_type = Double;
}

CVariant::CVariant(bool b)
{
	*this = b;
	m_type = Bool;
}

CVariant::CVariant(const char* cstr)
{
	*this = CString(cstr);
	m_type = String;
}

CVariant::CVariant(const CString& str)
{
	*this = str;
	m_type = String;
}

CVariant::CVariant(const CStringList& str)
{
	*this = str;
	m_type = StringList;
}

CVariant::CVariant(const CByteArray& data)
{
	*this = data;
	m_type = ByteArray;
}

CVariant::CVariant(Type type)
{
	*this = (opint64)0;
	m_type = type;
}

CVariant::~CVariant()
{
}

bool CVariant::operator==(const CVariant &other) const
{
	if( Invalid == m_type )
		return Invalid == other.m_type;
    
	if( ByteArray == m_type || ByteArray == other.m_type )
	{
		CByteArray array1 = this->toByteArray();
		CByteArray array2 = other.toByteArray();
        
		int len = array1.size();
		int len1 = array2.size();
		if( len != len1 )
			return false;
		const char* data = array1.data();
		const char* data1 = array2.data();
		if( data == data1 )
			return true;
		if( !data || !data1 )
			return false;
		if( !memcmp(data, data1, len) )
			return true;
		return false;
	}
	else if( String == m_type || String == other.m_type )
	{
		CString str1 = this->toString();
		CString str2 = other.toString();
		return str1 == str2;
	}
	else if( StringList == m_type || StringList == other.m_type )
	{
		return false;
	}
	else
		return m_builtins.ui64 == other.m_builtins.ui64;
}

bool CVariant::operator!=(const CVariant &other) const
{
	return !(*this == other);
}

bool CVariant::operator<(const CVariant& other) const
{
	if( Invalid == m_type )
		return Invalid != other.m_type;
    
	if (ByteArray == m_type || ByteArray == other.m_type)
	{
		CByteArray array1 = this->toByteArray();
		CByteArray array2 = other.toByteArray();
        
		int len = array1.size();
		int len1 = array2.size();
		if( len < len1 )
			return false;
		const char* data = array1.data();
		const char* data1 = array2.data();
		if( data == data1 )
			return false;
		if( !data || !data1 )
			return false;
        
		return memcmp(data, data1, len) < 0;
	}
	else if( String == m_type || String == other.m_type )
	{
		CString str1 = this->toString();
		CString str2 = other.toString();
		return str1 < str2;
	}
	else if (m_type == Double)
		return m_builtins.d < other.m_builtins.d;
	else
		return m_builtins.ui64 < other.m_builtins.ui64;
}

CVariant & CVariant::operator=( const CVariant &other )
{
	if( Invalid == other.m_type )
		this->clear();
	else
	{
		m_str           = other.m_str;
		m_strList       = other.m_strList;
		m_array         = other.m_array;
		m_builtins.ui64 = other.m_builtins.ui64;
		m_type          = other.m_type;
	}
	return *this;
}

CVariant& CVariant::operator=(opuint64 ui)
{
	m_str           = CString();
	m_strList       = CStringList();
	m_array         = CByteArray();
	m_builtins.ui64 = ui;
	m_type          = UInt;
	return *this;
}

CVariant& CVariant::operator=(opint64 i)
{
	m_str          = CString();
	m_strList       = CStringList();
	m_array        = CByteArray();
	m_builtins.i64 = i;
	m_type         = Int;
	return *this;
}

CVariant& CVariant::operator=(opuint32 ui)
{
	m_str           = CString();
	m_strList       = CStringList();
	m_array         = CByteArray();
	m_builtins.ui64 = ui;
	m_type          = UInt;
	return *this;
}

CVariant& CVariant::operator=(opint32 i)
{
	m_str          = CString();
	m_strList       = CStringList();
	m_array        = CByteArray();
	m_builtins.i64 = i;
	m_type         = Int;
	return *this;
}

CVariant& CVariant::operator=(double d)
{
	m_str        = CString();
	m_strList       = CStringList();
	m_array      = CByteArray();
	m_builtins.d = d;
	m_type       = Double;
	return *this;
}

CVariant& CVariant::operator=(bool b)
{
	m_str        = CString();
	m_strList       = CStringList();
	m_array      = CByteArray();
	m_builtins.b = b;
	m_type       = Bool;
	return *this;
}

CVariant& CVariant::operator= (const char* cstr)
{
	return *this = CString(cstr);
}

CVariant& CVariant::operator=(const CString& str)
{
	m_str           = str;
	m_strList       = CStringList();
	m_array         = CByteArray();
	m_builtins.ui64 = 0;
	m_type          = String;
	return *this;
}

CVariant& CVariant::operator=(const CStringList& str)
{
	m_str           = CString();
	m_strList       = str;
	m_array         = CByteArray();
	m_builtins.ui64 = 0;
	m_type          = StringList;
	return *this;
}

CVariant& CVariant::operator=(const CByteArray& data)
{
	m_str           = CString();
	m_strList       = CStringList();
	m_array         = data;
	m_builtins.ui64 = 0;
	m_type          = ByteArray;
	return *this;
}

opuint64 CVariant::toUInt(bool loslessConvert /*= true*/) const
{
	switch( m_type )
	{
        case Invalid:
            return 0;
        case Int:
            return (opuint64)m_builtins.i64;
        case UInt:
            return m_builtins.ui64;
        case Double:
            assert( !loslessConvert );
            return (opint64)(m_builtins.d + 0.5);
        case Bool:
            assert( !loslessConvert );
            return m_builtins.b != 0 ? 1 : 0;
        case String:
            return m_str.toUInt64();
        default:
            assert( false );
            break;
	}
	return 0;
}

opint64 CVariant::toInt( bool loslessConvert /*= true*/ ) const
{
	switch( m_type )
	{
        case Invalid:
            return 0;
        case Int:
            return m_builtins.i64;
        case UInt:
            return (opint64)m_builtins.ui64;
        case Double:
            assert( !loslessConvert );
            return (opint64)(m_builtins.d + 0.5);
        case Bool:
            assert( !loslessConvert );
            return m_builtins.b != 0 ? 1 : 0;
        case String:
            return m_str.toInt64();
        default:
            assert( false );
            break;
	}
	return 0;
}

double CVariant::toDouble( bool loslessConvert /*= true*/ ) const
{
	switch( m_type )
	{
        case Invalid:
            return 0;
        case Int:
            return (double)m_builtins.i64;
        case UInt:
            return (double)m_builtins.ui64;
        case Double:
            return m_builtins.d;
        case Bool:
            assert( !loslessConvert );
            return m_builtins.b != 0 ? 1.0 : 0.0;
        case String:
            return m_str.toDouble();
        default:
            assert( false );
            break;
	}
	return 0;
}

bool CVariant::toBool( bool loslessConvert /*= true*/ ) const
{
	switch( m_type )
	{
        case Invalid:
            return false;
        case Int:
            assert( !loslessConvert );
            return m_builtins.i64 != 0;
        case UInt:
            assert( !loslessConvert );
            return m_builtins.ui64 != 0;
        case Double:
            assert( !loslessConvert );
            return m_builtins.d != 0.0;
        case Bool:
            return m_builtins.b;
        case String:
            return m_str.toBool();
        default:
            assert( false );
            break;
	}
	return false;
}

CString CVariant::toString() const
{
	switch( m_type )
	{
        case Invalid:
            return CString();
        case Int:
            return CString( m_builtins.i64 );
        case UInt:
            return CString( m_builtins.ui64 );
        case Double:
            return CString( m_builtins.d );
        case Bool:
            return CString( m_builtins.b );
        case String:
            return m_str;
        case StringList:
            return m_strList.join( "\r\n" );
        default:
            assert( false );
            break;
	}
	return CString();
}

CStringList CVariant::toStringList() const
{
	CStringList list;
	switch( m_type )
	{
        case Invalid:
            return CStringList();
        case Int:
            list << CString(m_builtins.i64);
            break;
        case UInt:
            list << CString(m_builtins.ui64);
            break;
        case Double:
            list << CString(m_builtins.d);
            break;
        case Bool:
            list << CString(m_builtins.b);
            break;
        case String:
            list << m_str;
            break;
        case StringList:
            return m_strList;
        case ByteArray:
		{
			if( m_array.isString() )
			{
				CString str( m_array.data(), m_array.size() );
				list << str;
			}
			else
				assert( false );
		}
            break;
        default:
            assert( false );
            break;
	}
	return list;
}

CByteArray CVariant::toByteArray() const
{
	switch( m_type )
	{
        case Invalid:
            return CByteArray();
        case String:
            return CByteArray(m_str.data(), m_str.size());
        case ByteArray:
            return m_array;
        default:
            assert(false);
            break;
	}
	return CByteArray();
}

bool CVariant::isValid() const
{
	return Invalid != m_type;
}

CVariant::Type CVariant::type() const
{
	return m_type;
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

void CVariant::clear()
{
	m_str           = CString();
	m_array         = CByteArray();
	m_builtins.ui64 = 0;
	m_type          = Invalid;
}
