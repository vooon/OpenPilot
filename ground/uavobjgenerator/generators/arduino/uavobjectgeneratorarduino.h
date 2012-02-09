/**
 ******************************************************************************
 *
 * @file       uavobjectgeneratorarduino.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      produce arduino code for handling uavobjects
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

#ifndef UAVOBJECTGENERATORARDUINO_H
#define UAVOBJECTGENERATORARDUINO_H

#include "../generator_common.h"

class UAVObjectGeneratorArduino
{
public:
    bool generate(UAVObjectParser *parser, QString templatepath, QString outputpath);

private:
    bool process_object(ObjectInfo *info);
    QStringList fieldTypeStrC;
    QString arduinoIncludeObjTemplate;
    QString arduinoCodeObj1Template, arduinoCodeObj2Template;
    QString arduinoKeywordsObjTemplate;
    QString arduinoObjectsObjTemplate;
    QString codeObj2;
    QString outInclude, outCode, outKeywords, outObjects;
};

#endif
