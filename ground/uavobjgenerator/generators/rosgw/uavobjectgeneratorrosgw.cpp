/**
 ******************************************************************************
 *
 * @file       uavobjectgeneratorgcs.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      produce gcs code for uavobjects
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

#include "uavobjectgeneratorrosgw.h"
using namespace std;

bool UAVObjectGeneratorRosGW::generate(UAVObjectParser *parser, QString templatepath, QString outputpath)
{
    fieldTypeStrCPP << "int8_t" << "int16_t" << "int32_t" <<
        "uint8_t" << "uint16_t" << "uint32_t" << "float" << "uint8_t";

    fieldTypeStrCPPClass << "INT8" << "INT16" << "INT32"
                         << "UINT8" << "UINT16" << "UINT32" << "FLOAT32" << "ENUM";

    rosgwCodePath        = QDir(templatepath + QString(ROSGW_CODE_DIR));
    rosgwOutputPath      = QDir(outputpath + QString("rosuavobject"));
    rosgwOutputPath.mkpath(rosgwOutputPath.absolutePath());

    rosgwCodeTemplate    = readFile(rosgwCodePath.absoluteFilePath("uavobject.cpp.template"));
    rosgwIncludeTemplate = readFile(rosgwCodePath.absoluteFilePath("uavobject.h.template"));
    QString rosgwInitTemplate = readFile(rosgwCodePath.absoluteFilePath("uavobjectsinit.cpp.template"));

    if (rosgwCodeTemplate.isEmpty() || rosgwIncludeTemplate.isEmpty() || rosgwInitTemplate.isEmpty()) {
        std::cerr << "Problem reading ROS GW code templates" << endl;
        return false;
    }

    QString objInc;
    QString rosgwObjInit;

    for (int objidx = 0; objidx < parser->getNumObjects(); ++objidx) {
        ObjectInfo *info = parser->getObjectByIndex(objidx);
        process_object(info);

        rosgwObjInit.append("\tobjMngr->registerObject( new " + info->name + "() );\n");
        objInc.append("#include \"" + info->namelc + ".h\"\n");
    }

    // Write the gcs object inialization files
    rosgwInitTemplate.replace(QString("$(OBJINC)"), objInc);
    rosgwInitTemplate.replace(QString("$(OBJINIT)"), rosgwObjInit);
    bool res = writeFileIfDiffrent(rosgwOutputPath.absolutePath() + "/uavobjectsinit.cpp", rosgwInitTemplate);
    if (!res) {
        cout << "Error: Could not write output files" << endl;
        return false;
    }

    return true; // if we come here everything should be fine
}

/**
 * Generate the ROS GW object files
 */
