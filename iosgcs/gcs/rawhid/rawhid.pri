HEADERS += \
    ../gcs/rawhid/src/rawhid.h \
    ../gcs/rawhid/src/usbmonitor.h \
    ../gcs/rawhid/src/pjrc_rawhid.h

SOURCES += \
    ../gcs/rawhid/src/pjrc_common.cpp \
    ../gcs/rawhid/src/rawhid.cpp

mac* {
SOURCES += \
    ../gcs/rawhid/src/pjrc_rawhid_mac.cpp

OBJECTIVE_SOURCES += ../gcs/rawhid/src/usbmonitor_mac.mm
}

HEADERS += \
    ../gcs/rawhid/include/UsbMonitor

