/**
 ******************************************************************************
 *
 * @file       vmreadinstructionform.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup VMReadInstructionForm
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
#ifndef VMREADINSTRUCTIONFORM_H_
#define VMREADINSTRUCTIONFORM_H_

#include <QWidget>
//#include <QLabel>
//#include <QComboBox>
//#include <QSpinBox>
#include <vector>

using namespace std;

#include "ui_vmreadinstructionform.h"

class ConfigOnputWidget2;

class VMReadInstructionForm : public QWidget
{
    Q_OBJECT

public:
    explicit VMReadInstructionForm(const int index, QWidget *parent = NULL);
    ~VMReadInstructionForm();
    friend class ConfigOnputWidget2;

    void setNumInstructions(int val);
    QString getInstructionType();
    void getJumpInstruction(int *relativeJump, int *numJumps);
    void getReadInstruction(int*numReadBytes);
    void getWriteInstruction(vector<int> *val);
    int getDelayInstruction();
    void setHexRepresentation(bool val);
    void setNumReadBytes(int numBytes);
    void getReadOutputInstructions(vector<int> *valInt, vector<QString> *valStr);

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
    Ui::VMReadInstructionForm ui;
    int m_index; //Instruction index
    int numInstructions;

private slots:
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

#endif // VMREADINSTRUCTIONFORM_H_
