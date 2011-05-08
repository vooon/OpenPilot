TEMPLATE = lib
TARGET = UAVSettingsManagerGadget
DEFINES += UAVSETTINGSMANAGER_LIBRARY
QT += xml
include(../../openpilotgcsplugin.pri)
include(uavsettingsmanager_dependencies.pri)
HEADERS += uavsettingsmanagerplugin.h \
    uavsettingsmanagergadgetwidget.h \
    uavsettingsmanagerdialog.h
HEADERS += uavsettingsmanagergadget.h
HEADERS += uavsettingsmanagergadgetfactory.h
HEADERS += uavsettingsmanagergadgetconfiguration.h
HEADERS += uavsettingsmanagergadgetoptionspage.h
SOURCES += uavsettingsmanagerplugin.cpp \
    uavsettingsmanagergadgetwidget.cpp \
    uavsettingsmanagerdialog.cpp
SOURCES += uavsettingsmanagergadget.cpp
SOURCES += uavsettingsmanagergadgetfactory.cpp
SOURCES += uavsettingsmanagergadgetconfiguration.cpp
SOURCES += uavsettingsmanagergadgetoptionspage.cpp
OTHER_FILES += UAVSettingsManagerGadget.pluginspec
FORMS += uavsettingsmanagergadgetoptionspage.ui \
    uavsettingsmanagergadgetwidget.ui \
    uavsettingsmanagerdialog.ui
