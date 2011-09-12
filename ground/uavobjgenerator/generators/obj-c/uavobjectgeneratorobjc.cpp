/**
 ******************************************************************************
 *
 * @file       uavobjectgeneratorobjc.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      produce objc code for uavobjects
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

#include "uavobjectgeneratorobjc.h"
using namespace std;

bool UAVObjectGeneratorObjC::generate(UAVObjectParser* parser,QString templatepath,QString outputpath) {

    fieldTypeStrCPP << "int8_t" << "int16_t" << "int32_t" <<
        "uint8_t" << "uint16_t" << "uint32_t" << "float" << "uint8_t";

    fieldTypeStrCPPClass << "UAVObjectField_INT8" << "UAVObjectField_INT16" << "UAVObjectField_INT32"
        << "UAVObjectField_UINT8" << "UAVObjectField_UINT16" << "UAVObjectField_UINT32" << "UAVObjectField_FLOAT32" << "UAVObjectField_ENUM";

    objcCodePath = QDir( templatepath + QString(OBJC_TEMPLATE_DIR));
    objcOutputPath = QDir( outputpath + QString("objc") );
    objcOutputPath.mkpath(objcOutputPath.absolutePath());




    objcCodeTemplate = readFile( objcCodePath.absoluteFilePath("uavobjecttemplate-objc.m") );
    objcIncludeTemplate = readFile( objcCodePath.absoluteFilePath("uavobjecttemplate-objc.h") );
    QString objcInitTemplate = readFile( objcCodePath.absoluteFilePath("uavobjectsinittemplate-objc.m") );

    if (objcCodeTemplate.isEmpty() || objcIncludeTemplate.isEmpty() || objcInitTemplate.isEmpty()) {
        std::cerr << "Problem reading objc code templates" << endl;
        return false;
    }

    QString objInc;
    QString objcObjInit;

    for (int objidx = 0; objidx < parser->getNumObjects(); ++objidx) {
        ObjectInfo* info=parser->getObjectByIndex(objidx);
        process_object(info);

        objcObjInit.append("    tmpObj=[[" + info->name + " alloc] init];\n");
        objcObjInit.append("    [objMngr registerObject:tmpObj];\n");
        objcObjInit.append("    [tmpObj release];\n\n");


        objInc.append("#import \"" + info->namelc + ".h\"\n");
    }

    // Write the objc object inialization files
    objcInitTemplate.replace( QString("$(OBJINC)"), objInc);
    objcInitTemplate.replace( QString("$(OBJINIT)"), objcObjInit);
    bool res = writeFileIfDiffrent( objcOutputPath.absolutePath() + "/uavobjectsinit.m", objcInitTemplate );
    if (!res) {
        cout << "Error: Could not write output files" << endl;
        return false;
    }

    return true; // if we come here everything should be fine
}

/**
 * Generate the ObjC object files
 */
