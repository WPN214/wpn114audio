TARGET = WPN114-audio
TEMPLATE = lib
CONFIG += c++14 dll
QT += quick

QMAKE_CFLAGS += -std=c11

OTHER_FILES += $$PWD/qml/qmldir
OTHER_FILES += $$PWD/qml/audio.qmltypes

macx: QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/

target.path = $$[QT_INSTALL_QML]/WPN114/Audio
dist.path = $$[QT_INSTALL_QML]/WPN114/Audio
dist.files = $$PWD/qml/*

INSTALLS += target dist

macx {
    LIBS += -framework CoreFoundation
    LIBS += -framework CoreAudio
    DEFINES += __MACOSX_CORE__
}

linux {
    DEFINES += __LINUX_ALSA__
    DEFINES += __LINUX_PULSE__
    DEFINES += __UNIX_JACK__
    LIBS += -lpthread -ljack -lasound -lpulse-simple
}

HEADERS += $$PWD/external/rtaudio/RtAudio.h
HEADERS += $$PWD/external/wpn-c/source/stream.h
HEADERS += $$PWD/external/wpn-c/source/graph.h
HEADERS += $$PWD/external/wpn-c/source/utilities.h
HEADERS += $$PWD/source/qtwrapper.hpp

SOURCES += $$PWD/external/rtaudio/RtAudio.cpp
SOURCES += $$PWD/external/wpn-c/source/stream.c
SOURCES += $$PWD/external/wpn-c/source/graph.c
SOURCES += $$PWD/external/wpn-c/source/utilities.c
SOURCES += $$PWD/source/qtwrapper.cpp

SOURCES += $$PWD/qml_plugin.cpp
HEADERS += $$PWD/qml_plugin.hpp

DEFINES += WPN114_EXTERN_DEF_DOUBLE_PRECISION
#DEFINES += WPN114_EXTERN_DEF_SINGLE_PRECISION

DISTFILES += examples/Audio.qml
