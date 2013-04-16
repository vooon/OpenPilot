//
//  uavobjectfield.cpp
//  gcs_osx
//
//  Created by Vova Starikh on 2/10/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#include "uavobjectfield.h"
#include "uavobject.h"
#include "Logger"
#include <assert.h>

UAVObjectField::UAVObjectField(const CString& name, const CString& units, FieldType type, opuint32 numElements, const CStringList& options,const CString& limits/*=CString()*/)
{
    CStringList elementNames;
    // Set element names
    for (opuint32 n = 0; n < numElements; ++n)
    {
        elementNames.push_back(CString("%1").arg(n));
    }
    // Initialize
    constructorInitialize(name, units, type, elementNames, options,limits);
}

UAVObjectField::UAVObjectField(const CString& name, const CString& units, FieldType type, const CStringList& elementNames, const CStringList& options,const CString& limits/*=CString()*/)
{
    constructorInitialize(name, units, type, elementNames, options,limits);
}

void UAVObjectField::initialize(opuint8* data, opuint32 dataOffset, UAVObject* obj)
{
    m_data   = data;
    m_offset = dataOffset;
    m_obj    = obj;
    clear();
}

UAVObject* UAVObjectField::getObject()
{
    return m_obj;
}

UAVObjectField::FieldType UAVObjectField::getType()
{
    return m_type;
}

CString UAVObjectField::getTypeAsString()
{
    switch (m_type)
    {
        case UAVObjectField::INT8:
            return "int8";
        case UAVObjectField::INT16:
            return "int16";
        case UAVObjectField::INT32:
            return "int32";
        case UAVObjectField::UINT8:
            return "uint8";
        case UAVObjectField::UINT16:
            return "uint16";
        case UAVObjectField::UINT32:
            return "uint32";
        case UAVObjectField::FLOAT32:
            return "float32";
        case UAVObjectField::ENUM:
            return "enum";
        case UAVObjectField::BITFIELD:
            return "bitfield";
        case UAVObjectField::STRING:
            return "string";
        default:
            return "";
    }
}

CString UAVObjectField::getName()
{
    return m_name;
}

CString UAVObjectField::getUnits()
{
    return m_units;
}

opuint32 UAVObjectField::getNumElements()
{
    return m_numElements;
}

CStringList UAVObjectField::getElementNames()
{
    return m_elementNames;
}

CStringList UAVObjectField::getOptions()
{
    return m_options;
}

opint32 UAVObjectField::pack(opuint8* dataOut)
{
    CMutexSection locker(m_obj->getMutex());
    // Pack each element in output buffer
    switch (m_type)
    {
        case INT8:
            memcpy(dataOut, &m_data[m_offset], m_numElements);
            break;
        case INT16:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&dataOut[m_numBytesPerElement*index], &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case INT32:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&dataOut[m_numBytesPerElement*index], &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case UINT8:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                dataOut[m_numBytesPerElement*index] = m_data[m_offset + m_numBytesPerElement*index];
            }
            break;
        case UINT16:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&dataOut[m_numBytesPerElement*index], &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case UINT32:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&dataOut[m_numBytesPerElement*index], &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case FLOAT32:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&dataOut[m_numBytesPerElement*index], &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case ENUM:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                dataOut[m_numBytesPerElement*index] = m_data[m_offset + m_numBytesPerElement*index];
            }
            break;
        case BITFIELD:
            for (opuint32 index = 0; index < (opuint32)(1+(m_numElements-1)/8); ++index)
            {
                dataOut[m_numBytesPerElement*index] = m_data[m_offset + m_numBytesPerElement*index];
            }
            break;
        case STRING:
            memcpy(dataOut, &m_data[m_offset], m_numElements);
            break;
    }
    // Done
    return getNumBytes();
}

