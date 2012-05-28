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

//#include "vminstructionform.h"
#include "vmreadinstructionform.h"
//#include "generici2cwidget.h"
#include "generici2csensor.h"
#include "extensionsystem/pluginmanager.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QSpacerItem>

VMReadInstructionForm::VMReadInstructionForm(const int index, QWidget *parent) :
    QWidget(parent),
    ui(),
    m_index(index)
{
    //--------------------------//
    // Setup widget environment //
    //--------------------------//
    //Setup GUI
    ui.setupUi(this);

    //Get UAVO
    ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager* objManager = pm->getObject<UAVObjectManager>();

    GenericI2CSensor *genericI2CSensor = GenericI2CSensor::GetInstance(objManager);
    Q_ASSERT(genericI2CSensor);

    QStringList outputRegisterList;
    QList<UAVObjectField*> genericI2CSensorFields=genericI2CSensor->getFields();
    foreach(UAVObjectField* sensorField, genericI2CSensorFields){
        outputRegisterList << sensorField->getName();
    }
    ui.outputRegisterComboBox->addItems(outputRegisterList);
    ui.outputRegisterComboBox->setCurrentIndex(0);


/*

    //-----------------------//
    // Setup widget elements //
    //-----------------------//
    // Name VM instructions (starts at 0).
    ui.instructionNumberLabel->setText(QString("Line #%1:").arg(m_index));

    //Declare elements
    numReadBytesSpinBox = new QSpinBox;
    delayMsSpinBox = new QSpinBox;
    jumpToLineComboBox = new QComboBox;
    jumpXTimesSpinBox = new QSpinBox;
    jumpXTimesComboBox = new QComboBox;

    QHBoxLayout *readVMLayout = new QHBoxLayout;

//    addReadOutputPushButton = new QPushButton;
//    removeReadOutputPushButton = new QPushButton;

    outputRegisterEndianessComboBox = new QComboBox;
    outputRegisterComboBox = new QComboBox;
    outputRangeLowComboBox = new QComboBox;
    outputRangeHighComboBox = new QComboBox;

    readText1Label = new QLabel;
    readText2Label = new QLabel;
    readText3Label = new QLabel;
    readText4Label = new QLabel;

    //Add elements to layout.
    ui.gridLayout->addWidget(numReadBytesSpinBox,0,2);
    ui.gridLayout->addWidget(delayMsSpinBox,0,2);
    ui.gridLayout->addWidget(jumpToLineComboBox,0,2);
    ui.gridLayout->addWidget(jumpXTimesComboBox,0,3);
    ui.gridLayout->addWidget(jumpXTimesSpinBox,0,4);
#if 0
    ui.gridLayout->addLayout(readVMLayout, 1, 2, 10, 1);
    readVMLayout->addWidget(readText1Label);
    readVMLayout->addWidget(outputRangeLowComboBox);
    readVMLayout->addWidget(readText2Label);
    readVMLayout->addWidget(outputRangeHighComboBox);
    readVMLayout->addWidget(readText3Label);
    readVMLayout->addWidget(outputRegisterComboBox);
    readVMLayout->addWidget(readText4Label);
    readVMLayout->addWidget(outputRegisterEndianessComboBox);
    readVMLayout->addWidget(addReadOutputPushButton);
    readVMLayout->addWidget(removeReadOutputPushButton);
#elseif 0
    ui.gridLayout->addWidget(readText1Label,1,2);
    ui.gridLayout->addWidget(outputRangeLowComboBox, 1, 3);
    ui.gridLayout->addWidget(readText2Label,1,4);
    ui.gridLayout->addWidget(outputRangeHighComboBox, 1, 5);
    ui.gridLayout->addWidget(readText3Label,1,6);
    ui.gridLayout->addWidget(outputRegisterComboBox, 1, 7);
    ui.gridLayout->addWidget(readText4Label,1,8);
    ui.gridLayout->addWidget(outputRegisterEndianessComboBox, 1, 9);
    QSpacerItem *bob = new QSpacerItem(1,1);
//    bob->expandingDirections(Qt::Horizontal);
    ui.gridLayout->addItem(bob, 1, 10);
    ui.gridLayout->addWidget(addReadOutputPushButton, 1, 11);
    ui.gridLayout->addWidget(removeReadOutputPushButton, 1, 12);
#endif

    //------------------------//
    // Set element parameters //
    //------------------------//
    /*
    QStringList instructionTypes;
    instructionTypes << "None" << "Delay [ms]" << "Write" << "Read" << "Jump to line" << "Send UAVO" << "Delete line";
    ui.InstructionComboBox->addItems(instructionTypes);
    ui.InstructionComboBox->setCurrentIndex(0);

    QStringList jumpXTimesList;
    jumpXTimesList << "Continuously" << "Exactly";
    jumpXTimesComboBox->addItems(jumpXTimesList);
    jumpXTimesComboBox->setCurrentIndex(0);

    QStringList outputRegisterEndianessList;
    outputRegisterEndianessList << "Little endian" << "Big endian";
    outputRegisterEndianessComboBox->addItems(outputRegisterEndianessList);
    outputRegisterEndianessComboBox->setCurrentIndex(0);

    QStringList outputRegisterList;
    QList<UAVObjectField*> genericI2CSensorFields=genericI2CSensor->getFields();
    foreach(UAVObjectField* sensorField, genericI2CSensorFields){
        outputRegisterList << sensorField->getName();
    }
    outputRegisterComboBox->addItems(outputRegisterList);
    outputRegisterComboBox->setCurrentIndex(0);

    addReadOutputPushButton->setText("+");
    removeReadOutputPushButton->setText("-");

//    readText1Label->setText(QString("Put bytes range"));
//    readText2Label->setText(QString("to"));
//    readText3Label->setText(QString("into output"));
//    readText4Label->setText(QString("formatted"));

    numReadBytesSpinBox->setSuffix(" bytes");
    numReadBytesSpinBox->setMaximum(255); //8-bit container

    jumpXTimesSpinBox->setSuffix(" times");
    jumpXTimesSpinBox->setMaximum(255); //8-bit container

    //---------------//
    // Hide elements //
    //---------------//
/*
    //Hide all elements, except the first register box
    ui.register2LineEdit->hide();
    ui.register3LineEdit->hide();
    ui.register4LineEdit->hide();
    ui.register5LineEdit->hide();
    ui.register6LineEdit->hide();
    ui.register7LineEdit->hide();
    ui.register8LineEdit->hide();
    delayMsSpinBox->hide();
    jumpToLineComboBox->hide();
    jumpXTimesSpinBox->hide();
    jumpXTimesComboBox->hide();
    numReadBytesSpinBox->hide();
    outputRegisterEndianessComboBox->hide();
    outputRegisterComboBox->hide();
    outputRangeLowComboBox->hide();
    outputRangeHighComboBox->hide();
    readText1Label->hide();
    readText2Label->hide();
    readText3Label->hide();
    readText4Label->hide();
    addReadOutputPushButton->hide();
    removeReadOutputPushButton->hide();

    //Connect widget signals to handler functions
    connect(ui.InstructionComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(switchCompilerInst(QString)));
    connect(ui.AddFieldPushButton, SIGNAL(clicked()), this, SLOT(addRegisterField()));
    connect(ui.RemoveFieldPushButton, SIGNAL(clicked()), this, SLOT(removeRegisterField()));
    connect(jumpXTimesComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(switchNumJumpTimes(QString)));
    connect(numReadBytesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(switchNumReadBytes(int)));

    //This is supposed to prevent collisions on Macs. DOESN'T SEEM TO WORK!
    numReadBytesSpinBox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui.InstructionComboBox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
*/
}

