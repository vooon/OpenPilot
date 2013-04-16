/**
 ******************************************************************************
 *
 * @file       uavdataobject.h
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

#ifndef __gcs_osx__uavdataobject__
#define __gcs_osx__uavdataobject__

#include "uavobject.h"

class CString;

class UAVMetaObject;

class UAVDataObject : public UAVObject
{
public:
    UAVDataObject(opuint32 objID, bool isSingleInst, bool isSet, const CString& name);

    void initialize(opuint32 instID, UAVMetaObject* mobj);
    void initialize(UAVMetaObject* mobj);
    bool isSettings();
    void setMetadata(const Metadata& mdata);
    Metadata getMetadata();
    UAVMetaObject* getMetaObject();
    virtual UAVDataObject* clone(opuint32 instID = 0) = 0;
    virtual UAVDataObject* dirtyClone() = 0;

private:
    UAVMetaObject * m_mobj;
    bool            m_isSet;
};

#endif /* defined(__gcs_osx__uavdataobject__) */
