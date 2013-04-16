//
//  cloggeros_ios.cpp
//  os.ios
//
//  Created by Vladiamir Starikh on 25.06.12.
//  Copyright (c) 2012 HeliconSoft. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "cloggeros_ios.h"

CLoggerOs_iOs::CLoggerOs_iOs() :
	CLoggerOs()
{
}

CLoggerOs_iOs::~CLoggerOs_iOs()
{
	
}

/////////////////////////////////////////////////////////////////////
//                            запись в лог                         //
/////////////////////////////////////////////////////////////////////

// записывает строку в лог
void CLoggerOs_iOs::writeString( const CString& text )
{
	NSLog( @"%s", text.data() );
	CLoggerOs::writeString( text );
}