VMReadInstructionForm::~VMReadInstructionForm()
{
    // Do nothing
}

/**
 * Configure following fields based on the type of VM instruction.
 */
/*
void VMReadInstructionForm::switchCompilerInst(QString instruction)
{
    qDebug()<<"Switching compiler instruction to " << instruction << ".";

    //First, hide all elements
    numReadBytesSpinBox->hide();
    jumpToLineComboBox->hide();
    jumpXTimesComboBox->hide();
    jumpXTimesSpinBox->hide();
    delayMsSpinBox->hide();
    ui.AddFieldPushButton->hide();
    ui.RemoveFieldPushButton->hide();
    outputRegisterEndianessComboBox->hide();
    outputRegisterComboBox->hide();
    outputRangeLowComboBox->hide();
    outputRangeHighComboBox->hide();
    readText1Label->hide();
    readText2Label->hide();
    readText3Label->hide();
    readText4Label->hide();
    addReadOutputPushButton->hide();
    removeReadOutputPushButton->hide();

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
        readText1Label->show();
        readText2Label->show();
        readText3Label->show();
        readText4Label->show();

        numReadBytesSpinBox->show();
        outputRegisterEndianessComboBox->show();
        outputRegisterComboBox->show();
        outputRangeLowComboBox->show();
        outputRangeHighComboBox->show();
        addReadOutputPushButton->show();
        removeReadOutputPushButton->show();
    }
    else if(instruction=="Delay [ms]"){
        delayMsSpinBox->show();
    }
    else if(instruction=="Jump to line"){
        jumpToLineComboBox->show();
        jumpXTimesComboBox->show();
        switchNumJumpTimes(QString("Continuously"));
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

void VMReadInstructionForm::addRegisterField()
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

void VMReadInstructionForm::removeRegisterField(){
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


void VMReadInstructionForm::switchNumJumpTimes(QString jumpTimes){
    if (jumpXTimesComboBox->currentText() == "Exactly"){
        jumpXTimesSpinBox->show();
//        jumpXTimesSpinBox->setEnabled(true);
    }
    else{
        jumpXTimesSpinBox->hide();
        jumpXTimesSpinBox->setEnabled(false);
    }
    return;

}


void VMReadInstructionForm::switchNumReadBytes(int){
    QStringList bytesList;
    for (int i=0; i<numReadBytesSpinBox->value(); i++)
        bytesList.append(QString("%1").arg(i));

    QString tmpValLow=outputRangeLowComboBox->currentText();
    QString tmpValHigh=outputRangeHighComboBox->currentText();
    outputRangeLowComboBox->clear();
    outputRangeHighComboBox->clear();
    outputRangeLowComboBox->addItems(bytesList);
    outputRangeHighComboBox->addItems(bytesList);
    outputRangeLowComboBox->setCurrentIndex(outputRangeLowComboBox->findText(tmpValLow));
    outputRangeHighComboBox->setCurrentIndex(outputRangeHighComboBox->findText(tmpValHigh));

}


/*
 * Make this form instance aware of how many other instruction form instances exist.
 */
