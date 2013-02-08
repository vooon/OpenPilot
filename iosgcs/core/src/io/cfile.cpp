//
//  cfile.cpp
//  gcs_osx
//
//  Created by Vova Starikh on 2/8/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#include "cfile.h"
#include "../op_errors.h"

#include <assert.h>

CFile::CFile(const CString& fileName /*= CString()*/)
{
	m_file      = 0;
	m_openFlags = 0;
#ifdef _WIN32
	m_hFile     = INVALID_HANDLE_VALUE;
	m_hMap      = 0;
#endif // _WIN32
	setFileName( fileName );
}

CFile::~CFile()
{
	if (m_file)
		close();
}

// return the type of the device
CIODevice::DeviceType CFile::deviceType() const
{
    return DeviceTypeFile;
}

/////////////////////////////////////////////////////////////////////
//                          parameters of file                     //
/////////////////////////////////////////////////////////////////////

void CFile::setFileName(const CString& fileName)
{
	if (isOpen())
		return;
	m_fileName = fileName;
}

CString CFile::fileName()
{
    return m_fileName;
}

/////////////////////////////////////////////////////////////////////
//                           open/close                            //
/////////////////////////////////////////////////////////////////////

bool CFile::open(opuint32 mode)
{
	if( m_fileName.isEmpty() || isOpen() )
		return false;
    
	CString strMod;
	if( (mode & ReadOnly) && (mode & WriteOnly) )
		strMod += "a+";
    //		strMod += "w+";
	else if( mode & ReadOnly )
		strMod += "r";
	else if( mode & WriteOnly )
		strMod += "w";
	strMod += "b";
    
#ifndef _WIN32
	m_file = fopen(m_fileName.data(), strMod.data());
#else
#pragma message("need write conversion to utf16 in class of CString")
//	// кусок для тупорылой винды - она не понимает человеческую кодировку UTF-8
//	// а по сему придется юзать припезденные функции!!!!
//	CByteArray utf16Name = m_fileName.toUtf16();
//	CByteArray utf16Mod = strMod.toUtf16();
//	m_file = _wfopen( (wchar_t*)utf16Name.getData(), (wchar_t*)utf16Mod.getData() );
#endif
	if (!m_file){
        //Logger() << __PRETTY_FUNCTION__ << "Error opening file " << m_fileName << ": " << strerror(errno);
		return false;
	}
	m_openFlags = mode;
	return true;
}

void CFile::close()
{
	if (!m_file)
		return;
	fclose(m_file);
	m_file = 0;
}

bool CFile::isOpen() const
{
	if (m_file)
		return true;
	return false;
}

bool CFile::remove()
{
	if (m_fileName.isEmpty())
		return false;			// не могу удалить неизвестный файл
#ifndef _WIN32
	if (!::remove(m_fileName.data()))
		return true;
#else
#pragma message("need write conversion to utf16 in class of CString")
//	CByteArray utf16Name = fileName().toUtf16();
//	if( !_wremove((wchar_t*)utf16Name.getData()) )
//		return true;
#endif
//	Logger() << "Error removing file " << m_fileName << ", errno = " << strerror(errno);
	return false;
}

bool CFile::rename(const CString& newName)
{
	if( m_fileName.isEmpty() || newName.isEmpty() )
		return false;
#ifndef _WIN32
	if (::rename(m_fileName.data(), newName.data()))
	{
//		Logger() << "Error renaming file " << m_fileName << " to " << newName << ", errno = " << strerror(errno);
		return false;
	}
#else
#pragma message("need write conversion to utf16 in class of CString")
//	if (_wrename((wchar_t*)m_fileName.toUtf16().getData(), (wchar_t*)newName.toUtf16().getData()) != 0)
//	{
////		Logger() << "Error renaming file " << m_fileName << " to " << newName << ", errno = " << strerror(errno);
//		return false;
//	}
#endif
	close();
	setFileName(newName);
    
	return true;
}

/////////////////////////////////////////////////////////////////////
//                                  io                             //
/////////////////////////////////////////////////////////////////////

int CFile::write(const void* data, int len)
{
	if( !m_file )
		return 0;
	int ret = (int)fwrite( data, 1, len, m_file );
	return ret;
}

int CFile::write(const CByteArray& data)
{
    return write((void*)data.data(), data.size());
}

int CFile::read(void* data, int len) const
{
	if( !m_file )
		return 0;
	int ret = (int)fread( data, 1, len, m_file );
	return ret;
    
}

CByteArray CFile::readAll()
{
	opint64 pos, size;
	pos = this->pos();
	size = this->size();
	if( 0 == size )
		return CByteArray();
    
	if (seek(0) != OP_OK)
		return CByteArray();
	CByteArray data;
	data.setSize( int(size) );
	read((char*)data.data(), data.size());
	seek (pos);
	return data;
}

void CFile::flush()
{
	if (!m_file)
		return;
	fflush(m_file);
}

/////////////////////////////////////////////////////////////////////
//                                                                 //
/////////////////////////////////////////////////////////////////////

int CFile::seek(opint64 pos)
{
	if( !m_file )
		return OPERR_INVALIDCALL;
#if defined(__APPLE__) || defined(__ANDROID__)
	if (!fseeko(m_file, pos, SEEK_SET) )
		return OP_OK;
#elif !defined(_WIN32)
	if (!fseeko64(m_file, pos, SEEK_SET))
		return OP_OK;
#else
	if (!_fseeki64(m_file, pos, SEEK_SET))
		return OP_OK;
#endif
	return OPERR_FAIL;
}

opuint64 CFile::pos() const
{
	if (!m_file)
		return (opuint64)-1;
	opuint64 ret = 0;
#if defined(__APPLE__) || defined(__ANDROID__)
	ret = ftello(m_file);
#elif !defined(_WIN32)
	ret = ftello64(m_file);
#else
	ret = (opuint64)_ftelli64(m_file);
#endif
	return ret;
}

opuint64 CFile::size() const
{
	if( !m_file )
		return 0;
	opuint64 currentPos = pos();
	fseek(m_file, 0, SEEK_END);
	opuint64 size = pos();
    
#if defined(__APPLE__) || defined(__ANDROID__)
    int ret = fseeko(m_file, currentPos, SEEK_SET);
	assert(!ret);
#elif !defined(_WIN32)
    /*ret =*/ fseeko64(m_file, currentPos, SEEK_SET);
#else
    /*ret =*/ _fseeki64(m_file, (__int64)currentPos, SEEK_SET);
#endif
	return size;
}
