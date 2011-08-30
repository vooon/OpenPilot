/**
 ******************************************************************************
 *
 * @file       $(NAMELC).cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 *   
 * @note       Object definition file: $(XMLFILE). 
 *             This is an automatically generated file.
 *             DO NOT modify manually.
 *
 * @brief      The UAVUObjects Obj-C framework
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
#import "$(NAMELC).h"
#import "uavobjectfield.h"

@implementation $(NAME)

/**
 * Constructor
 */
-(id) init 
{
    self = [super initWithID:$(OBJIDHEX) andSingleInst:$(ISSINGLEINSTYN) andIsSet:$(ISSETTINGSYN) andName:@"$(NAME)"];
    
    // Create fields
    NSMutableArray* tmpFields = [NSMutableArray arrayWithCapacity:$(NUMFIELDS)];

$(FIELDSINIT)

    fields = [NSArray arrayWithArray:tmpFields];
    [fields retain];
    
    // Initialize object
    [super initializeFields:fields withData:(uint8_t*)&objectData andLength:sizeof($(NAMELC)_DataFields)];
    
    // Set the default field values
    [self setDefaultFieldValues];
    
    // Set the object description
    self.description = @"$(DESCRIPTION)";
    
    return self;

}

/**
 * Get the default metadata for this object
 */
-(Metadata) getDefaultMetadata
{
    Metadata metadata;
    
    metadata.flightAccess                   = $(FLIGHTACCESS);
    metadata.gcsAccess                      = $(GCSACCESS);
    metadata.gcsTelemetryAcked              = $(GCSTELEM_ACKED);
    metadata.gcsTelemetryUpdateMode         = $(GCSTELEM_UPDATEMODE);
    metadata.gcsTelemetryUpdatePeriod       = $(GCSTELEM_UPDATEPERIOD);
    metadata.flightTelemetryAcked           = $(FLIGHTTELEM_ACKED);
    metadata.flightTelemetryUpdateMode      = $(FLIGHTTELEM_UPDATEMODE);
    metadata.flightTelemetryUpdatePeriod    = $(FLIGHTTELEM_UPDATEPERIOD);
    metadata.loggingUpdateMode              = $(LOGGING_UPDATEMODE);
    metadata.loggingUpdatePeriod            = $(LOGGING_UPDATEPERIOD);
    
    return metadata;
}

/**
 * Initialize object fields with the default values.
 * If a default value is not specified the object fields
 * will be initialized to zero.
 */
-(void) setDefaultFieldValues
{
$(INITFIELDS)
}

/**
 * Get the object data fields
 */
-($(NAMELC)_DataFields) getData
{
    return *($(NAMELC)_DataFields*)data;
}

/**
 * Set the object data fields
 */
-(void) setData:($(NAMELC)_DataFields) newData
{
    // Get metadata
    if ( self.metadata.gcsAccess == ACCESS_READWRITE )
    {
        //data = (uint8_t*)&newData;
        if (data)
        {
            memcpy(data, &newData, self.numBytes);
        }
    }
}

/**
 * Create a clone of this object, a new instance ID must be specified.
 * Do not use this function directly to create new instances, the
 * UAVObjectManager should be used instead.
 */
-(UAVDataObject*) clone:(uint32_t) newInstID
{
    $(NAME)* obj = [[[$(NAME) alloc]init]autorelease];
    [obj initializeWithInstId:newInstID andMetaObject:self.MetaObject];
    return obj;
}

/**
 * Static function to retrieve an instance of the object.
 */
+($(NAME)*) GetInstance:(UAVObjectManager*)objMngr
{
    return ($(NAME)*)[objMngr getObjectWithObjId:$(OBJIDHEX) andInstId:0];
}

+($(NAME)*) GetInstance:(UAVObjectManager*)objMngr withInstId:(uint32_t)newInstID
{
    return ($(NAME)*)[objMngr getObjectWithObjId:$(OBJIDHEX) andInstId:newInstID];
}

@end
