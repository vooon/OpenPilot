//
//  uavobjectfield.h
//  gcs_osx
//
//  Created by Vova Starikh on 2/10/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#ifndef __gcs_osx__uavobjectfield__
#define __gcs_osx__uavobjectfield__

#include "OPVector"
#include "OPMap"
#include "core/CVariant"
#include "signals/Signals"

class UAVObject;

class UAVObjectField
{
public:
    enum FieldType{
        INT8 = 0,
        INT16,
        INT32,
        UINT8,
        UINT16,
        UINT32,
        FLOAT32,
        ENUM,
        BITFIELD,
        STRING
    };
    
    enum LimitType {
        EQUAL,
        NOT_EQUAL,
        BETWEEN,
        BIGGER,
        SMALLER
    };
    typedef struct
    {
        LimitType type;
        opvector<CVariant> values;
        int board;
    } LimitStruct;
    
    UAVObjectField(const CString& name, const CString& units, FieldType type, opuint32 numElements, const CStringList& options,const CString& limits=CString());
    UAVObjectField(const CString& name, const CString& units, FieldType type, const CStringList& elementNames, const CStringList& options,const CString& limits=CString());
    
    void initialize(opuint8* data, opuint32 dataOffset, UAVObject* obj);
    UAVObject* getObject();
    FieldType getType();
    CString getTypeAsString();
    CString getName();
    CString getUnits();
    opuint32 getNumElements();
    CStringList getElementNames();
    CStringList getOptions();
    opint32 pack(opuint8* dataOut);
    opint32 unpack(const opuint8* dataIn);
    CVariant getValue(opuint32 index = 0);
    bool checkValue(const CVariant& data, opuint32 index = 0);
    void setValue(const CVariant& data, opuint32 index = 0);
    double getDouble(opuint32 index = 0);
    void setDouble(double value, opuint32 index = 0);
    opuint32 getDataOffset();
    opuint32 getNumBytes();
    bool isNumeric();
    bool isText();
    CString toString();
    
    bool isWithinLimits(CVariant var, opuint32 index, int board=0);
    CVariant getMaxLimit(opuint32 index, int board=0);
    CVariant getMinLimit(opuint32 index, int board=0);

// signals
    CL_Signal_v1<UAVObjectField*>& signal_fieldUpdated();
    
protected:
    CString       m_name;
    CString       m_units;
    FieldType     m_type;
    CStringList   m_elementNames;
    CStringList   m_options;
    opuint32      m_numElements;
    opuint32      m_numBytesPerElement;
    opuint32      m_offset;
    opuint8     * m_data;
    UAVObject   * m_obj;
    opmap<opuint32, opvector<LimitStruct> > m_elementLimits;
    
    CL_Signal_v1<UAVObjectField*> m_signalFieldUpdated;
    
    void clear();
    void constructorInitialize(const CString& name, const CString& units, FieldType type, const CStringList& elementNames, const CStringList& options, const CString &limits);
    void limitsInitialize(const CString &limits);
};

#endif /* defined(__gcs_osx__uavobjectfield__) */
