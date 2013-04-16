//
//  cloggeros_ios.h
//  os.ios
//
//  Created by Vladiamir Starikh on 25.06.12.
//  Copyright (c) 2012 HeliconSoft. All rights reserved.
//

#ifndef os_ios_cloggeros_ios_h
#define os_ios_cloggeros_ios_h

#include "cloggeros.h"

class CLoggerOs_iOs : public CLoggerOs
{
public:
	CLoggerOs_iOs();
	virtual ~CLoggerOs_iOs();
	
// запись в лог
	// записывает строку в лог
	virtual void writeString( const CString& text );

private:
};

#endif