bool UAVObjectGeneratorRosGW::process_object(ObjectInfo *info)
{
    if (info == NULL) {
        return false;
    }

    // Prepare output strings
    QString outInclude = rosgwIncludeTemplate;
    QString outCode    = rosgwCodeTemplate;

    // Replace common tags
    replaceCommonTags(outInclude, info);
    replaceCommonTags(outCode, info);

    // Replace the $(DATAFIELDS) tag
    QString type;
    QString fields;
    for (int n = 0; n < info->fields.length(); ++n) {
        // Determine type
        type = fieldTypeStrCPP[info->fields[n]->type];
        // Append field
        if (info->fields[n]->numElements > 1) {
            fields.append(QString("\t\t%1 %2[%3];\n").arg(type).arg(info->fields[n]->name)
                          .arg(info->fields[n]->numElements));
        } else {
            fields.append(QString("\t\t%1 %2;\n").arg(type).arg(info->fields[n]->name));
        }
    }
    outInclude.replace(QString("$(DATAFIELDS)"), fields);

    // Replace $(TOSTRINGDATA) and related tags
    QString tostringdata;

    // to avoid name conflicts
    QStringList reservedProperties;
    reservedProperties << "Description" << "Metadata";

    for (int n = 0; n < info->fields.length(); ++n) {
        FieldInfo *field = info->fields[n];

        if (reservedProperties.contains(field->name)) {
            continue;
        }

        // Determine type
        type = fieldTypeStrCPP[field->type];
        // Append field
        if (field->numElements > 1) {
            QString h = "\tsout << \" %1: [\" ";
            tostringdata.append(h.arg(field->name));
            for (int idx = 0; idx < field->numElements; ++idx) {
                QString d = " << std::dec << data.%1[%2]";
                if (idx)
                    d.append(" << \", \"");
                tostringdata.append(d.arg(field->name).arg(idx));
            }
            tostringdata.append(" << \"]\";\n");
        } else {
            QString s = "\tsout << \" %1: \" << data.%2;\n";
            tostringdata.append(s.arg(field->name).arg(field->name));
        }
    }
    outCode.replace(QString("$(TOSTRINGDATA)"), tostringdata);

    // Replace the $(DATAFIELDINFO) tag
    QString name;
    QString enums;
    for (int n = 0; n < info->fields.length(); ++n) {
        enums.append(QString("\t// Field %1 information\n").arg(info->fields[n]->name));
        // Only for enum types
        if (info->fields[n]->type == FIELDTYPE_ENUM) {
            enums.append(QString("\t/* Enumeration options for field %1 */\n").arg(info->fields[n]->name));
            enums.append("\ttypedef enum { ");
            // Go through each option
            QStringList options = info->fields[n]->options;
            for (int m = 0; m < options.length(); ++m) {
                QString s = (m != (options.length() - 1)) ? "%1_%2=%3, " : "%1_%2=%3";
                enums.append(s.arg(info->fields[n]->name.toUpper())
                             .arg(options[m].toUpper().replace(QRegExp(ENUM_SPECIAL_CHARS), ""))
                             .arg(m));
            }
            enums.append(QString(" } %1Options;\n")
                         .arg(info->fields[n]->name));
        }
        // Generate element names (only if field has more than one element)
        if (info->fields[n]->numElements > 1 && !info->fields[n]->defaultElementNames) {
            enums.append(QString("\t/* Array element names for field %1 */\n").arg(info->fields[n]->name));
            enums.append("\ttypedef enum { ");
            // Go through the element names
            QStringList elemNames = info->fields[n]->elementNames;
            for (int m = 0; m < elemNames.length(); ++m) {
                QString s = (m != (elemNames.length() - 1)) ? "%1_%2=%3, " : "%1_%2=%3";
                enums.append(s.arg(info->fields[n]->name.toUpper())
                             .arg(elemNames[m].toUpper())
                             .arg(m));
            }
            enums.append(QString(" } %1Elem;\n")
                         .arg(info->fields[n]->name));
        }
        // Generate array information
        if (info->fields[n]->numElements > 1) {
            enums.append(QString("\t/* Number of elements for field %1 */\n").arg(info->fields[n]->name));
            enums.append(QString("\tstatic const uint32_t %1_NUMELEM = %2;\n")
                         .arg(info->fields[n]->name.toUpper())
                         .arg(info->fields[n]->numElements));
        }
    }
    outInclude.replace(QString("$(DATAFIELDINFO)"), enums);

    // Replace the $(INITFIELDS) tag
    QString initfields;
    for (int n = 0; n < info->fields.length(); ++n) {
        if (!info->fields[n]->defaultValues.isEmpty()) {
            // For non-array fields
            if (info->fields[n]->numElements == 1) {
                if (info->fields[n]->type == FIELDTYPE_ENUM) {
                    initfields.append(QString("\tdata.%1 = %2;\n")
                                      .arg(info->fields[n]->name)
                                      .arg(info->fields[n]->options.indexOf(info->fields[n]->defaultValues[0])));
                } else if (info->fields[n]->type == FIELDTYPE_FLOAT32) {
                    initfields.append(QString("\tdata.%1 = %2;\n")
                                      .arg(info->fields[n]->name)
                                      .arg(info->fields[n]->defaultValues[0].toFloat()));
                } else {
                    initfields.append(QString("\tdata.%1 = %2;\n")
                                      .arg(info->fields[n]->name)
                                      .arg(info->fields[n]->defaultValues[0].toInt()));
                }
            } else {
                // Initialize all fields in the array
                for (int idx = 0; idx < info->fields[n]->numElements; ++idx) {
                    if (info->fields[n]->type == FIELDTYPE_ENUM) {
                        initfields.append(QString("\tdata.%1[%2] = %3;\n")
                                          .arg(info->fields[n]->name)
                                          .arg(idx)
                                          .arg(info->fields[n]->options.indexOf(info->fields[n]->defaultValues[idx])));
                    } else if (info->fields[n]->type == FIELDTYPE_FLOAT32) {
                        initfields.append(QString("\tdata.%1[%2] = %3;\n")
                                          .arg(info->fields[n]->name)
                                          .arg(idx)
                                          .arg(info->fields[n]->defaultValues[idx].toFloat()));
                    } else {
                        initfields.append(QString("\tdata.%1[%2] = %3;\n")
                                          .arg(info->fields[n]->name)
                                          .arg(idx)
                                          .arg(info->fields[n]->defaultValues[idx].toInt()));
                    }
                }
            }
        }
    }
    outCode.replace(QString("$(INITFIELDS)"), initfields);

    // Write the ROS GW code
    bool res = writeFileIfDiffrent(rosgwOutputPath.absolutePath() + "/" + info->namelc + ".cpp", outCode);
    if (!res) {
        cout << "Error: Could not write ros gw output files" << endl;
        return false;
    }
    res = writeFileIfDiffrent(rosgwOutputPath.absolutePath() + "/" + info->namelc + ".h", outInclude);
    if (!res) {
        cout << "Error: Could not write ros gw output files" << endl;
        return false;
    }

    return true;
}