opint32 UAVObjectField::unpack(const opuint8* dataIn)
{
    CMutexSection locker(m_obj->getMutex());
    // Unpack each element from input buffer
    switch (m_type)
    {
        case INT8:
            memcpy(&m_data[m_offset], dataIn, m_numElements);
            break;
        case INT16:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &dataIn[m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case INT32:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &dataIn[m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case UINT8:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                m_data[m_offset + m_numBytesPerElement*index] = dataIn[m_numBytesPerElement*index];
            }
            break;
        case UINT16:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &dataIn[m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case UINT32:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &dataIn[m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case FLOAT32:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &dataIn[m_numBytesPerElement*index], m_numBytesPerElement);
            }
            break;
        case ENUM:
            for (opuint32 index = 0; index < m_numElements; ++index)
            {
                m_data[m_offset + m_numBytesPerElement*index] = dataIn[m_numBytesPerElement*index];
            }
            break;
        case BITFIELD:
            for (opuint32 index = 0; index < (opuint32)(1+(m_numElements-1)/8); ++index)
            {
                m_data[m_offset + m_numBytesPerElement*index] = dataIn[m_numBytesPerElement*index];
            }
            break;
        case STRING:
            memcpy(&m_data[m_offset], dataIn, m_numElements);
            break;
    }
    // Done
    return getNumBytes();
}

CVariant UAVObjectField::getValue(opuint32 index /*= 0*/)
{
    CMutexSection locker(m_obj->getMutex());
    // Check that index is not out of bounds
    if ( index >= m_numElements )
    {
        return CVariant();
    }
    // Get value
    switch (m_type)
    {
        case INT8:
        {
            opint8 tmpint8;
            memcpy(&tmpint8, &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            return CVariant(tmpint8);
        }
        case INT16:
        {
            opint16 tmpint16;
            memcpy(&tmpint16, &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            return CVariant(tmpint16);
        }
        case INT32:
        {
            opint32 tmpint32;
            memcpy(&tmpint32, &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            return CVariant(tmpint32);
        }
        case UINT8:
        {
            opuint8 tmpuint8;
            memcpy(&tmpuint8, &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            return CVariant(tmpuint8);
        }
        case UINT16:
        {
            opuint16 tmpuint16;
            memcpy(&tmpuint16, &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            return CVariant(tmpuint16);
        }
        case UINT32:
        {
            opuint32 tmpuint32;
            memcpy(&tmpuint32, &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            return CVariant(tmpuint32);
        }
        case FLOAT32:
        {
            float tmpfloat;
            memcpy(&tmpfloat, &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
            return CVariant(tmpfloat);
        }
        case ENUM:
        {
            opuint8 tmpenum;
            memcpy(&tmpenum, &m_data[m_offset + m_numBytesPerElement*index], m_numBytesPerElement);
//            Q_ASSERT((tmpenum < options.length()) && (tmpenum >= 0)); // catch bad enum settings
            if (tmpenum >= m_options.size()) {
//                Logger() << "Invalid value for" << m_name;
                return CVariant(CString("Bad Value"));
            }
            return CVariant(m_options.at(tmpenum) );
        }
        case BITFIELD:
        {
            opuint8 tmpbitfield;
            memcpy(&tmpbitfield, &m_data[m_offset + m_numBytesPerElement*((opuint32)(index/8))], m_numBytesPerElement);
            tmpbitfield = (tmpbitfield >> (index % 8)) & 1;
            return CVariant(tmpbitfield);
        }
        case STRING:
        {
            m_data[m_offset + m_numElements - 1] = '\0';
            CString str((char*)&m_data[m_offset]);
            return CVariant( str );
        }
    }
    // If this point is reached then we got an invalid type
    return CVariant();
}

bool UAVObjectField::checkValue(const CVariant& value, opuint32 index /*= 0*/)
{
    CMutexSection locker(m_obj->getMutex());
    // Check that index is not out of bounds
    if (index >= m_numElements)
    {
        return false;
    }
    // Get metadata
    UAVObject::Metadata mdata = m_obj->getMetadata();
    // Update value if the access mode permits
    if (UAVObject::GetFlightAccess(mdata) == UAVObject::ACCESS_READWRITE)
    {
        switch (m_type)
        {
            case INT8:
            case INT16:
            case INT32:
            case UINT8:
            case UINT16:
            case UINT32:
            case FLOAT32:
            case STRING:
            case BITFIELD:
                return true;
            case ENUM:
            {
                opint8 tmpenum = m_options.indexOf(value.toString());
                return ((tmpenum < 0) ? false : true);
            }
            default:
//                Logger() << "checkValue: other types" << type;
                assert(0); // To catch any programming errors where we tried to test invalid values
                break;
        }
    }
    return true;
}

void UAVObjectField::setValue(const CVariant& value, opuint32 index /*= 0*/)
{
    CMutexSection locker(m_obj->getMutex());
    // Check that index is not out of bounds
    if ( index >= m_numElements )
    {
        return;
    }
    // Get metadata
    UAVObject::Metadata mdata = m_obj->getMetadata();
    // Update value if the access mode permits
    if ( UAVObject::GetGcsAccess(mdata) == UAVObject::ACCESS_READWRITE )
    {
        switch (m_type)
        {
            case INT8:
            {
                opint8 tmpint8 = value.toInt();
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &tmpint8, m_numBytesPerElement);
                break;
            }
            case INT16:
            {
                opint16 tmpint16 = value.toInt();
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &tmpint16, m_numBytesPerElement);
                break;
            }
            case INT32:
            {
                opint32 tmpint32 = opint32(value.toInt());
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &tmpint32, m_numBytesPerElement);
                break;
            }
            case UINT8:
            {
                opuint8 tmpuint8 = value.toUInt();
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &tmpuint8, m_numBytesPerElement);
                break;
            }
            case UINT16:
            {
                opuint16 tmpuint16 = value.toUInt();
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &tmpuint16, m_numBytesPerElement);
                break;
            }
            case UINT32:
            {
                opuint32 tmpuint32 = opint32(value.toUInt());
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &tmpuint32, m_numBytesPerElement);
                break;
            }
            case FLOAT32:
            {
                float tmpfloat = float (value.toDouble());
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &tmpfloat, m_numBytesPerElement);
                break;
            }
            case ENUM:
            {
                opint8 tmpenum = m_options.indexOf(value.toString());
                assert(tmpenum >= 0); // To catch any programming errors where we set invalid values
                memcpy(&m_data[m_offset + m_numBytesPerElement*index], &tmpenum, m_numBytesPerElement);
                break;
            }
            case BITFIELD:
            {
                opuint8 tmpbitfield;
                memcpy(&tmpbitfield, &m_data[m_offset + m_numBytesPerElement*((opuint32)(index/8))], m_numBytesPerElement);
                tmpbitfield = (tmpbitfield & ~(1 << (index % 8))) | ( (value.toUInt()!=0?1:0) << (index % 8) );
                memcpy(&m_data[m_offset + m_numBytesPerElement*((opuint32)(index/8))], &tmpbitfield, m_numBytesPerElement);
                break;
            }
            case STRING:
            {
                CString str = value.toString();
                assert(int (m_numElements*m_numBytesPerElement) < str.size());
                memcpy(m_data+m_offset, str.data(), str.size());
                m_data[m_offset+str.size()] = '\0';
                break;
            }
        }
    }
}

double UAVObjectField::getDouble(opuint32 index /*= 0*/)
{
   return getValue(index).toDouble();
}

void UAVObjectField::setDouble(double value, opuint32 index /*= 0*/)
{
    setValue(CVariant(value), index);
}

opuint32 UAVObjectField::getDataOffset()
{
    return m_offset;
}

opuint32 UAVObjectField::getNumBytes()
{
    switch (m_type)
    {
        case BITFIELD:
            return m_numBytesPerElement * ((opuint32) (1+(m_numElements-1)/8));
            break;
        default:
            return m_numBytesPerElement * m_numElements;
            break;
    }
}

bool UAVObjectField::isNumeric()
{
    switch (m_type)
    {
        case INT8:
            return true;
        case INT16:
            return true;
        case INT32:
            return true;
        case UINT8:
            return true;
        case UINT16:
            return true;
        case UINT32:
            return true;
        case FLOAT32:
            return true;
        case ENUM:
            return false;
        case BITFIELD:
            return true;
        case STRING:
            return false;
        default:
            return false;
    }
}

bool UAVObjectField::isText()
{
    switch (m_type)
    {
        case INT8:
            return false;
        case INT16:
            return false;
        case INT32:
            return false;
        case UINT8:
            return false;
        case UINT16:
            return false;
        case UINT32:
            return false;
        case FLOAT32:
            return false;
        case ENUM:
            return true;
        case BITFIELD:
            return false;
        case STRING:
            return true;
        default:
            return false;
    }
}

CString UAVObjectField::toString()
{
    CString sout;
    sout += CString("%1: [ ").arg(m_name);
    for (unsigned int n = 0; n < m_numElements; ++n)
    {
        CVariant value = getValue(n);
        if (value.type() == CVariant::String) {
            sout += CString("%1 ").arg(value.toString());
        } else {
            sout += CString("%1 ").arg(value.toDouble(false));
        }
    }
    sout += CString("] %1\n").arg(m_units);
    return sout;
}

bool UAVObjectField::isWithinLimits(CVariant var, opuint32 index, int board/*=0*/)
{
    if (!m_elementLimits.count(index))
        return true;
    const opvector<LimitStruct>& limits = m_elementLimits[index];
    for (opvector<LimitStruct>::const_iterator it = limits.begin(); it<limits.end();it++) {
        const LimitStruct& struc = *it;
        if((struc.board!=board) && board!=0 && struc.board!=0)
            continue;
        switch(struc.type)
        {
            case EQUAL:
                switch (m_type)
            {
                case INT8:
                case INT16:
                case INT32:
                    for (opvector<CVariant>::const_iterator it = struc.values.begin();it<struc.values.end();it++) {
                        if (var.toInt() == (*it).toInt())
                            return true;
                    }
                    return false;
                    break;
                case UINT8:
                case UINT16:
                case UINT32:
                case BITFIELD:
                    for (opvector<CVariant>::const_iterator it = struc.values.begin();it<struc.values.end();it++) {
                        if (var.toUInt() == (*it).toUInt())
                            return true;
                    }
                    return false;
                    break;
                case ENUM:
                case STRING:
                    for (opvector<CVariant>::const_iterator it = struc.values.begin();it<struc.values.end();it++) {
                        if(var.toString() == (*it).toString())
                            return true;
                    }
                    return false;
                    break;
                case FLOAT32:
                    for (opvector<CVariant>::const_iterator it = struc.values.begin();it<struc.values.end();it++) {
                        if(var.toDouble() == (*it).toDouble())
                            return true;
                    }
                    return false;
                    break;
                default:
                    return true;
            }
                break;
            case NOT_EQUAL:
                switch (m_type)
            {
                case INT8:
                case INT16:
                case INT32:
                    for (opvector<CVariant>::const_iterator it = struc.values.begin();it<struc.values.end();it++) {
                        if (var.toInt() == (*it).toInt())
                            return false;
                    }
                    return true;
                    break;
                case UINT8:
                case UINT16:
                case UINT32:
                case BITFIELD:
                    for (opvector<CVariant>::const_iterator it = struc.values.begin();it<struc.values.end();it++) {
                        if (var.toUInt() == (*it).toUInt())
                            return false;
                    }
                    return true;
                    break;
                case ENUM:
                case STRING:
                    for (opvector<CVariant>::const_iterator it = struc.values.begin();it<struc.values.end();it++) {
                        if(var.toString() == (*it).toString())
                            return false;
                    }
                    return true;
                    break;
                case FLOAT32:
                    for (opvector<CVariant>::const_iterator it = struc.values.begin();it<struc.values.end();it++) {
                        if(var.toDouble() == (*it).toDouble())
                            return false;
                    }
                    return true;
                    break;
                default:
                    return true;
            }
                break;
            case BETWEEN:
                if (struc.values.size()<2)
                {
//                    Logger() << __FUNCTION__ << "between limit with less than 1 pair, aborting; field:" << m_name;
                    return true;
                }
                if(struc.values.size()>2) {
//                    Logger() << __FUNCTION__ << "between limit with more than 1 pair, using first; field" << m_name;
                }
                switch (m_type)
            {
                case INT8:
                case INT16:
                case INT32:
                    if(!(var.toInt()>=struc.values.at(0).toInt() && var.toInt()<=struc.values.at(1).toInt()))
                        return false;
                    return true;
                    break;
                case UINT8:
                case UINT16:
                case UINT32:
                case BITFIELD:
                    if(!(var.toUInt()>=struc.values.at(0).toUInt() && var.toUInt()<=struc.values.at(1).toUInt()))
                        return false;
                    return true;
                    break;
                case ENUM:
                    if (!(m_options.indexOf(var.toString())>=m_options.indexOf(struc.values.at(0).toString()) && m_options.indexOf(var.toString())<=m_options.indexOf(struc.values.at(1).toString())))
                        return false;
                    return true;
                    break;
                case STRING:
                    return true;
                    break;
                case FLOAT32:
                    if(!(var.toDouble()>=struc.values.at(0).toDouble() && var.toDouble()<=struc.values.at(1).toDouble()))
                        return false;
                    return true;
                    break;
                default:
                    return true;
            }
                break;
            case BIGGER:
                if(struc.values.size()<1)
                {
//                    Logger()<<__FUNCTION__<<"BIGGER limit with less than 1 value, aborting; field:"<<m_name;
                    return true;
                }
                if(struc.values.size()>1) {
//                    Logger()<<__FUNCTION__<<"BIGGER limit with more than 1 value, using first; field"<<name;
                }
                switch (m_type)
            {
                case INT8:
                case INT16:
                case INT32:
                    if(!(var.toInt()>=struc.values.at(0).toInt()))
                        return false;
                    return true;
                    break;
                case UINT8:
                case UINT16:
                case UINT32:
                case BITFIELD:
                    if(!(var.toUInt()>=struc.values.at(0).toUInt()))
                        return false;
                    return true;
                    break;
                case ENUM:
                    if(!(m_options.indexOf(var.toString())>=m_options.indexOf(struc.values.at(0).toString())))
                        return false;
                    return true;
                    break;
                case STRING:
                    return true;
                    break;
                case FLOAT32:
                    if(!(var.toDouble()>=struc.values.at(0).toDouble()))
                        return false;
                    return true;
                    break;
                default:
                    return true;
            }
                break;
            case SMALLER:
                switch (m_type)
            {
                case INT8:
                case INT16:
                case INT32:
                    if(!(var.toInt()<=struc.values.at(0).toInt()))
                        return false;
                    return true;
                case UINT8:
                case UINT16:
                case UINT32:
                case BITFIELD:
                    if(!(var.toUInt()<=struc.values.at(0).toUInt()))
                        return false;
                    return true;
                case ENUM:
                    if(!(m_options.indexOf(var.toString())<=m_options.indexOf(struc.values.at(0).toString())))
                        return false;
                    return true;
                case STRING:
                    return true;
                case FLOAT32:
                    if(!(var.toDouble()<=struc.values.at(0).toDouble()))
                        return false;
                    return true;
                default:
                    return true;
            }
        }
    }
    return true;
}

CVariant UAVObjectField::getMaxLimit(opuint32 index, int board/*=0*/)
{
    if (!m_elementLimits.count(index))
        return CVariant();
    const opvector<LimitStruct>& limits = m_elementLimits[index];
    for (opvector<LimitStruct>::const_iterator it = limits.begin(); it<limits.end();it++) {
        const LimitStruct& struc = *it;
        if((struc.board!=board) && board!=0 && struc.board!=0)
            continue;
        switch(struc.type)
        {
            case EQUAL:
            case NOT_EQUAL:
            case BIGGER:
                return CVariant();
                break;
                break;
            case BETWEEN:
                return struc.values.at(1);
                break;
            case SMALLER:
                return struc.values.at(0);
                break;
            default:
                return CVariant();
                break;
        }
    }
    return CVariant();
}

CVariant UAVObjectField::getMinLimit(opuint32 index, int board/*=0*/)
{
    if (!m_elementLimits.count(index))
        return CVariant();
    const opvector<LimitStruct>& limits = m_elementLimits[index];
    for (opvector<LimitStruct>::const_iterator it = limits.begin(); it<limits.end();it++) {
        const LimitStruct& struc = *it;
        if((struc.board!=board) && board!=0 && struc.board!=0)
            return CVariant();
        switch(struc.type)
        {
            case EQUAL:
            case NOT_EQUAL:
            case SMALLER:
                return CVariant();
                break;
                break;
            case BETWEEN:
                return struc.values.at(0);
                break;
            case BIGGER:
                return struc.values.at(0);
                break;
            default:
                return CVariant();
                break;
        }
    }
    return CVariant();
}

/////////////////////////////////////////////////////////////////////
//                             signals                             //
/////////////////////////////////////////////////////////////////////

CL_Signal_v1<UAVObjectField*>& UAVObjectField::signal_fieldUpdated()
{
    return m_signalFieldUpdated;
}

//===================================================================
//  p r o t e c t e d   f u n c t i o n s
//===================================================================

void UAVObjectField::clear()
{
}

void UAVObjectField::constructorInitialize(const CString& name, const CString& units, FieldType type, const CStringList& elementNames, const CStringList& options, const CString &limits)
{
    // Copy params
    m_name = name;
    m_units = units;
    m_type = type;
    m_options = options;
    m_numElements = int (elementNames.size());
    m_offset = 0;
    m_data = NULL;
    m_obj = NULL;
    m_elementNames = elementNames;
    // Set field size
    switch (type)
    {
        case INT8:
            m_numBytesPerElement = sizeof(opint8);
            break;
        case INT16:
            m_numBytesPerElement = sizeof(opint16);
            break;
        case INT32:
            m_numBytesPerElement = sizeof(opint32);
            break;
        case UINT8:
            m_numBytesPerElement = sizeof(opuint8);
            break;
        case UINT16:
            m_numBytesPerElement = sizeof(opuint16);
            break;
        case UINT32:
            m_numBytesPerElement = sizeof(opuint32);
            break;
        case FLOAT32:
            m_numBytesPerElement = sizeof(opuint32);
            break;
        case ENUM:
            m_numBytesPerElement = sizeof(opuint8);
            break;
        case BITFIELD:
            m_numBytesPerElement = sizeof(opuint8);
            m_options = CStringList()<<CString("0")<<CString("1");
            break;
        case STRING:
            m_numBytesPerElement = sizeof(opuint8);
            break;
        default:
            m_numBytesPerElement = 0;
    }
    limitsInitialize(limits);
}

void UAVObjectField::limitsInitialize(const CString &limits)
{
    /// format
    /// (TY)->type (EQ-equal;NE-not equal;BE-between;BI-bigger;SM-smaller)
    /// (VALX)->value
    /// %TY:VAL1:VAL2:VAL3,%TY,VAL1,VAL2,VAL3
    /// example: first element bigger than 3 and second element inside [2.3,5]
    /// "%BI:3,%BE:2.3:5"
    if (limits.isEmpty())
        return;
    CStringList stringPerElement = CStringList::splitString(limits, ',');
    opuint32 index=0;
    for (CStringList::const_iterator it = stringPerElement.begin();it!=stringPerElement.end();it++)
    {
        CString str = *it;
        CStringList ruleList = CStringList::splitString(str, ';');
        opvector<LimitStruct> limitList;
        for (CStringList::const_iterator it_rule = ruleList.begin();it_rule!=ruleList.end();it_rule++)
        {
            CString rule = *it_rule;
            
            CString _str=rule.trimmed();
            if(_str.isEmpty())
                continue;
            CStringList valuesPerElement = CStringList::splitString(_str, ':');
            LimitStruct lstruc;
            bool startFlag = valuesPerElement.at(0).startsWith("%");
            bool maxIndexFlag = (int)(index)<(int)m_numElements;
            bool elemNumberSizeFlag = valuesPerElement.at(0).length()==3;
            bool aux;
            valuesPerElement.at(0).mid(1,4).toInt64(&aux,16);
            bool b4 = ((valuesPerElement.at(0).size())==7 && aux);
            if(startFlag && maxIndexFlag && (elemNumberSizeFlag || b4))
            {
                if(b4)
                    lstruc.board = int (valuesPerElement.at(0).mid(1,4).toInt64(&aux,16));
                else
                    lstruc.board=0;
                if(valuesPerElement.at(0).right(2)=="EQ")
                    lstruc.type=EQUAL;
                else if(valuesPerElement.at(0).right(2)=="NE")
                    lstruc.type=NOT_EQUAL;
                else if(valuesPerElement.at(0).right(2)=="BE")
                    lstruc.type=BETWEEN;
                else if(valuesPerElement.at(0).right(2)=="BI")
                    lstruc.type=BIGGER;
                else if(valuesPerElement.at(0).right(2)=="SM")
                    lstruc.type=SMALLER;
                else
                    Logger()<<"limits parsing failed (invalid property) on UAVObjectField" << m_name;
                valuesPerElement.erase(valuesPerElement.begin());
                for (CStringList::const_iterator it_vpr= valuesPerElement.begin();it_vpr!=valuesPerElement.end();it_vpr++)
                {
                    CString _value = *it_vpr;
                    CString value = _value.trimmed();
                    switch (m_type)
                    {
                        case UINT8:
                        case UINT16:
                        case UINT32:
                        case BITFIELD:
                            lstruc.values.push_back((opuint32)value.toUInt64());
                            break;
                        case INT8:
                        case INT16:
                        case INT32:
                            lstruc.values.push_back((opint32)value.toInt64());
                            break;
                        case FLOAT32:
                            lstruc.values.push_back((float)value.toDouble());
                            break;
                        case ENUM:
                            lstruc.values.push_back((CString)value);
                            break;
                        case STRING:
                            lstruc.values.push_back((CString)value);
                            break;
                        default:
                            lstruc.values.push_back(CVariant());
                    }
                }
                limitList.push_back(lstruc);
            }
            else
            {
//                if(!valuesPerElement.at(0).isEmpty() && !startFlag)
//                    Logger() << "limits parsing failed (property doesn't start with %) on UAVObjectField"<<m_name;
//                else if(!maxIndexFlag)
//                    Logger() << "limits parsing failed (index>numelements) on UAVObjectField"<<m_name<<"index"<<m_index<<"numElements"<<m_numElements;
//                else if(!elemNumberSizeFlag || !b4 )
//                    Logger() << "limits parsing failed limit not starting with %XX or %YYYYXX where XX is the limit type and YYYY is the board type on UAVObjectField"<<name;
            }
        }
        m_elementLimits.insert(std::make_pair(index,limitList));
        ++index;
    }
//    foreach(QList<LimitStruct> limitList,elementLimits)
//    {
//        foreach(LimitStruct limit,limitList)
//        {
//            qDebug()<<"Limit type"<<limit.type<<"for board"<<limit.board<<"for field"<<getName();
//            foreach(QVariant var,limit.values)
//            {
//                qDebug()<<"value"<<var;
//            }
//        }
//    }
}
