//
//  CException.cpp
//  OpenPilotGCS_osx
//
//  Created by Vova Starikh on 1/21/13.
//  Copyright (c) 2013 OpenPilot. All rights reserved.
//

#include "cexception.h"

CException::CException(int error, const CString& message)
{
    m_error   = error;
	m_message = message;
}

CException::~CException()
{
	
}

const CString& CException::message() const
{
	return m_message;
}

int CException::error() const
{
    return m_error;
}