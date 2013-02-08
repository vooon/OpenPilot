/**
 ******************************************************************************
 *
 * @file       cstring_p.h
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

#ifndef __OpenPilotGCS__cstring_p__
#define __OpenPilotGCS__cstring_p__

#include "../optypes.h"

class CStringPrivate
{
public:
    CStringPrivate();
    CStringPrivate( const CStringPrivate& src );
    ~CStringPrivate();

    opuint32 addRef();
    opuint32 release();
    opuint32 refs();

    void setSize( int size );
    void clear();

    CStringPrivate& operator=( const CStringPrivate& src );
    CStringPrivate& operator=( const char* str );

    CStringPrivate& operator+=( const CStringPrivate& other );
    CStringPrivate& operator+=( const char* str );

    void prepend( CStringPrivate& str );
    void prepend( const char* str );
    void insert( const CStringPrivate& str, int start );
    void insert( const char* str, int start );
    void remove( int start, int len );

    bool contains( const CStringPrivate& str ) const;
    bool contains( const char* str ) const;
    int find( const CStringPrivate& what, int from = 0 );
    int find( const char* what, int from = 0 );

    int length() const;
    int numChars() const;

public:
    unsigned int   m_refs;
    char         * m_data;
    int            m_length;
    int            m_numChars;

    void recalcLength();
};

#endif /* defined(__OpenPilotGCS_osx__cstring_p__) */
