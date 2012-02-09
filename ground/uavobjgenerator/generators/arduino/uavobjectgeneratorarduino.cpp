/**
******************************************************************************
*
* @file       uavobjectgeneratorarduino.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      produce arduino code for uavobjects
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

#include "uavobjectgeneratorarduino.h"

using namespace std;

bool UAVObjectGeneratorArduino::generate(UAVObjectParser *parser, QString templatepath, QString outputpath)
{
    fieldTypeStrC << "int8_t" << "int16_t" << "int32_t" << "uint8_t"
                  << "uint16_t" << "uint32_t" << "float" << "uint8_t";

    QDir arduinoCodePath(templatepath + QString("ground/uavobjgenerator/generators/arduino/templates"));
    QDir arduinoOutputPath(outputpath + QString("arduino/UAVObjects"));
    arduinoOutputPath.mkpath(arduinoOutputPath.absolutePath());

    // UAVObjects.h
    QString arduinoIncludeHeadTemplate = readFile(arduinoCodePath.absoluteFilePath("UAVObjects.h.head.tmpl"));
    arduinoIncludeObjTemplate = readFile(arduinoCodePath.absoluteFilePath("UAVObjects.h.obj.tmpl"));

    // UAVObjects.cpp
    QString arduinoCodeHeadTemplate = readFile(arduinoCodePath.absoluteFilePath("UAVObjects.cpp.head.tmpl"));
    QString arduinoCodeTail1Template = readFile(arduinoCodePath.absoluteFilePath("UAVObjects.cpp.tail1.tmpl"));
    QString arduinoCodeTail2Template = readFile(arduinoCodePath.absoluteFilePath("UAVObjects.cpp.tail2.tmpl"));
    arduinoCodeObj1Template = readFile(arduinoCodePath.absoluteFilePath("UAVObjects.cpp.obj1.tmpl"));
    arduinoCodeObj2Template = readFile(arduinoCodePath.absoluteFilePath("UAVObjects.cpp.obj2.tmpl"));

    // keywords.txt
    QString arduinoKeywordsHeadTemplate = readFile(arduinoCodePath.absoluteFilePath("keywords.txt.head.tmpl"));
    arduinoKeywordsObjTemplate = readFile(arduinoCodePath.absoluteFilePath("keywords.txt.obj.tmpl"));

    // objects.lst
    arduinoObjectsObjTemplate = readFile(arduinoCodePath.absoluteFilePath("objects.lst.tmpl"));

    if (arduinoIncludeHeadTemplate.isNull() || arduinoIncludeObjTemplate.isNull()
        || arduinoCodeHeadTemplate.isNull() || arduinoCodeTail1Template.isNull()
        || arduinoCodeTail2Template.isNull() ||  arduinoCodeObj1Template.isNull()
        || arduinoCodeObj2Template.isNull() || arduinoKeywordsHeadTemplate.isNull()
        || arduinoKeywordsObjTemplate.isNull()) {
        cerr << "Error: Could not open arduino template files." << endl;
        return false;
    }

    outInclude += arduinoIncludeHeadTemplate;
    outCode += arduinoCodeHeadTemplate;
    outKeywords += arduinoKeywordsHeadTemplate;

    QString arduinoObjInit;
    for (int objidx = 0; objidx < parser->getNumObjects(); ++objidx) {
        ObjectInfo *info = parser->getObjectByIndex(objidx);
        process_object(info);

        arduinoObjInit.append("#if defined(UAVOBJ_INIT_" + info->name.toUpper()
            + ") || ((defined(UAVOBJ_INIT_ALL) || defined(UAVOBJ_INIT_"
            + (info->isSettings ? "SETTINGS" : "DATA")
            + ")) && !defined(UAVOBJ_NOINIT_" + info->name.toUpper() + "))\r\n");
        arduinoObjInit.append("  " + info->name + "Initialize();\r\n");
        arduinoObjInit.append("#endif\r\n");
    }

    // Create the arduino object inialization ocde
    arduinoCodeTail1Template.replace(QString("$(OBJINIT)"), arduinoObjInit);
    outCode += arduinoCodeTail1Template;
    outCode += codeObj2;
    outCode += arduinoCodeTail2Template;

    // Write the arduino code
    if (!writeFileIfDiffrent(arduinoOutputPath.absolutePath() + "/UAVObjects.cpp", outCode)) {
        cout << "Error: Could not write arduino code file" << endl;
        return false;
    }

    if (!writeFileIfDiffrent(arduinoOutputPath.absolutePath() + "/UAVObjects.h", outInclude)) {
        cout << "Error: Could not write arduino include file" << endl;
        return false;
    }

    if (!writeFileIfDiffrent(arduinoOutputPath.absolutePath() + "/keywords.txt", outKeywords)) {
        cout << "Error: Could not write arduino keywords file" << endl;
        return false;
    }

    if (!writeFileIfDiffrent(arduinoOutputPath.absolutePath() + "/objects.lst", outObjects)) {
        cout << "Error: Could not write arduino object list file" << endl;
        return false;
    }

    return true;
}

/**
 * Generate the Arduino object files
 **/
