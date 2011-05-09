
TEMPLATE = lib 
TARGET = DView
 
include(../../openpilotgcsplugin.pri) 
include(../../plugins/coreplugin/coreplugin.pri) 
include(../../plugins/uavobjects/uavobjects.pri)
 
HEADERS += dviewplugin.h
HEADERS += dviewgadget.h
HEADERS += dviewwidget.h
HEADERS += dviewgadgetfactory.h
HEADERS += dviewgadgetconfiguration.h
#HEADERS += dviewgadgetoptionspage.h
HEADERS += telemetryparser.h
HEADERS += gpsparser.h

SOURCES += dviewplugin.cpp
SOURCES += dviewgadget.cpp
SOURCES += dviewgadgetfactory.cpp
SOURCES += dviewwidget.cpp
SOURCES += dviewgadgetconfiguration.cpp
#SOURCES += dviewgadgetoptionspage.cpp
SOURCES += telemetryparser.cpp
SOURCES += gpsparser.cpp

OTHER_FILES += DView.pluginspec \
    dview.pluginspec

FORMS += dviewgadgetoptionspage.ui
FORMS += dviewwidget.ui

#RESOURCES += widgetresources.qrc
