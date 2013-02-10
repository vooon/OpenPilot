//
//  opvector.h
//  gcs_osx
//
//  Created by Vova Starikh on 2/9/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#ifndef gcs_osx_opvector_h
#define gcs_osx_opvector_h


#include <vector>

using namespace std;

template< typename _Tp >
class opvector : public vector<_Tp, allocator<_Tp> >
{
public:
};


#endif