bool UAVObjectGeneratorObjC::process_object(ObjectInfo* info)
{
    if (info == NULL)
        return false;

    // Prepare output strings
    QString outInclude = objcIncludeTemplate;
    QString outCode = objcCodeTemplate;

    // Replace common tags
    replaceCommonTags(outInclude, info);
    replaceCommonTags(outCode, info);

    // Replace the $(DATAFIELDS) tag
    QString type;
    QString fields;
    for (int n = 0; n < info->fields.length(); ++n)
    {
        // Determine type
        type = fieldTypeStrCPP[info->fields[n]->type];
        // Append field
        if ( info->fields[n]->numElements > 1 )
        {
            fields.append( QString("        %1 %2[%3];\n").arg(type).arg(info->fields[n]->name)
                           .arg(info->fields[n]->numElements) );
        }
        else
        {
            fields.append( QString("        %1 %2;\n").arg(type).arg(info->fields[n]->name) );
        }
    }
    outInclude.replace(QString("$(DATAFIELDS)"), fields);

    // Replace the $(FIELDSINIT) tag
    QString finit;
    for (int n = 0; n < info->fields.length(); ++n)
    {
        // Setup element names
        QString varElemName = info->fields[n]->name + "ElemNames";
        finit.append( QString("    NSArray *%1 = [NSArray arrayWithObjects:").arg(varElemName) );
        QStringList elemNames = info->fields[n]->elementNames;
        for (int m = 0; m < elemNames.length(); ++m)
            finit.append( QString("@\"%1\",")
                          .arg(elemNames[m]) );
        finit.append( QString("nil];\n") );


        // Only for enum types
        if (info->fields[n]->type == FIELDTYPE_ENUM) {
            QString varOptionName = info->fields[n]->name + "EnumOptions";
            finit.append( QString("    NSArray *%1 = [NSArray arrayWithObjects:").arg(varOptionName) );
            QStringList options = info->fields[n]->options;
            for (int m = 0; m < options.length(); ++m)
            {
                finit.append( QString("@\"%1\",")
                              .arg(options[m]) );
            }
            finit.append( QString("nil];\n") );
            finit.append( QString("    [tmpFields addObject:[[UAVObjectField alloc]initWithName:@\"%1\" andUnits:@\"%2\" andType:UAVObjectField_ENUM andElementNames:%3 andOptions:%4]];\n\n")
                          .arg(info->fields[n]->name)
                          .arg(info->fields[n]->units)
                          .arg(varElemName)
                          .arg(varOptionName) );

        }
        // For all other types
        else {
            finit.append( QString("    [tmpFields addObject:[[UAVObjectField alloc]initWithName:@\"%1\" andUnits:@\"%2\" andType:%3 andElementNames:%4 andOptions:nil]];\n\n")
                          .arg(info->fields[n]->name)
                          .arg(info->fields[n]->units)
                          .arg(fieldTypeStrCPPClass[info->fields[n]->type])
                          .arg(varElemName) );
        }
    }
    outCode.replace(QString("$(FIELDSINIT)"), finit);

    // Replace the $(DATAFIELDINFO) tag
    QString name;
    QString enums;
    for (int n = 0; n < info->fields.length(); ++n)
    {
        enums.append(QString("    // Field %1 information\n").arg(info->fields[n]->name));
        // Only for enum types
        if (info->fields[n]->type == FIELDTYPE_ENUM)
        {
            enums.append(QString("    /* Enumeration options for field %1 */\n").arg(info->fields[n]->name));
            enums.append("    typedef enum { ");
            // Go through each option
            QStringList options = info->fields[n]->options;
            for (int m = 0; m < options.length(); ++m) {
                QString s = (m != (options.length()-1)) ? "%1_%2_%3=%4, " : "%1_%2_%3=%4";
                enums.append( s.arg( info->namelc )
                               .arg( info->fields[n]->name.toUpper() )
                               .arg( options[m].toUpper().replace(QRegExp(ENUM_SPECIAL_CHARS), "") )
                               .arg(m) );

            }
            enums.append( QString(" } %1_%2Options;\n")
                          .arg(info->namelc).arg( info->fields[n]->name ) );
        }
        // Generate element names (only if field has more than one element)
        if (info->fields[n]->numElements > 1 && !info->fields[n]->defaultElementNames) {
            enums.append(QString("    /* Array element names for field %1 */\n").arg(info->fields[n]->name));
            enums.append("    typedef enum { ");
            // Go through the element names
            QStringList elemNames = info->fields[n]->elementNames;
            for (int m = 0; m < elemNames.length(); ++m) {
                QString s = (m != (elemNames.length()-1)) ? "%1_%2=%3, " : "%1_%2=%3";
                enums.append( s.arg( info->fields[n]->name.toUpper() )
                               .arg( elemNames[m].toUpper() )
                               .arg(m) );

            }
            enums.append( QString(" } %1_%2Elem;\n")
                          .arg(info->namelc).arg( info->fields[n]->name ) );
        }
        // Generate array information
        if (info->fields[n]->numElements > 1) {
            enums.append(QString("    /* Number of elements for field %1 */\n").arg(info->fields[n]->name));
            enums.append( QString("    #define %1_%2_NUMELEM  %3\n")
                          .arg(info->namelc)
                          .arg( info->fields[n]->name.toUpper() )
                          .arg( info->fields[n]->numElements ) );
        }
    }
    enums.append( QString("    #define %1_OBJID  0x%2\n")
                  .arg(info->namelc)
                  .arg(QString().setNum(info->id,16).toUpper()) );

    outInclude.replace(QString("$(DATAFIELDINFO)"), enums);

    // Replace the $(INITFIELDS) tag
    QString initfields;
    for (int n = 0; n < info->fields.length(); ++n)
    {
        if (!info->fields[n]->defaultValues.isEmpty() )
        {
            // For non-array fields
            if ( info->fields[n]->numElements == 1)
            {
                if ( info->fields[n]->type == FIELDTYPE_ENUM )
                {
                    initfields.append( QString("    ((%1_DataFields*)data)->%2 = %3;\n")
                                .arg( info->namelc )
                                .arg( info->fields[n]->name )
                                .arg( info->fields[n]->options.indexOf( info->fields[n]->defaultValues[0] ) ) );
                }
                else if ( info->fields[n]->type == FIELDTYPE_FLOAT32 )
                {
                    initfields.append( QString("    ((%1_DataFields*)data)->%2 = %3;\n")
                                .arg( info->namelc )
                                 .arg( info->fields[n]->name )
                                .arg( info->fields[n]->defaultValues[0].toFloat() ) );
                }
                else
                {
                    initfields.append( QString("    ((%1_DataFields*)data)->%2 = %3;\n")
                                .arg( info->namelc )
                                .arg( info->fields[n]->name )
                                .arg( info->fields[n]->defaultValues[0].toInt() ) );
                }
            }
            else
            {
                // Initialize all fields in the array
                for (int idx = 0; idx < info->fields[n]->numElements; ++idx)
                {
                    if ( info->fields[n]->type == FIELDTYPE_ENUM ) {
                        initfields.append( QString("    ((%1_DataFields*)data)->%2[%3] = %4;\n")
                                    .arg( info->namelc )
                                    .arg( info->fields[n]->name )
                                    .arg( idx )
                                    .arg( info->fields[n]->options.indexOf( info->fields[n]->defaultValues[idx] ) ) );
                    }
                    else if ( info->fields[n]->type == FIELDTYPE_FLOAT32 ) {
                        initfields.append( QString("    ((%1_DataFields*)data)->%2[%3] = %4;\n")
                                    .arg( info->namelc )
                                    .arg( info->fields[n]->name )
                                    .arg( idx )
                                    .arg( info->fields[n]->defaultValues[idx].toFloat() ) );
                    }
                    else {
                        initfields.append( QString("    ((%1_DataFields*)data)->%2[%3] = %4;\n")
                                    .arg( info->namelc )
                                    .arg( info->fields[n]->name )
                                    .arg( idx )
                                    .arg( info->fields[n]->defaultValues[idx].toInt() ) );
                    }
                }
            }
        }
    }

    outCode.replace(QString("$(INITFIELDS)"), initfields);

    // Write the ObjC code
    bool res = writeFileIfDiffrent( objcOutputPath.absolutePath() + "/" + info->namelc + ".m", outCode );
    if (!res) {
        cout << "Error: Could not write objc output files" << endl;
        return false;
    }
    res = writeFileIfDiffrent( objcOutputPath.absolutePath() + "/" + info->namelc + ".h", outInclude );
    if (!res) {
        cout << "Error: Could not write objc output files" << endl;
        return false;
    }

    return true;
}
