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

/**
 ******************************************************************************
 *
 * @file       UAVObjectMetaData.java
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      Class to handle the metadata of an UAVObject
 *
 ****************************************************************************
 */
public class UAVObjectMetaData extends UAVObject {

    public UAVObjectMetaData(UAVObject parent) {
        this.parent=parent;
    }

    private UAVObject parent;

    /**
     * the MetaData objId is the parent objId with lsb to 1
     **/
    public int getObjID() {
        return parent.getObjID() | 1; 
    }
    
    public String getObjName() {
        return parent.getObjName()+"MetaData";
    }

    public String getObjDescription() {
        return "MetaData for " + parent.getObjName();
    }

    @Override
    public byte[] serialize() {
        super.serialize();
        return new byte[] {
            getFlightAccess(),
            getGCSAccess(),
            isFlightTelemetryAcked()?(byte)1:(byte)0,
            getFlightTelemetryUpdateMode(),
            (byte)((getFlightTelemetryUpdatePeriod()>>0)&0xFF),
            (byte)((getFlightTelemetryUpdatePeriod()>>8)&0xFF),
            (byte)((getFlightTelemetryUpdatePeriod()>>16)&0xFF),
            (byte)((getFlightTelemetryUpdatePeriod()>>24)&0xFF),
            isGCSTelemetryAcked()?(byte)1:(byte)0,
            getGCSTelemetryUpdateMode(),
            (byte)((getGCSTelemetryUpdatePeriod()>>0)&0xFF),
            (byte)((getGCSTelemetryUpdatePeriod()>>8)&0xFF),
            (byte)((getGCSTelemetryUpdatePeriod()>>16)&0xFF),
            (byte)((getGCSTelemetryUpdatePeriod()>>24)&0xFF),
            getLoggingUpdateMode(),
            (byte)((getLoggingUpdatePeriod()>>0)&0xFF),
            (byte)((getLoggingUpdatePeriod()>>8)&0xFF),
            (byte)((getLoggingUpdatePeriod()>>16)&0xFF),
            (byte)((getLoggingUpdatePeriod()>>24)&0xFF),
        };
    }

    public Object getField(int fieldid,int arr_pos) {
        return null;
    }

    public void setField(int fieldid,int arr_pos,Object val) {
    }

    public void setGeneratedMetaData() {
        getMetaData().gcsAccess = ACCESS_READWRITE;
        getMetaData().gcsTelemetryAcked = FALSE;
        getMetaData().gcsTelemetryUpdateMode = UPDATEMODE_ONCHANGE;
        getMetaData().gcsTelemetryUpdatePeriod = 0;

        getMetaData().flightAccess = ACCESS_READWRITE;
        getMetaData().flightTelemetryAcked = FALSE;
        getMetaData().flightTelemetryUpdateMode = UPDATEMODE_ONCHANGE;
        getMetaData().flightTelemetryUpdatePeriod = 0;

        getMetaData().loggingUpdateMode = UPDATEMODE_NEVER;
        getMetaData().loggingUpdatePeriod = 0;
    }

    public UAVObjectFieldDescription[] getFieldDescriptions() {
        return null;
    }

    public final static byte ACCESS_READWRITE=0;
    public final static byte ACCESS_READONLY=1;
    public final static byte ACCESS_WRITEONLY=2;

    public final static String getAccessString(byte access_mode) {
        switch (access_mode) {
        case ACCESS_READWRITE:
            return "Read/Write";
        case ACCESS_WRITEONLY:
            return "Write only";
        case ACCESS_READONLY:
            return "Read only";
        default:
            return "unknowm Access Mode";
        }
    }

    public final static byte UPDATEMODE_NEVER=3;
    public final static byte UPDATEMODE_MANUAL=2;
    public final static byte UPDATEMODE_ONCHANGE=1;
    public final static byte UPDATEMODE_PERIODIC=0;

    public final static String getUpdateModeString(byte update_mode) {
        switch(update_mode) {
        case UPDATEMODE_NEVER:
            return "never";
        case UPDATEMODE_MANUAL:
            return "manual";
        case UPDATEMODE_ONCHANGE:
            return "on change";
        case UPDATEMODE_PERIODIC:
            return "periodic";
        default:
            return "unknown";
        }
    }

    public void setGCSAccess(byte gcsAccess) {
        this.gcsAccess = gcsAccess;
    }

