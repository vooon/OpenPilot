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
    vminstructionform.h
SOURCES += generici2cplugin.cpp \
    generici2c.cpp \
    generici2cfactory.cpp \
    generici2cwidget.cpp \
    vminstructionform.cpp


FORMS += generici2c.ui \
    vminstructionform.ui

OTHER_FILES += GenericI2C.pluginspec

