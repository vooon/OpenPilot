//
//  oplist.h
//  gcs_osx
//
//  Created by Vova Starikh on 2/9/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#ifndef gcs_osx_oplist_h
#define gcs_osx_oplist_h

#include <list>


using namespace std;

template< typename _Tp >
class oplist : public list<_Tp>
{
public:
	_Tp takeFirst() {
		_Tp ret = this->front();
		this->pop_front();
		return ret;
	}
    
	oplist<_Tp>& operator<<( const _Tp& val ) {
		push_back( val );
		return *this;
	}
};

#endif