bool UAVObjectGeneratorArduino::process_object(ObjectInfo *info)
{
    if (!info)
        return false;

    // Replace common tags
    QString includeObjTemplate(arduinoIncludeObjTemplate);
    QString codeObj1Template(arduinoCodeObj1Template);
    QString codeObj2Template(arduinoCodeObj2Template);
    QString keywordsObjTemplate(arduinoKeywordsObjTemplate);
    QString objectsObjTemplate(arduinoObjectsObjTemplate);

    replaceCommonTags(includeObjTemplate, info);
    replaceCommonTags(codeObj1Template, info);
    replaceCommonTags(codeObj2Template, info);
    replaceCommonTags(keywordsObjTemplate, info);
    replaceCommonTags(objectsObjTemplate, info);

    QString type;
    QString fields;
    QString enums;
    QString keyword1;
    QString keyword2;
    QString literal1;

    // Replace the $(DATAFIELDS) tag
    for (int n = 0; n < info->fields.length(); ++n) {
        // Determine type
        type = fieldTypeStrC[info->fields[n]->type];
        // Append field
        if (info->fields[n]->numElements > 1)
            fields.append(QString("    %1 %2[%3];\r\n").arg(type)
                .arg(info->fields[n]->name).arg(info->fields[n]->numElements));
        else
            fields.append(QString("    %1 %2;\r\n").arg(type).arg(info->fields[n]->name));
    }
    includeObjTemplate.replace(QString("$(DATAFIELDS)"), fields);

    // Replace the $(DATAFIELDINFO) tag
    for (int n = 0; n < info->fields.length(); ++n) {
        enums.append(QString("// Field %1 information\r\n").arg(info->fields[n]->name));
        // Only for enum types
        if (info->fields[n]->type == FIELDTYPE_ENUM) {
            enums.append(QString("/* Enumeration options for field %1 */\r\n").arg(info->fields[n]->name));
            enums.append("typedef enum { ");
            // Go through each option
            QStringList options = info->fields[n]->options;
            for (int m = 0; m < options.length(); ++m) {
                QString s = (m == (options.length() - 1)) ? "%1_%2_%3=%4" : "%1_%2_%3=%4, ";
                enums.append(s
                    .arg(info->name.toUpper())
                    .arg(info->fields[n]->name.toUpper())
                    .arg(options[m].toUpper().replace(QRegExp(ENUM_SPECIAL_CHARS), ""))
                    .arg(m));
                literal1.append(QString("%1_%2_%3\tLITERAL1\r\n")
                    .arg(info->name.toUpper())
                    .arg(info->fields[n]->name.toUpper())
                    .arg(options[m].toUpper().replace(QRegExp(ENUM_SPECIAL_CHARS), "")));
            }
            enums.append(QString(" } %1%2Options;\r\n")
                .arg(info->name)
                .arg(info->fields[n]->name));
            keyword1.append(QString("%1%2Options\tKEYWORD1\r\n")
                .arg(info->name)
                .arg(info->fields[n]->name));
        }

        // Generate element names (only if field has more than one element)
        if (info->fields[n]->numElements > 1 && !info->fields[n]->defaultElementNames) {
            enums.append(QString("/* Array element names for field %1 */\r\n").arg(info->fields[n]->name));
            enums.append("typedef enum { ");
            // Go through the element names
            QStringList elemNames = info->fields[n]->elementNames;
            for (int m = 0; m < elemNames.length(); ++m) {
                QString s = (m != (elemNames.length() - 1)) ? "%1_%2_%3=%4, " : "%1_%2_%3=%4";
                enums.append(s
                    .arg(info->name.toUpper())
                    .arg(info->fields[n]->name.toUpper())
                    .arg(elemNames[m].toUpper())
                    .arg(m));
                literal1.append(QString("%1_%2_%3\tLITERAL1\r\n")
                    .arg(info->name.toUpper())
                    .arg(info->fields[n]->name.toUpper())
                    .arg(elemNames[m].toUpper()));
            }
            enums.append(QString(" } %1%2Elem;\r\n")
                .arg(info->name)
                .arg(info->fields[n]->name));
            keyword1.append(QString("%1%2Elem\tKEYWORD1\r\n")
                .arg(info->name)
                .arg(info->fields[n]->name));
        }

        // Generate array information
        if (info->fields[n]->numElements > 1) {
            enums.append(QString("/* Number of elements for field %1 */\r\n").arg(info->fields[n]->name));
            enums.append(QString("#define %1_%2_NUMELEM %3\r\n")
                .arg(info->name.toUpper())
                .arg(info->fields[n]->name.toUpper())
                .arg(info->fields[n]->numElements));
            literal1.append(QString("%1_%2_NUMELEM\tLITERAL1\r\n")
                .arg(info->name.toUpper())
                .arg(info->fields[n]->name.toUpper()));
        }
    }
    includeObjTemplate.replace(QString("$(DATAFIELDINFO)"), enums);
    keywordsObjTemplate.replace(QString("$(DATAFIELDINFO)"), keyword1.append(literal1));

    // Replace the $(INITFIELDS) tag
    QString initfields;
    for (int n = 0; n < info->fields.length(); ++n) {
        if (!info->fields[n]->defaultValues.isEmpty()) {
            // For non-array fields
            if (info->fields[n]->numElements == 1) {
                if (info->fields[n]->type == FIELDTYPE_ENUM) {
                    initfields.append(QString("\tdata.%1 = %2;\r\n")
                        .arg(info->fields[n]->name)
                        .arg(info->fields[n]->options.indexOf(info->fields[n]->defaultValues[0])));
                } else if (info->fields[n]->type == FIELDTYPE_FLOAT32) {
                    initfields.append(QString("\tdata.%1 = %2;\r\n")
                        .arg(info->fields[n]->name)
                        .arg(info->fields[n]->defaultValues[0].toFloat()));
                } else {
                    initfields.append(QString("\tdata.%1 = %2;\r\n")
                        .arg(info->fields[n]->name)
                        .arg(info->fields[n]->defaultValues[0].toInt()));
                }
            } else {
                // Initialize all fields in the array
                for (int idx = 0; idx < info->fields[n]->numElements; ++idx) {
                    if (info->fields[n]->type == FIELDTYPE_ENUM) {
                        initfields.append(QString("\tdata.%1[%2] = %3;\r\n")
                            .arg(info->fields[n]->name)
                            .arg(idx)
                            .arg(info->fields[n]->options.indexOf(info->fields[n]->defaultValues[idx])));
                    } else if (info->fields[n]->type == FIELDTYPE_FLOAT32) {
                        initfields.append(QString("\tdata.%1[%2] = %3;\r\n")
                            .arg(info->fields[n]->name)
                            .arg(idx)
                            .arg(info->fields[n]->defaultValues[idx].toFloat()));
                    } else {
                        initfields.append(QString("\tdata.%1[%2] = %3;\r\n")
                            .arg(info->fields[n]->name)
                            .arg(idx)
                            .arg(info->fields[n]->defaultValues[idx].toInt()));
                    }
                }
            }
        }
    }
    codeObj1Template.replace(QString("$(INITFIELDS)"), initfields);

    // Replace the $(SETGETFIELDS) tag
    QString setgetfields;
    for (int n = 0; n < info->fields.length(); ++n) {
        // For non-array fields
        if (info->fields[n]->numElements == 1) {
            // Set
            setgetfields.append(QString("void %2%3Set( %1 *New%3 )\r\n")
                .arg(fieldTypeStrC[info->fields[n]->type])
                .arg(info->name)
                .arg(info->fields[n]->name));
            setgetfields.append(QString("{\r\n"));
            setgetfields.append(QString("\tUAVObjSetDataField(%1Handle(), (void*)New%2, offsetof( %1Data, %2), sizeof(%3));\r\n")
                .arg(info->name)
                .arg(info->fields[n]->name)
                .arg(fieldTypeStrC[info->fields[n]->type]));
            setgetfields.append(QString("}\r\n"));

            // Get
            setgetfields.append(QString("void %2%3Get( %1 *New%3 )\r\n")
                .arg(fieldTypeStrC[info->fields[n]->type])
                .arg(info->name)
                .arg(info->fields[n]->name));
            setgetfields.append(QString("{\r\n"));
            setgetfields.append(QString("\tUAVObjGetDataField(%1Handle(), (void*)New%2, offsetof( %1Data, %2), sizeof(%3));\r\n")
                .arg(info->name)
                .arg(info->fields[n]->name)
                .arg(fieldTypeStrC[info->fields[n]->type]));
            setgetfields.append(QString("}\r\n"));
        } else {
            // Set
            setgetfields.append(QString("void %2%3Set( %1 *New%3 )\r\n")
                .arg(fieldTypeStrC[info->fields[n]->type])
                .arg(info->name)
                .arg(info->fields[n]->name));
            setgetfields.append(QString("{\r\n"));
            setgetfields.append(QString("\tUAVObjSetDataField(%1Handle(), (void*)New%2, offsetof( %1Data, %2), %3*sizeof(%4));\r\n")
                .arg(info->name)
                .arg(info->fields[n]->name)
                .arg(info->fields[n]->numElements)
                .arg(fieldTypeStrC[info->fields[n]->type]));
            setgetfields.append(QString("}\r\n"));

            // Get
            setgetfields.append(QString("void %2%3Get( %1 *New%3 )\r\n")
                .arg(fieldTypeStrC[info->fields[n]->type])
                .arg(info->name)
                .arg(info->fields[n]->name));
            setgetfields.append(QString("{\r\n"));
            setgetfields.append(QString("\tUAVObjGetDataField(%1Handle(), (void*)New%2, offsetof( %1Data, %2), %3*sizeof(%4));\r\n")
                .arg(info->name)
                .arg(info->fields[n]->name)
                .arg(info->fields[n]->numElements)
                .arg(fieldTypeStrC[info->fields[n]->type]));
            setgetfields.append(QString("}\r\n"));
        }
    }
    codeObj1Template.replace(QString("$(SETGETFIELDS)"), setgetfields);

    // Replace the $(SETGETFIELDSEXTERN) tag
    QString setgetfieldsextern;
    for (int n = 0; n < info->fields.length(); ++n) {
        // Set
        setgetfieldsextern.append(QString("extern void %2%3Set( %1 *New%3 );\r\n")
            .arg(fieldTypeStrC[info->fields[n]->type])
            .arg(info->name)
            .arg(info->fields[n]->name));
        keyword2.append(QString("%2%3Set\tKEYWORD1\r\n")
            .arg(info->name)
            .arg(info->fields[n]->name));

      /* GET */
        setgetfieldsextern.append(QString("extern void %2%3Get( %1 *New%3 );\r\n")
            .arg(fieldTypeStrC[info->fields[n]->type])
            .arg(info->name)
            .arg(info->fields[n]->name));
        keyword2.append(QString("%2%3Get\tKEYWORD1\r\n")
            .arg(info->name)
            .arg(info->fields[n]->name));
    }
    includeObjTemplate.replace(QString("$(SETGETFIELDSEXTERN)"), setgetfieldsextern);
    keywordsObjTemplate.replace(QString("$(SETGETFIELDS)"), keyword2);

    outCode += codeObj1Template;
    codeObj2 += codeObj2Template;
    outInclude += includeObjTemplate;
    outKeywords += keywordsObjTemplate;
    outObjects += objectsObjTemplate;

    return true;
}
