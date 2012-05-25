/**
 ******************************************************************************
 *
 * @file       generici2cwidget.cpp
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
#include "generici2cwidget.h"
#include "generici2cfactory.h"

#include <QDebug>
#include <QStringList>
#include <QtGui/QWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

GenericI2CWidget::GenericI2CWidget(QWidget *parent) : QLabel(parent)
{
    m_widget = new Ui_GenericI2CWidget();
    m_widget->setupUi(this);

    m_widget->hexDexComboBox->addItem("Hexadecimal");
    m_widget->hexDexComboBox->addItem("Decimal");

//    setMinimumSize(64,64);
//    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
//    this->
//    setText(tr("Choose a gadget to display in this view.\n") +
//            tr("You can also split this view in two.\n\n") +
//            tr("Maybe you first have to choose Edit Gadgets Mode in the Window menu."));
}

GenericI2CWidget::~GenericI2CWidget()
{
    // Do nothing
}

