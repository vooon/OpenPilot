/**
 ******************************************************************************
 *
 * @file       vminstructionform.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup VMInstructionForm
 * @{
 * @brief GenericI2C virtual machine configuration form
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
#ifndef VMINSTRUCTIONFORM_H_
#define VMINSTRUCTIONFORM_H_

#include <QWidget>
#include "ui_vminstructionform.h"

class ConfigOnputWidget2;

class VMInstructionForm : public QWidget
{
    Q_OBJECT

public:
    explicit VMInstructionForm(const int index, QWidget *parent = NULL);
    ~VMInstructionForm();
    friend class ConfigOnputWidget2;

    void setAssignment(const QString &assignment);
    int index() const;

public slots:
//    void max(int maximum);
//    int max() const;
//    void min(int minimum);
//    int min() const;
//    void minmax(int minimum, int maximum);
//    void neutral(int value);
//    int neutral() const;
//    void enableChannelTest(bool state);

signals:
    void channelChanged(int index, int value);

private:
    Ui::VMInstructionForm ui;
    QStringList instructionTypes;
    int m_index; //Instruction index
    int unhideIdx; //Index of unhidden text fields

private slots:
    void switchCompilerInst(QString instruction);
    void addRegisterField();
    void removeRegisterField();
};

//inline int VMInstructionForm::index() const
//{
//    return m_index;
//}

//inline int VMInstructionForm::max() const
//{
//    return 0;
////    return ui.actuatorMax->value();
//}

//inline int VMInstructionForm::min() const
//{
//    return 0;
////    return ui.actuatorMin->value();
//}

//inline int VMInstructionForm::neutral() const
//{
//    return 0;
////    return ui.actuatorNeutral->value();
//}
#endif // VMINSTRUCTIONFORM_H_
