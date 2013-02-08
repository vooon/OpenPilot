//
//  cfile.h
//  gcs_osx
//
//  Created by Vova Starikh on 2/8/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#ifndef __gcs_osx__cfile__
#define __gcs_osx__cfile__

#include "ciodevice.h"
#include "../text/cstring.h"
#include <stdio.h>

class CFile : public CIODevice
{
public:
public:
	enum Permission {
		ReadOwner = 0x4000, WriteOwner = 0x2000, ExeOwner = 0x1000,
		ReadUser  = 0x0400, WriteUser  = 0x0200, ExeUser  = 0x0100,
		ReadGroup = 0x0040, WriteGroup = 0x0020, ExeGroup = 0x0010,
		ReadOther = 0x0004, WriteOther = 0x0002, ExeOther = 0x0001
	};
    
public:
	CFile(const CString& fileName = CString());
	virtual ~CFile();
    
	// return the type of the device
	DeviceType deviceType() const;
    
// parameters of file
	void setFileName(const CString& fileName);
	CString fileName();
    
// open/close
	virtual bool open(opuint32 mode);
	virtual void close();
	virtual bool isOpen() const;
	bool remove();
	bool rename(const CString& newName);
    
// io
	int write(const void* data, int len);
	int write(const CByteArray& data);
	int read(void* data, int len) const;
	CByteArray readAll();
	void flush();
    
//
	int seek(opint64 pos);
	opuint64 pos() const;
	opuint64 size() const;
    
private:
	CString    m_fileName;
	FILE     * m_file;
	opuint32   m_openFlags;
    
#ifdef _WIN32
	void* m_hFile;
	void* m_hMap;
#endif
};

#endif /* defined(__gcs_osx__cfile__) */
