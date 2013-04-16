#ifndef COBJECT_H
#define COBJECT_H

#include "../optypes.h"

class CObject
{
public:
    CObject();
    virtual ~CObject();
    
	virtual opuint32 addRef();
	virtual opuint32 release();
	opuint32 refs();
    
protected:
    opuint32 m_refs;
    
    virtual void selfDelete();
};

#endif // COBJECT_H
