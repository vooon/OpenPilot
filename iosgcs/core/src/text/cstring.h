/**
 ******************************************************************************
 *
 * @file       cstring.h
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

#ifndef __OpenPilotGCS__cstring__
#define __OpenPilotGCS__cstring__

#include "../optypes.h"
#include "../core/cbytearray.h"

class CStringPrivate;

class CString
{
public:
    CString();
    CString(const CString& other);
    CString(const char* str, int len = -1);
    CString(char ch);
    CString(opuint64 ui64, int width = -1, int base = 10, char fillChar = ' ');
    CString(opint64 i64);
    CString(opuint32 ui64);
    CString(opint32 i32);
    CString(opuint16 ui16);
    CString(opint16 i16);
    CString(double val, int precision = 0);
    ~CString();

    bool isEmpty() const;
    // return the number of characters per string
    int length() const;
    // returns the size of the strung
    int size() const;
    const char* data() const;
    bool getCharacters(opuint32* buffer, const RANGE& range) const;
    opuint32 at(int i) const;
    opuint32 operator[](int i) const;
    CString left(int size) const;

    CString& operator=(const CString& other);
    CString& operator=(const char* str);

    CString& operator+=(const CString& str);
    CString& operator+=(const char* str);
    CString& operator+=(char val);
    CString& operator+=(opuint64 ui64);
    CString& operator+=(opint64 i64);
    CString& operator+=(opuint32 ui32);
    CString& operator+=(opint32 i32);
    CString& operator+=(opuint16 ui16);
    CString& operator+=(opint16 i16);
    CString& operator+=(double val);

    CString& operator<<(const char* str);
    CString& operator<<(const CString& text);
    CString& operator<<(opint32 i);
    CString& operator<<(opuint32 i);
    CString& operator<<(opint64 i);
    CString& operator<<(opuint64 i);

    bool operator==(const CString& str) const;
    bool operator!=(const CString& str) const;
    bool operator<(const CString& str) const;

    CString& prepend(const CString& str);
    CString& prepend(const char* str);
    CString& insert(const CString& what, int from);
    CString& insert(const char* what, int from);
    CString& remove(int start, int len);
    CString& remove(const RANGE& range);
    CString& remove(const CString& str);
    CString& remove(const char* str);
    CString& replace(const CString& subject, const CString& replacement);
    CString& replace(const char* subject, const char* replacement);
    CString& replaceAll(const CString& subject, const CString& replacement);
    bool contains(const CString& str) const;
    bool contains(const char* str) const;
    int  indexOf(const CString& str, int start) const;
    int  lastIndexOf(const CString& str, int start) const;

    CString toLower() const;
    CString toUpper() const;

    CString arg(const CString& argument) const;
    CString arg(opuint64 argument, int width, int base = 10, char fillChar = ' ') const;

    opuint64 toInt64() const;
    opuint64 toUInt64() const;
    double toDouble() const;
    bool toBool() const;

    // find and highlight a substring
    int find( const CString& what, opuint32 from = 0 ) const;
    int find( const char* what, opuint32 from = 0 ) const;
    CString substr (int from, int to = -1) const;

private:
    CStringPrivate * d;

    void makeNewDataIfNeed();
    RANGE findMinArg() const;
    // converts unichar* array in char*
    // (in the array must be numbers only)
    char* toCharArray( opuint32* buffer, int length ) const;
    // collect char* to convert to number
    char* makeArrayForIntConvert() const;

private:
};

bool operator==( const char* str1, const CString& str );
bool operator==( const CString& str, const char* str1 );

const CString operator+(const CString &s1, const CString &s2);
const CString operator+(const CString &s1, const char* s2);

#ifdef LIB_FOR_QT
#ifdef _WIN32
#define TO_QSTRING(x) QString::fromUtf8( x.data() )
#else
#define TO_QSTRING(x) QString::fromUtf8( x.data() )
#endif
#endif // LIB_FOR_QT

#endif /* defined(__OpenPilotGCS__cstring__) */
