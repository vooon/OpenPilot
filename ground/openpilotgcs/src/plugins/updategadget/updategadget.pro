TEMPLATE = lib 
TARGET = UpdateGadget

include(../../openpilotgcsplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri) 
include(../../plugins/uavobjectutil/uavobjectutil.pri)

HEADERS += updateplugin.h \
    downloadpage.h \
    updatelogic.h \
    infodialog.h
HEADERS += updategadget.h
HEADERS += updategadgetwidget.h
HEADERS += updategadgetfactory.h
SOURCES += updateplugin.cpp \
    downloadpage.cpp \
    updatelogic.cpp \
    infodialog.cpp
SOURCES += updategadget.cpp
SOURCES += updategadgetfactory.cpp
SOURCES += updategadgetwidget.cpp

OTHER_FILES += UpdateGadget.pluginspec

FORMS += \
    updategadgetwidget.ui \
    infodialog.ui

QT       += network
QT += xml
