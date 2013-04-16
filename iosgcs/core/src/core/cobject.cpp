#include "cobject.h"

CObject::CObject()
{
    m_refs = 0;
}

CObject::~CObject()
{
    
}

opuint32 CObject::addRef()
{
    return ++m_refs;
}

opuint32 CObject::release()
{
	if( !m_refs )
		return 0;
	m_refs--;
	if( !m_refs )
	{
		selfDelete();
		return 0;
	}
	return m_refs;
}

opuint32 CObject::refs()
{
    return m_refs;
}

//===================================================================
//   p r o t e c t e d   f u n c t i o n s
//===================================================================

void CObject::selfDelete()
{
    delete this;
}