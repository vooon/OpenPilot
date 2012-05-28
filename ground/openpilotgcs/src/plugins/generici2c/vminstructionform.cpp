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
#include "generici2csensor.h"
#include "extensionsystem/pluginmanager.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QMessageBox>

VMInstructionForm::VMInstructionForm(const int index, QWidget *parent) :
    QWidget(parent),
    ui(),
    m_index(index),
    unhideIdx(1),
    readInstrctIdx(0)
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
#endif
//    ui.gridLayout->addWidget(addReadOutputPushButton, 0, 5);
//    ui.gridLayout->addWidget(removeReadOutputPushButton, 0, 6);

    //------------------------//
    // Set element parameters //
    //------------------------//
    QStringList instructionTypes;
    instructionTypes << "None" << "Delay [ms]" << "Write" << "Read" << "Jump to line" << "Send UAVO" << "Delete line";
    ui.InstructionComboBox->addItems(instructionTypes);
    ui.InstructionComboBox->setCurrentIndex(0);

    QStringList jumpXTimesList;
    jumpXTimesList << "Continuously" << "Exactly";
    jumpXTimesComboBox->addItems(jumpXTimesList);
    jumpXTimesComboBox->setCurrentIndex(0);

//    ui.addReadOutputPushButton->setText("+");
//    ui.removeReadOutputPushButton->setText("-");

    numReadBytesSpinBox->setSuffix(" bytes");
    numReadBytesSpinBox->setMaximum(255); //8-bit container

    jumpXTimesSpinBox->setSuffix(" times");
    jumpXTimesSpinBox->setMaximum(255); //8-bit container

    //---------------//
    // Hide elements //
    //---------------//

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
    ui.addReadOutputPushButton->hide();
    ui.removeReadOutputPushButton->hide();

    //Connect widget signals to handler functions
    connect(ui.InstructionComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(switchCompilerInst(QString)));
    connect(ui.AddFieldPushButton, SIGNAL(clicked()), this, SLOT(addRegisterField()));
    connect(ui.RemoveFieldPushButton, SIGNAL(clicked()), this, SLOT(removeRegisterField()));
    connect(ui.addReadOutputPushButton, SIGNAL(clicked()), this, SLOT(addAdditionalReadInstructionLine()));
    connect(ui.removeReadOutputPushButton, SIGNAL(clicked()), this, SLOT(removeAdditionalReadInstructionLine()));
    connect(jumpXTimesComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(switchNumJumpTimes(QString)));
    connect(numReadBytesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(switchNumReadBytes(int)));

    //This is supposed to prevent collisions on Macs. DOESN'T SEEM TO WORK!
    numReadBytesSpinBox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui.InstructionComboBox->setAttribute(Qt::WA_LayoutUsesWidgetRect);

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
    jumpXTimesComboBox->hide();
    jumpXTimesSpinBox->hide();
    delayMsSpinBox->hide();
    ui.AddFieldPushButton->hide();
    ui.RemoveFieldPushButton->hide();
    ui.addReadOutputPushButton->hide();
    ui.removeReadOutputPushButton->hide();
    for (int i=0; i< readFormList.size(); i++){
        readFormList[i]->hide();
    }

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
        numReadBytesSpinBox->show();
        ui.addReadOutputPushButton->show();
        ui.removeReadOutputPushButton->show();
        if (readFormList.size()==0){
            addAdditionalReadInstructionLine();
        }
        else{
            for (int i=0; i< readFormList.size(); i++){
                readFormList[i]->show();
            }
        }

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
        QMessageBox msgBox;
//        msgBox.setInformativeText(tr("<p>Are you sure you want to delete this line?</p><p>All associated data will be lost.</p>"));
        msgBox.setText(tr("Are you sure you want to delete this line?"));
        msgBox.setInformativeText(tr("All associated data will be lost."));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        int ret = msgBox.exec();

        if (ret == QMessageBox::No)
        {
            return;
        }

        //Then, remove GUI line
//        if(m_index!=0)
//            delete this;

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


void VMInstructionForm::switchNumJumpTimes(QString jumpTimes){
    if (jumpXTimesComboBox->currentText() == "Exactly"){
        jumpXTimesSpinBox->show();
//        jumpXTimesSpinBox->setEnabled(true);
    }
    else{
        jumpXTimesSpinBox->hide();
//        jumpXTimesSpinBox->setEnabled(false);
    }
    return;

}

/*
 * Sets the byte number comboboxes in the Read Instruction Form
 */

void VMInstructionForm::switchNumReadBytes(int){
    for (int i=0; i< readFormList.size(); i++){
        readFormList[i]->setNumReadBytes(numReadBytesSpinBox->value());
    }

}


void VMInstructionForm::addAdditionalReadInstructionLine(){
    qDebug()<<"Add read instruction line";
    VMReadInstructionForm *readForm = new VMReadInstructionForm(readInstrctIdx, this);

    //Add readForm to widget
//    ui.gridLayout->addWidget(readForm, readInstrctIdx+1, 1, 20, 1);
    ui.gridLayout->addWidget(readForm);
    readForm->setNumReadBytes(numReadBytesSpinBox->value());


    //Add readForm to array for future use
    readFormList.push_back(readForm);
//    for (int i=0; i< readFormList.size(); i++){
////        readFormList[i]->setNumReadBytes(numReadBytesSpinBox->value());
//    }

    readInstrctIdx++;
}

void VMInstructionForm::removeAdditionalReadInstructionLine(){
    qDebug()<<"Remove read instruction line";
    if (readFormList.size() >1){
        ui.gridLayout->removeWidget(readFormList.back());
        delete readFormList.back();

        //Add readForm to array for future use
        readFormList.pop_back();

        readInstrctIdx--;
    }
}


/*
 * Make this form instance aware of how many other instruction form instances exist.
 */
void VMInstructionForm::setNumInstructions(int val){
    numInstructions=val;

    //Set the jump table combo box to reflect the new number of items
    QStringList jumpInstructionList;
    for (int i=0; i<numInstructions; i++)
        jumpInstructionList.append(QString("%1").arg(i));

    int tmpVal=jumpToLineComboBox->currentIndex();
    jumpToLineComboBox->clear();
    jumpToLineComboBox->addItems(jumpInstructionList);
    jumpToLineComboBox->setCurrentIndex(tmpVal);
}

QString VMInstructionForm::getInstructionType()
{
    return ui.InstructionComboBox->currentText();
}

/*
 * Compute the relative jump. A negative jump implies backwards movement, vice-versa for a positive jump
 */
void VMInstructionForm::getJumpInstruction(int *relativeJump, int *numJumps)
{
    *relativeJump = jumpToLineComboBox->currentText().toInt() - m_index;

    if (jumpXTimesComboBox->currentText() == "Exactly")
        *numJumps=jumpXTimesSpinBox->value();
    else
        *numJumps=-1;
    return;
}

void VMInstructionForm::getReadInstruction(int*numReadBytes)
{
    *numReadBytes=numReadBytesSpinBox->value();

    return;
}

void VMInstructionForm::getOutputInstruction(vector<int> *valIntOut, vector<QString> *valStrOut)
{
    vector<int> valInt;
    vector<QString> valStr;
    for (int i=0; i< readFormList.size(); i++){
        readFormList[i]->getReadOutputInstructions(&valInt, &valStr);
        valIntOut->insert(valIntOut->end(), valInt.begin(), valInt.end() );
        valStrOut->insert(valStrOut->end(), valStr.begin(), valStr.end() );
    }
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

