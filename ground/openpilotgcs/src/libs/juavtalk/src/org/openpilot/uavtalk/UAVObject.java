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

package org.openpilot.uavtalk;

import org.openpilot.uavtalk.UAVObjectFieldDescription;
import java.util.Vector;

/**
 ******************************************************************************
 *
 * @file       UAVObject.java
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      abstract class for an UAVObject
 *
 ****************************************************************************
*/
public abstract class UAVObject {

    abstract public int getObjID();
    abstract public String getObjName();
    abstract public String getObjDescription();

    abstract public Object getField(int fieldid,int arr_pos);
    abstract public void setField(int fieldid,int arr_pos,Object val);

    abstract public void setGeneratedMetaData();

    abstract public UAVObjectFieldDescription[] getFieldDescriptions(); 

    private Vector change_listeners=new Vector();
    private UAVObjectMetaData myMetaData=null;

    public void notifyChangeListeners() {
	for (Object listener: change_listeners)
	    ((UAVObjectChangeListener)listener).notifyUAVObjectChange(this);
    }

    public void addChangeListener(UAVObjectChangeListener newChangeListener) {
	change_listeners.add(newChangeListener);
    }

    public byte[] serialize() {
    	getMetaData().setLastSerialize(System.currentTimeMillis());
    	return new byte[0];
    }
    
    public void deserialize(byte[] data) {
    	deserialize(data,0);
    }
    
    public void deserialize(byte[] data,int offset) {
    	getMetaData().setLastDeserialize(System.currentTimeMillis());
    }
    
    public UAVObjectMetaData getMetaData() {
    	if (myMetaData==null) {
    		myMetaData=new UAVObjectMetaData(this);
    		setGeneratedMetaData();
    	}
    	return myMetaData;
    }

    public int getDataLength() {
    	return serialize().length;
    }

    public final static boolean TRUE=true;
    public final static boolean FALSE=false;
}
