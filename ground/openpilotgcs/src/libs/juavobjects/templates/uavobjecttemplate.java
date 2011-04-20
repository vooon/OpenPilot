/**
 ******************************************************************************
 *
 * @file       uavobjecttemplate.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Template for an uavobject in java
 *             $(GENERATEDWARNING)
 *             $(DESCRIPTION)
 *
 * @see        The GNU Public License (GPL) Version 3
 *
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

package org.openpilot.uavtalk.uavobjects;

import  org.openpilot.uavtalk.UAVObject;
import  org.openpilot.uavtalk.UAVObjectMetaData;
import org.openpilot.uavtalk.UAVObjectFieldDescription;
/**
$(DESCRIPTION)

generated from $(XMLFILE)
**/
public class $(NAME) extends UAVObject{

$(FIELDSINIT)

    public void setGeneratedMetaData() {

        getMetaData().setGCSAccess(UAVObjectMetaData.$(GCSACCESS));
	getMetaData().setGCSTelemetryAcked(UAVObjectMetaData.$(GCSTELEM_ACKEDTF));
	getMetaData().setGCSTelemetryUpdateMode(UAVObjectMetaData.$(GCSTELEM_UPDATEMODE));
	getMetaData().setGCSTelemetryUpdatePeriod($(GCSTELEM_UPDATEPERIOD));

	getMetaData().setFlightAccess(UAVObjectMetaData.$(FLIGHTACCESS));
        getMetaData().setFlightTelemetryAcked(UAVObjectMetaData.$(FLIGHTTELEM_ACKEDTF));
	getMetaData().setFlightTelemetryUpdateMode(UAVObjectMetaData.$(FLIGHTTELEM_UPDATEMODE));
	getMetaData().setFlightTelemetryUpdatePeriod($(FLIGHTTELEM_UPDATEPERIOD));

	getMetaData().setLoggingUpdateMode(UAVObjectMetaData.$(LOGGING_UPDATEMODE));
	getMetaData().setLoggingUpdatePeriod($(LOGGING_UPDATEPERIOD));

    }
    
    public int getObjID() {
	return $(OBJIDHEX);
    }
    
    public String getObjName() {
	return "$(NAME)";
    }

    public String getObjDescription() {
	return "$(DESCRIPTION)";
    }

    public boolean isSetting() {
	return $(ISSETTINGSTF);
    }

    public boolean isMetaData() {
	return false;
    }


$(GETTERSETTER)
}