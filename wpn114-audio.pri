DEFINES += QT_DEPRECATED_WARNINGS

macx {
    QMAKE_MAC_SDK = macosx10.14
    LIBS +=  \
    -framework CoreFoundation \
    -framework CoreAudio
    DEFINES += __MACOSX_CORE__
}

linux {
    DEFINES += __LINUX_ALSA__
    DEFINES += __UNIX_JACK__
}

HEADERS += $$PWD/external/rtaudio/RtAudio.h
HEADERS += $$PWD/source/audio.hpp
HEADERS += $$PWD/source/soundfile.hpp
SOURCES += $$PWD/external/rtaudio/RtAudio.cpp
SOURCES += $$PWD/source/audio.cpp
SOURCES += $$PWD/source/soundfile.cpp

SOURCES += $$PWD/qml_plugin.cpp
HEADERS += $$PWD/qml_plugin.hpp
