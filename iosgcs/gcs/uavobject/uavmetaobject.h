/**
 ******************************************************************************
 *
 * @file       uavmetaobject.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectsPlugin UAVObjects Plugin
 * @{
 * @brief      The UAVUObjects GCS plugin
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __gcs_osx__uavmetaobject__
#define __gcs_osx__uavmetaobject__

#include "uavobject.h"

class UAVMetaObject : public UAVObject
{
public:
    UAVMetaObject(opuint32 objID, const CString& name, UAVObject* parent);
    
    UAVObject* getParentObject();
    void setMetadata(const Metadata& mdata);
    Metadata getMetadata();
    Metadata getDefaultMetadata();
    void setData(const Metadata& mdata);
    Metadata getData();
    
private:
    UAVObject * m_parent;
    Metadata    m_ownMetadata;
    Metadata    m_parentMetadata;
};


#endif /* defined(__gcs_osx__uavmetaobject__) */
