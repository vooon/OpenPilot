TEMPLATE = lib
TARGET = StreamingGadget
DEFINES += STREAMING_LIBRARY
QT += svg
include(../../openpilotgcsplugin.pri)
include(streaming_dependencies.pri)
HEADERS += streamingplugin.h \
    stream.h \
    streaminggadget.h

SOURCES += streamingplugin.cpp \
    stream.cpp \
    streaminggadget.cpp
OTHER_FILES += StreamingGadget.pluginspec
