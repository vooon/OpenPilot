TEMPLATE = lib

CONFIG *= qt3d

TARGET = ModelViewQt3DGadget
include(../../openpilotgcsplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(modelviewqt3d_dependencies.pri)

HEADERS += modelviewqt3dplugin.h \
    modelviewqt3dgadgetconfiguration.h \
    modelviewqt3dgadget.h \
    modelviewqt3dgadgetwidget.h \
    modelviewqt3dgadgetfactory.h \
    modelviewqt3dgadgetoptionspage.h \
    modelviewqt3d.h

SOURCES += modelviewqt3dplugin.cpp \
    modelviewqt3dgadgetconfiguration.cpp \
    modelviewqt3dgadget.cpp \
    modelviewqt3dgadgetfactory.cpp \
    modelviewqt3dgadgetwidget.cpp \
    modelviewqt3dgadgetoptionspage.cpp \
    modelviewqt3d.cpp

OTHER_FILES += ModelViewQt3DGadget.pluginspec

FORMS += modelviewqt3doptionspage.ui

RESOURCES += modelviewqt3d.qrc




