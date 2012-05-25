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
#include <vector>

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

void GenericI2C::GenerateVmCode()
{
    //Determine program length
    int programLength;


    vector<uint32_t> program;
    program.clear();
/*
    program.at(1)=I2C_VM_ASM_SET_DEV_ADDR(m_widget->I2CAddressLineEdit);


//    foreach(QComboBox *combobox, this->findChildren<QComboBox*>(QRegExp("\\S+ChannelBo\\S+")))//FOR WHATEVER REASON, THIS DOES NOT WORK WITH ChannelBox. ChannelBo is sufficiently accurate
//	{
//		combobox->addItems(channels);
//	}
    for (int i=1; i<programLength; i++){
        if(Line[i]=="Wait"){
            program.push_back(I2C_VM_ASM_DELAY(50));
        }
        if(Line[i]=="Read"){
            program.push_back(I2C_VM_ASM_READ_I2C(6));   //Read six bytes
            program.push_back(I2C_VM_ASM_LOAD_LE(0, 2, VM_R0)); //Load formatted bytes into first output register
            program.push_back(I2C_VM_ASM_LOAD_LE(2, 2, VM_R1)); //Load formatted bytes into second output register
        }
        if(Line[i]=="Write"){
            program.push_back(I2C_VM_ASM_STORE(0x2D, 0)); //Store register address
            program.push_back(I2C_VM_ASM_STORE(0x08, 1)); //Store register value
            program.push_back(I2C_VM_ASM_WRITE_I2C(1));
        }
        if(Line[i]=="Loop"){
            program.push_back(I2C_VM_ASM_JUMP(-9));
        }
    }

    if (program.size() > MAX_PRGM_SIZE){
        //THROW ERROR OF SOME KIND
        return;
    }
*/
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
