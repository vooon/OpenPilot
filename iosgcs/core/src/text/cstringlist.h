/**
 ******************************************************************************
 *
 * @file       cstringlist.h
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

#ifndef __gcs_osx__cstringlist__
#define __gcs_osx__cstringlist__

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

#include "../templ/oplist.h"
#include "cstring.h"

class CStringList : public oplist<CString>
{
public:
	CStringList() {}
	~CStringList() {}
    
	static CStringList splitString( const CString& str, char delim );
	CString join( const CString& delim ) const;
    
	inline bool contains( const CString& str ) const {
		oplist<CString>::const_iterator it;
		for( it = this->begin();it!=this->end();it++ )
			if( str == (*it) )
				return true;
		return false;
	}
    
	inline bool contains( const char* str ) const {
		CStringList::const_iterator it;
		for( it = this->begin();it!=this->end();it++ )
			if( (*it) == str )
				return true;
		return false;
	}
    
	inline CStringList& operator<<( const CString& str ) {
		(*this).push_back( str );
		return *this;
	}
    
	inline CStringList& operator<<( const char* str ) {
		(*this).push_back( CString(str) );
		return *this;
	}
	inline CString at( int pos ) const {
		if( pos < 0 || pos >= int(size()) )
			return CString();
        
		oplist<CString>::const_iterator it = begin();
		for (int i = 0; i < pos; ++i, ++it) { }
        return *it;
	}
    
private:
};

#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32

#endif /* defined(__gcs_osx__cstringlist__) */
