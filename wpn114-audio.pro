TARGET = WPN114-audio
TEMPLATE = lib
CONFIG += c++11 dll
QT += quick

localmod: DESTDIR = $$QML_MODULE_DESTDIR/WPN114/Audio
else: DESTDIR = $$[QT_INSTALL_QML]/WPN114/Audio
QMLDIR_FILES += $$PWD/qml/qmldir

for(FILE,QMLDIR_FILES) {
    QMAKE_POST_LINK += $$quote(cp $${FILE} $${DESTDIR}$$escape_expand(\n\t))
}

macx {
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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
