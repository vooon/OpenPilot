//
//  CException.h
//  OpenPilotGCS_osx
//
//  Created by Vova Starikh on 1/21/13.
//  Copyright (c) 2013 OpenPilot. All rights reserved.
//

#ifndef __OpenPilotGCS_osx__CException__
#define __OpenPilotGCS_osx__CException__

#include "../text/cstring.h"

class CException
{
public:
	CException(int error, const CString& message);
	~CException();

	virtual const CString& message() const;
    int error() const;
	
private:
    int     m_error;
	CString m_message;
};

#endif /* defined(__OpenPilotGCS_osx__CException__) */
