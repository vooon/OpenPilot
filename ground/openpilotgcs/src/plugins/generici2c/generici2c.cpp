/**
 ******************************************************************************
 *
 * @file       generici2c.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GenericI2CPlugin
 * @{
 * @brief A place holder gadget plugin
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
#include "generici2c.h"
#include "generici2cwidget.h"
#include "../../../../../shared/lib/gen_i2c_vm.h" //<------ICK, UGLY

#include "extensionsystem/pluginmanager.h"
//#include "uavobjectmanager.h"
//#include "uavobject.h"
//#include "uavobjectfield.h"
//#include "uavobjectutilmanager.h"

#include "generici2csensorsettings.h"
#include <QDebug>

using namespace std;


GenericI2C::GenericI2C(QString classId, GenericI2CWidget *widget, QWidget *parent) :
    IUAVGadget(classId, parent),
    m_widget(widget)
{
}

GenericI2C::~GenericI2C()
{
    delete m_widget;
}

void generateVmCode(int i2c_addr, vector<VMInstructionForm*> formList)
{

    qDebug() << "Loading UAVO for I2C compiler";

    //Get UAVO
    ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager* objManager = pm->getObject<UAVObjectManager>();

    GenericI2CSensorSettings *genericI2CSensorSettings = GenericI2CSensorSettings::GetInstance(objManager);
    Q_ASSERT(genericI2CSensorSettings);
    GenericI2CSensorSettings::DataFields genericI2CSensorSettingsData = genericI2CSensorSettings->getData();

    UAVObjectField *programField=genericI2CSensorSettings->getField("Program");

    //Compile virtual machine program
    qDebug() << "Processing " << formList.size() << " instructions.";

    //Determine program length
    int programLength=formList.size();

    vector<uint32_t> program;

    //Add setting I2C address as first operation
    program.push_back(I2C_VM_ASM_SET_DEV_ADDR(i2c_addr));

    for (int i=0; i<programLength; i++){
        qDebug() << "Instruction to generic I2C compiler: " << formList[i]->getInstructionType();
        if(formList[i]->getInstructionType()=="Delay [ms]"){
            qDebug() << "Wait " << formList[i]->getDelayInstruction() << " ms.";
            program.push_back(I2C_VM_ASM_DELAY(formList[i]->getDelayInstruction()));
        }
        if(formList[i]->getInstructionType()=="Read"){
            int numReadBytes;
            formList[i]->getReadInstruction(&numReadBytes);

            qDebug() << "Reading " << numReadBytes << " bytes.";

            program.push_back(I2C_VM_ASM_READ_I2C(numReadBytes));   //Read bytes

            program.push_back(I2C_VM_ASM_LOAD_LE(0, 2, VM_R0)); //Load formatted bytes into first output register
            program.push_back(I2C_VM_ASM_LOAD_LE(2, 2, VM_R1)); //Load formatted bytes into second output register
        }
        if(formList[i]->getInstructionType()=="Write"){
            vector<int> val;
            formList[i]->getWriteInstruction(&val);

            qDebug() << "Writing " << val.size() << " bytes.";


            for (int j=0; j< val.size(); j++){
                qDebug()<< val[j];
                program.push_back(I2C_VM_ASM_STORE(val[j], j)); //Store register address
            }
            program.push_back(I2C_VM_ASM_WRITE_I2C(val.size())); //Write bytes
        }
        if(formList[i]->getInstructionType()=="Jump to line"){
            qDebug() << "Relative jump of " << formList[i]->getJumpInstruction() << " instructions.";
            program.push_back(I2C_VM_ASM_JUMP(formList[i]->getJumpInstruction()));
        }
    }

    if (program.size() > programField->getNumElements()){
        //THROW ERROR OF SOME KIND
        qDebug() << "Program too long!";
        return;
    }

    qDebug() << "Saving I2C compiler to UAVO";
    for (unsigned int i=0; i<program.size(); i++){
        genericI2CSensorSettingsData.Program[i]=program[i];
    }
    genericI2CSensorSettings->setData(genericI2CSensorSettingsData);
}

/*
I2C_VM_ASM_NOP(), //Do nothing

I2C_VM_ASM_SET_CTR(10), //Set counter to 10
I2C_VM_ASM_SET_DEV_ADDR(0x53), //Set I2C device address (in 7-bit)
I2C_VM_ASM_STORE(0x2D, 0), //Store register address
I2C_VM_ASM_STORE(0x08, 1), //Store register value
I2C_VM_ASM_WRITE_I2C(2), //Write two bytes
I2C_VM_ASM_DEC_CTR(), //Decrement counter
I2C_VM_ASM_BNZ(-4), //If the counter is not zero, go back four steps

I2C_VM_ASM_SET_DEV_ADDR(0x53), //Set I2C device address (in 7-bit)
I2C_VM_ASM_STORE(0x32, 0), //Store register address
I2C_VM_ASM_WRITE_I2C(1),  //Write one bytes
I2C_VM_ASM_DELAY(50), //Delay 50ms
I2C_VM_ASM_READ_I2C(6),   //Read six bytes
I2C_VM_ASM_LOAD_LE(0, 2, VM_R0), //Load formatted bytes into first output register
I2C_VM_ASM_LOAD_LE(2, 2, VM_R1), //Load formatted bytes into second output register

I2C_VM_ASM_SET_DEV_ADDR(0x27), //Set I2C device address (in 7-bit)

I2C_VM_ASM_SEND_UAVO(), //Set the UAVObjects
I2C_VM_ASM_JUMP(-9),    //Jump back 9 steps
*/