    public byte getGCSAccess() {
        return gcsAccess;
    }

    public void setGCSTelemetryAcked(boolean gcsTelemetryAcked) {
        this.gcsTelemetryAcked = gcsTelemetryAcked;
    }

    public boolean isGCSTelemetryAcked() {
        return gcsTelemetryAcked;
    }

    public void setGCSTelemetryUpdateMode(byte gcsTelemetryUpdateMode) {
        this.gcsTelemetryUpdateMode = gcsTelemetryUpdateMode;
    }

    public byte getGCSTelemetryUpdateMode() {
        return gcsTelemetryUpdateMode;
    }

    public void setGCSTelemetryUpdatePeriod(int gcsTelemetryUpdatePeriod) {
        this.gcsTelemetryUpdatePeriod = gcsTelemetryUpdatePeriod;
        this.notifyChangeListeners();
    }

    public int getGCSTelemetryUpdatePeriod() {
        return gcsTelemetryUpdatePeriod;
    }

    public void setFlightAccess(byte flightAccess) {
        this.flightAccess = flightAccess;
    }

    public byte getFlightAccess() {
        return flightAccess;
    }

    public void setFlightTelemetryAcked(boolean flightTelemetryAcked) {
        this.flightTelemetryAcked = flightTelemetryAcked;
    }

    public boolean isFlightTelemetryAcked() {
        return flightTelemetryAcked;
    }

    public void setFlightTelemetryUpdatePeriod(int flightTelemetryUpdatePeriod) {
        this.flightTelemetryUpdatePeriod = flightTelemetryUpdatePeriod;
        this.notifyChangeListeners();
    }

    public int getFlightTelemetryUpdatePeriod() {
        return flightTelemetryUpdatePeriod;
    }

    public void setFlightTelemetryUpdateMode(byte flightTelemetryUpdateMode) {
        this.flightTelemetryUpdateMode = flightTelemetryUpdateMode;
    }

    public byte getFlightTelemetryUpdateMode() {
        return flightTelemetryUpdateMode;
    }

    public void setLoggingUpdateMode(byte loggingUpdateMode) {
        this.loggingUpdateMode = loggingUpdateMode;
    }

    public byte getLoggingUpdateMode() {
        return loggingUpdateMode;
    }

    public void setLoggingUpdatePeriod(int loggingUpdatePeriod) {
        this.loggingUpdatePeriod = loggingUpdatePeriod;
    }

    public int getLoggingUpdatePeriod() {
        return loggingUpdatePeriod;
    }

    public void setLastSerialize(long last_serialize) {
        this.last_serialize = last_serialize;
    }

    public long getLastSerialize() {
        return last_serialize;
    }

    public void setLastDeserialize(long last_deserialize) {
        this.last_deserialize = last_deserialize;
    }

    public long getLastDeserialize() {
        return last_deserialize;
    }

    public void setLastLog(long last_log) {
        this.last_log = last_log;
    }

    public long getLastLog() {
        return last_log;
    }

    public void setLastGCSUpdate(long last_gcs_update) {
        this.last_gcs_update = last_gcs_update;
    }

    public long getLastGCSUpdate() {
        return last_gcs_update;
    }

    public void setLastFligtUpdate(long last_fligt_update) {
        this.last_fligt_update = last_fligt_update;
    }

    public long getLastFlightUpdate() {
        return last_fligt_update;
    }

    public void setLastSendTime(long last_send_time) {
        this.last_send_time = last_send_time;
    }

    public long getLastSendTime() {
        return last_send_time;
    }

    public void setAckPending(boolean ack_pending) {
        this.ack_pending = ack_pending;
    }

    public boolean isAckPending() {
        return ack_pending;
    }

    private byte gcsAccess;
    private boolean gcsTelemetryAcked;
    private byte gcsTelemetryUpdateMode;
    private int gcsTelemetryUpdatePeriod;

    private byte flightAccess; 
    private boolean  flightTelemetryAcked;
    private byte  flightTelemetryUpdateMode;
    private int flightTelemetryUpdatePeriod;

    private byte loggingUpdateMode;
    private int loggingUpdatePeriod;

    private long last_serialize;
    private long last_deserialize;

    private long last_log;
    private long last_gcs_update;
    private long last_fligt_update;

    private long last_send_time;

    private boolean ack_pending=false;

    public boolean isSetting() {
        return false;
    }

    public boolean isMetaData() {
        return true;
    }
}
