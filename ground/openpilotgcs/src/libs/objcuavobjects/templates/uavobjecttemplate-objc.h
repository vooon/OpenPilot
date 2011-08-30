/**
 ******************************************************************************
 *
 * @file       $(NAMELC).h
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

#import "UAVDataObject.h"
#import "UAVObjectManager.h"
#import <Foundation/Foundation.h>

// Field structure
typedef struct {
$(DATAFIELDS)
} __attribute__((packed)) $(NAMELC)_DataFields;

// Field information
$(DATAFIELDINFO)

@interface $(NAME) : UAVDataObject {
    $(NAMELC)_DataFields objectData;
};

-(id) init;
-($(NAMELC)_DataFields) getData;
-(void) setData:($(NAMELC)_DataFields) newData;
-(Metadata) getDefaultMetadata;
-(UAVDataObject*) clone:(uint32_t) newInstID;
+($(NAME)*) GetInstance:(UAVObjectManager*)objMngr ;
+($(NAME)*) GetInstance:(UAVObjectManager*)objMngr withInstId:(uint32_t)instID;
-(void) setDefaultFieldValues;

@end
