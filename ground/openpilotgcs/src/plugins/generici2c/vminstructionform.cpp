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
    unhideIdx(1)
{
    ui.setupUi(this);

    // Name VM instructions starting at 0.
    ui.instructionNumberLabel->setText(QString("Line #%1:").arg(m_index));

    instructionTypes << "None" << "Delay [ms]" << "Write" << "Read" << "Jump to line" << "Send UAVO" << "Delete line";
    ui.InstructionComboBox->addItems(instructionTypes);
    ui.InstructionComboBox->setCurrentIndex(0);

    //Hide extra text fields
    ui.register2LineEdit->hide();
    ui.register3LineEdit->hide();
    ui.register4LineEdit->hide();
    ui.register5LineEdit->hide();
    ui.register6LineEdit->hide();
    ui.register7LineEdit->hide();
    ui.register8LineEdit->hide();


    //Declare elements
    numReadBytesSpinBox = new QSpinBox;
    delayMsSpinBox = new QSpinBox;
    jumpToLineComboBox = new QComboBox;

    //Add elements to layout.
    ui.gridLayout->addWidget(numReadBytesSpinBox,0,2);
    ui.gridLayout->addWidget(delayMsSpinBox,0,2);
    ui.gridLayout->addWidget(jumpToLineComboBox,0,2);

    numReadBytesSpinBox->setSuffix(" bytes");

    //Hide all elements
    numReadBytesSpinBox->hide();
    delayMsSpinBox->hide();
    jumpToLineComboBox->hide();

//    QHBoxLayout *layout = new QHBoxLayout;
//    layout->addWidget(button1);
//    layout->addWidget(button2);
//    layout->addWidget(button3);
//    layout->addWidget(button4);
//    layout->addWidget(button5);

//    parent->setLayout(layout);
//    parent->show();
    // Register for ActuatorSettings changes:
    connect(ui.InstructionComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(switchCompilerInst(QString)));
    connect(ui.AddFieldPushButton, SIGNAL(clicked()), this, SLOT(addRegisterField()));
    connect(ui.RemoveFieldPushButton, SIGNAL(clicked()), this, SLOT(removeRegisterField()));

//    setAttribute(Qt::WA_LayoutUsesWidgetRect);

}

VMInstructionForm::~VMInstructionForm()
{
    // Do nothing
}

/**
 * Configure following fields based on the type of VM instruction.
 */
void VMInstructionForm::switchCompilerInst(QString instruction)
{
    qDebug()<<"Switching compiler instruction to " << instruction << ".";

    //First, hide all elements
    numReadBytesSpinBox->hide();
    jumpToLineComboBox->hide();
    delayMsSpinBox->hide();
    ui.AddFieldPushButton->hide();
    ui.RemoveFieldPushButton->hide();

    //NOT the best way to hide the register fields. Should do something with regexp instead
    for (int i=0; i<100; i++){ //Why 100? Why not?
        removeRegisterField();
    }
    ui.register1LineEdit->hide();

    //Then put them back
    if(instruction=="Write"){
        ui.register1LineEdit->show();
        ui.AddFieldPushButton->show();
        ui.RemoveFieldPushButton->show();
    }
    else if(instruction=="Read"){
//        ui.register1LineEdit->show();
        numReadBytesSpinBox->show();
    }
    else if(instruction=="Delay [ms]"){
        delayMsSpinBox->show();
    }
    else if(instruction=="Jump to line"){
        jumpToLineComboBox->show();
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

void VMInstructionForm::addRegisterField()
{
    switch(unhideIdx){
    case 1:
        ui.register2LineEdit->show();
        break;
    case 2:
        ui.register3LineEdit->show();
        break;
    case 3:
        ui.register4LineEdit->show();
        break;
    case 4:
        ui.register5LineEdit->show();
        break;
    case 5:
        ui.register6LineEdit->show();
        break;
    case 6:
        ui.register7LineEdit->show();
        break;
    case 7:
        ui.register8LineEdit->show();
        break;
    default:
        break;

    }

    if(++unhideIdx > 7 )
        unhideIdx=7;
}

void VMInstructionForm::removeRegisterField(){
    switch(unhideIdx){
    case 1:
        ui.register2LineEdit->hide();
        break;
    case 2:
        ui.register3LineEdit->hide();
        break;
    case 3:
        ui.register4LineEdit->hide();
        break;
    case 4:
        ui.register5LineEdit->hide();
        break;
    case 5:
        ui.register6LineEdit->hide();
        break;
    case 6:
        ui.register7LineEdit->hide();
        break;
    case 7:
        ui.register8LineEdit->hide();
        break;
    default:
        break;
    }

    if(--unhideIdx < 1 )
        unhideIdx=1;
}

/*
 * Make this form instance aware of how many other instruction form instances exist.
 */
void VMInstructionForm::setNumInstructions(int val){
    numInstructions=val;

    //Set the jump table combo box to reflect the new number of items
    QStringList instructionList;
    for (int i=0; i<numInstructions; i++)
        instructionList.append(QString("%1").arg(i));

    int tmpVal=jumpToLineComboBox->currentIndex();
    jumpToLineComboBox->clear();
    jumpToLineComboBox->addItems(instructionList);
    jumpToLineComboBox->setCurrentIndex(tmpVal);
}

QString VMInstructionForm::getInstructionType()
{
    return ui.InstructionComboBox->currentText();
}

/*
 * Compute the relative jump. A negative jump implies backwards movement, vice-versa for a positive jump
 */
int VMInstructionForm::getJumpInstruction()
{
    int relativeJump = jumpToLineComboBox->currentText().toInt() - m_index;
    return relativeJump;
}

void VMInstructionForm::getReadInstruction(int*numReadBytes)
{
    *numReadBytes=numReadBytesSpinBox->value();

    return;
}

void VMInstructionForm::getWriteInstruction(vector<int> *val)
{
    val->clear();
    for (int i=0; i< unhideIdx; i++)
    {
        QLineEdit *lineedit=this->findChild<QLineEdit*>(QString("register%1LineEdit").arg(i+1));
        if(isHex)
            val->push_back(lineedit->text().toInt(0,16));
        else
            val->push_back(lineedit->text().toInt(0,10));
    }
    return;
}

int VMInstructionForm::getDelayInstruction()
{
    return delayMsSpinBox->value();
}

void VMInstructionForm::setHexRepresentation(bool val)
{
    isHex=val;
}

