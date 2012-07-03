TEMPLATE  = subdirs
CONFIG   += ordered

QMAKE_CXXFLAGS_DEBUG += -pg
QMAKE_LFLAGS_DEBUG += -pg

SUBDIRS   = \
    qscispinbox\
    qtconcurrent \
    aggregation \
    extensionsystem \
    utils \
    opmapcontrol \
    qwt \
    qextserialport \
    glc_lib\
    sdlgamepad \
    libqxt

SUBDIRS +=
