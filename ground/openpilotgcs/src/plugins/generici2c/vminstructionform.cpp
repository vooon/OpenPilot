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

#include "vminstructionform.h"
#include "generici2cwidget.h"
#include <QDebug>

VMInstructionForm::VMInstructionForm(const int index, QWidget *parent) :
    QWidget(parent),
    ui(),
    m_index(index),
    unhideIdx(0)
{
    ui.setupUi(this);

    // Name VM instructions starting at 0.
    ui.instructionNumberLabel->setText(QString("Line #%1:").arg(m_index));

    instructionTypes << "None" << "Write" << "Read" << "Delay [ms]" << "Send UAVO" << "Delete line";
    ui.InstructionComboBox->addItems(instructionTypes);
    ui.InstructionComboBox->setCurrentIndex(0);

    //Hide extra text fields
    ui.register2LineEdit->setEnabled(false);
    ui.register3LineEdit->setEnabled(false);
    ui.register4LineEdit->setEnabled(false);
    ui.register5LineEdit->setEnabled(false);
    ui.register6LineEdit->setEnabled(false);

    // Register for ActuatorSettings changes:
    connect(ui.InstructionComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(switchCompilerInst(QString)));
    connect(ui.AddFieldPushButton, SIGNAL(clicked()), this, SLOT(addRegisterField()));
    connect(ui.RemoveFieldPushButton, SIGNAL(clicked()), this, SLOT(removeRegisterField()));

}

VMInstructionForm::~VMInstructionForm()
{
    // Do nothing
}

/**
 * Configure following fields based on the type of VM instruction.
 */
void VMInstructionForm::switchCompilerInst(QString instruction){
    qDebug()<<"Switching compiler instruction to " << instruction << ".";
    if(instruction=="Write"){
    }
    else if(instruction=="Read"){
    }
    else if(instruction=="Delay [ms]"){
    }
    else if(instruction=="Send UAVO"){
    }
    else if (instruction=="Delete line"){
        //First, ask to ensure this is what the user wants to do
        //IMPLEMENT DIALOG BOX

        //Then, remove GUI line
        //IMPLEMENT THIS

        //Lastly shift all other indices up one
        //THIS TOO
    }
}

void VMInstructionForm::addRegisterField(){
    if(++unhideIdx > 5 )
        unhideIdx=5;
    switch(unhideIdx){
    case 1:
        ui.register2LineEdit->setEnabled(true);
        break;
    case 2:
        ui.register3LineEdit->setEnabled(true);
        break;
    case 3:
        ui.register4LineEdit->setEnabled(true);
        break;
    case 4:
        ui.register5LineEdit->setEnabled(true);
        break;
    case 5:
        ui.register6LineEdit->setEnabled(true);
        break;
    default:
        break;

    }
}

void VMInstructionForm::removeRegisterField(){
    if(--unhideIdx < 1 )
        unhideIdx=1;
    switch(unhideIdx){
    case 1:
        ui.register2LineEdit->setEnabled(false);
        break;
    case 2:
        ui.register3LineEdit->setEnabled(false);
        break;
    case 3:
        ui.register4LineEdit->setEnabled(false);
        break;
    case 4:
        ui.register5LineEdit->setEnabled(false);
        break;
    case 5:
        ui.register6LineEdit->setEnabled(false);
        break;
    default:
        break;

    }
}
