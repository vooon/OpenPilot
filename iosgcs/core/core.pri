
SOURCES +=
HEADERS +=

#common
SOURCES += \
    ../core/src/logger.cpp \
    ../core/src/core/cobject.cpp
HEADERS += \
    ../core/src/optypes.h \
    ../core/src/op_errors.h \
    ../core/src/logger.h \
    ../core/src/core/cobject.h

#core
SOURCES += \
    ../core/src/core/cvariant.cpp \
    ../core/src/core/cbytearray.cpp \
    ../core/src/core/capplication.cpp
HEADERS += \
    ../core/src/core/cvariant.h \
    ../core/src/core/cbytearray.h \
    ../core/src/core/capplication.h

#io
SOURCES += \
    ../core/src/io/ciodevice.cpp \
    ../core/src/io/cfile.cpp
HEADERS += \
    ../core/src/io/ciodevice.h \
    ../core/src/io/cfile.h

#system
SOURCES += \
    ../core/src/system/cexception.cpp \
    ../core/src/system/csystem.cpp
HEADERS += \
    ../core/src/system/weakptr.h \
    ../core/src/system/uniqueptr.h \
    ../core/src/system/sharedptr.h \
    ../core/src/system/cexception.h \
    ../core/src/system/csystem.h \
    ../core/src/system/ctime.h

win* {
SOURCES += ../core/src/system/ctime_windows.cpp
} else {
SOURCES += ../core/src/system/ctime_unix.cpp
}

#text
SOURCES += \
    ../core/src/text/cstringlist.cpp \
    ../core/src/text/cstring.cpp \
    ../core/src/text/cstring_p.cpp \
    ../core/src/text/unicode/gutf8.cpp \
    ../core/src/text/unicode/guniprop.cpp \
    ../core/src/text/unicode/gunidecomp.cpp
HEADERS += \
    ../core/src/text/cstringlist.h \
    ../core/src/text/cstring.h \
    ../core/src/text/cstring_p.h \
    ../core/src/text/unicode/gunidecomp.h \
    ../core/src/text/unicode/gunicode.h \
    ../core/src/text/unicode/gunichartables.h \

#thread
SOURCES += \
    ../core/src/thread/cmutex.cpp \
    ../core/src/thread/thread.cpp \
    ../core/src/thread/runnable.cpp
HEADERS += \
    ../core/src/thread/cmutex.h \
    ../core/src/thread/thread.h \
    ../core/src/thread/thread_impl.h \
    ../core/src/thread/runnable.h

win* {
} else {
SOURCES += \
    ../core/src/thread/unix/thread_unix.cpp
HEADERS += \
    ../core/src/thread/unix/thread_unix.h
}

#templ
HEADERS += \
    ../core/src/templ/opvector.h \
    ../core/src/templ/opmap.h \
    ../core/src/templ/oplist.h

#signals
HEADERS += \
    ../core/src/signals/virtual_function_v5.h \
    ../core/src/signals/virtual_function_v4.h \
    ../core/src/signals/virtual_function_v3.h \
    ../core/src/signals/virtual_function_v2.h \
    ../core/src/signals/virtual_function_v1.h \
    ../core/src/signals/virtual_function_v0.h \
    ../core/src/signals/virtual_function_5.h \
    ../core/src/signals/virtual_function_4.h \
    ../core/src/signals/virtual_function_3.h \
    ../core/src/signals/virtual_function_2.h \
    ../core/src/signals/virtual_function_1.h \
    ../core/src/signals/virtual_function_0.h \
    ../core/src/signals/slot.h \
    ../core/src/signals/slot_container.h \
    ../core/src/signals/signals_impl.h \
    ../core/src/signals/signalqueue.h \
    ../core/src/signals/signal_v5.h \
    ../core/src/signals/signal_v4.h \
    ../core/src/signals/signal_v3.h \
    ../core/src/signals/signal_v2.h \
    ../core/src/signals/signal_v1.h \
    ../core/src/signals/signal_v0.h \
    ../core/src/signals/callback_v5.h \
    ../core/src/signals/callback_v4.h \
    ../core/src/signals/callback_v3.h \
    ../core/src/signals/callback_v2.h \
    ../core/src/signals/callback_v1.h \
    ../core/src/signals/callback_v0.h \
    ../core/src/signals/callback_5.h \
    ../core/src/signals/callback_4.h \
    ../core/src/signals/callback_3.h \
    ../core/src/signals/callback_2.h \
    ../core/src/signals/callback_1.h \
    ../core/src/signals/callback_0.h \

# os
# system dependent classes

SOURCES += \
    ../core/src/os/cloggeros.cpp \
    ../core/src/os/cloggeros_qt.cpp
HEADERS += \
    ../core/src/os/in_common.h \
    ../core/src/os/cloggeros.h \
    ../core/src/os/cloggeros_qt.h

#includes

HEADERS += \
    ../core/include/OPVector \
    ../core/include/OPTypes \
    ../core/include/OPMap \
    ../core/include/OPList \
    ../core/include/Logger \
\
    ../core/include/core/CVariant \
    ../core/include/core/CByteArray \
    ../core/include/core/CApplication \
\
    ../core/include/io/CIODevice \
    ../core/include/io/CFile \
    ../core/include/signals/Signals \
    ../core/include/signals/Callback \
\
    ../core/include/system/WeakPtr \
    ../core/include/system/UniquePtr \
    ../core/include/system/SharedPtr \
    ../core/include/system/CSystem \
    ../core/include/system/CTime \
\
    ../core/include/text/CStringList \
    ../core/include/text/CString \
\
    ../core/include/thread/CMutex \
    ../core/include/thread/CThread
