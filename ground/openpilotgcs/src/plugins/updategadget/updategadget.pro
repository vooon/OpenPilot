TEMPLATE = lib 
TARGET = UpdateGadget

include(../../openpilotgcsplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri) 
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavtalk/uavtalk.pri)

QT += network \
    xml

HEADERS += updateplugin.h \
    downloadpage.h \
    updatelogic.h \
    infodialog.h \
    updategadget.h \
    updategadgetwidget.h \
    updategadgetfactory.h

SOURCES += updateplugin.cpp \
    downloadpage.cpp \
    updatelogic.cpp \
    infodialog.cpp \
    updategadget.cpp \
    updategadgetfactory.cpp \
    updategadgetwidget.cpp

OTHER_FILES += UpdateGadget.pluginspec

FORMS += updategadgetwidget.ui \
    infodialog.ui