/*
void VMReadInstructionForm::setNumInstructions(int val){
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

QString VMReadInstructionForm::getInstructionType()
{
    return ui.InstructionComboBox->currentText();
}

/*
 * Compute the relative jump. A negative jump implies backwards movement, vice-versa for a positive jump
 */
/*
void VMReadInstructionForm::getJumpInstruction(int *relativeJump, int *numJumps)
{
    *relativeJump = jumpToLineComboBox->currentText().toInt() - m_index;

    if (jumpXTimesComboBox->currentText() == "Exactly")
        *numJumps=jumpXTimesSpinBox->value();
    else
        *numJumps=-1;
    return;
}

void VMReadInstructionForm::getReadInstruction(int*numReadBytes)
{
    *numReadBytes=numReadBytesSpinBox->value();

    return;
}

void VMReadInstructionForm::getWriteInstruction(vector<int> *val)
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

int VMReadInstructionForm::getDelayInstruction()
{
    return delayMsSpinBox->value();
}

void VMReadInstructionForm::setHexRepresentation(bool val)
{
    isHex=val;
}

*/

void VMReadInstructionForm::setNumReadBytes(int numBytes){
    QStringList bytesList;
    for (int i=0; i<numBytes; i++)
        bytesList.append(QString("%1").arg(i));

    QString tmpValLow=ui.outputRangeLowComboBox->currentText();
    QString tmpValHigh=ui.outputRangeHighComboBox->currentText();
    ui.outputRangeLowComboBox->clear();
    ui.outputRangeHighComboBox->clear();
    ui.outputRangeLowComboBox->addItems(bytesList);
    ui.outputRangeHighComboBox->addItems(bytesList);
    ui.outputRangeLowComboBox->setCurrentIndex(ui.outputRangeLowComboBox->findText(tmpValLow));
    ui.outputRangeHighComboBox->setCurrentIndex(ui.outputRangeHighComboBox->findText(tmpValHigh));

}

void VMReadInstructionForm::getReadOutputInstructions(vector<int> *valInt, vector<QString> *valStr){
    valInt->clear();
    valStr->clear();

    valInt->push_back(ui.outputRangeLowComboBox->currentText().toInt());
    valInt->push_back(ui.outputRangeHighComboBox->currentText().toInt());
    valStr->push_back(ui.outputRegisterComboBox->currentText());
    valStr->push_back(ui.outputRegisterEndianessComboBox->currentText());

}
