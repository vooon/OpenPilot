//
//  cvariant.h
//  gcs_osx
//
//  Created by Vova Starikh on 2/10/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#ifndef __gcs_osx__cvariant__
#define __gcs_osx__cvariant__

#include "../optypes.h"
#include "../text/cstring.h"
#include "../text/cstringlist.h"
#include "cbytearray.h"

class CVariant
{
public:
	enum Type
	{
		Invalid,
		Int,
		UInt,
		Double,
		Bool,
		String,
		StringList,
		ByteArray
	};
    
public:
	CVariant();
	CVariant(const CVariant& other);
	CVariant(opint64 i);
	CVariant(opuint32 ui);
	CVariant(opint32 i);
	CVariant(double d);
	CVariant(bool b);
	CVariant(const CString& str);
	CVariant(const CStringList& str);
	CVariant(const CByteArray& data);
	CVariant(const char* cstr);
	CVariant(Type type);
	~CVariant();
    
	bool operator== (const CVariant& other) const;
	bool operator!= (const CVariant& other) const;
	bool operator<	(const CVariant& other) const;
    
	CVariant& operator= (const CVariant& other);
	CVariant& operator= (opuint64 ui);
	CVariant& operator= (opint64 i);
	CVariant& operator= (opuint32 ui);
	CVariant& operator= (opint32 i);
	CVariant& operator= (double d);
	CVariant& operator= (bool b);
	CVariant& operator= (const char* cstr);
	CVariant& operator= (const CString& str);
	CVariant& operator= (const CStringList& str);
	CVariant& operator=(const CByteArray& data);
    
	opuint64 toUInt(bool loslessConvert = true) const;
	opint64 toInt(bool loslessConvert = true) const;
	double toDouble(bool loslessConvert = true) const;
	bool toBool(bool loslessConvert = true) const;
	CString toString() const;
	CStringList toStringList() const;
	CByteArray toByteArray() const;
    
	bool isValid() const;
    
	Type type() const;
    
private:
	union {
		bool     b;
		opint64  i64;
		opuint64 ui64;
		double   d;
	} m_builtins;
    
	Type        m_type;
	CString     m_str;
	CStringList m_strList;
	CByteArray  m_array;
    
	void clear();
};

#endif /* defined(__gcs_osx__cvariant__) */
