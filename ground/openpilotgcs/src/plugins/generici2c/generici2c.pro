TEMPLATE = lib
TARGET = GenericI2C

#QT += svg
#QT += opengl

include(../../openpilotgcsplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)


HEADERS += generici2cplugin.h \
    generici2c.h \
    generici2cfactory.h \
    generici2cwidget.h \
    generici2cconfiguration.h \
    vminstructionform.h \
    vmreadinstructionform.h

SOURCES += generici2cplugin.cpp \
    generici2c.cpp \
    generici2cfactory.cpp \
    generici2cwidget.cpp \
    generici2cconfiguration.cpp \
    vminstructionform.cpp \
    vmreadinstructionform.cpp

FORMS += generici2c.ui \
    vminstructionform.ui \
    vmreadinstructionform.ui

OTHER_FILES += GenericI2C.pluginspec

