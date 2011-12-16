
TEMPLATE = lib 
TARGET = NotifyPlugin 
 
include(../../openpilotgcsplugin.pri) 
include(../../plugins/coreplugin/coreplugin.pri) 
include(notifyplugin_dependencies.pri)

QT        += phonon

HEADERS += notifyplugin.h \  
    notifypluginoptionspage.h \
    notifyitemdelegate.h \
    notifytablemodel.h \
    notificationitem.h \
    notifylogging.h \
    notifyaudiowrapper.h

SOURCES += notifyplugin.cpp \  
    notifypluginoptionspage.cpp \
    notifyitemdelegate.cpp \
    notifytablemodel.cpp \
    notificationitem.cpp \
    notifylogging.cpp \
    notifyaudiowrapper.cpp
 
OTHER_FILES += NotifyPlugin.pluginspec

FORMS += \
    notifypluginoptionspage.ui

RESOURCES += \
    res.qrc




