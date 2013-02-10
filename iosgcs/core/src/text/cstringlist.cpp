/**
 ******************************************************************************
 *
 * @file       cstringlist.cpp
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

#include "cstringlist.h"

#include "unicode/gunicode.h"

CStringList CStringList::splitString( const CString& str, char delim )
{
	if( str.isEmpty() )
		return CStringList();
    
	CStringList list;
	CString result;
	RANGE range;
	range.location = 0;
	//		int pos = 0;
	int length = str.size();
	const char* ptr = str.data();
	const char* cur = ptr;
	const char* end = ptr+length;
	while( cur < end )
	{
		if( (char)g_utf8_get_char(cur) == delim )
		{
			range.length = int((cur - ptr)) - range.location;
			//				range.length = pos - range.location;
			result = CString( ptr+range.location, range.length );
			list.push_back( result );
			range.location = int(g_utf8_next_char(cur)-ptr);
		}
		cur = g_utf8_next_char( cur );
	}
	range.length = int((cur-ptr)) - range.location;
	//		range.length = pos - range.location-1;
	result = CString( ptr+range.location, range.length );
	list.push_back( result );
	return list;
}

CString CStringList::join( const CString& delim ) const
{
	if( !size() )
		return CString();
	CStringList::const_iterator it = this->begin();
	CString str = (*it);
	it++;
	for( ;it!=this->end();it++ )
	{
		str += delim;
		str += (*it);
	}
	return str;
}
