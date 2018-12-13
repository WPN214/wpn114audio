TARGET = WPN114-audio
TEMPLATE = lib
CONFIG += c++11 dll
QT += quick

#message(Qt version: $$[QT_VERSION])
#message(Qt is installed in $$[QT_INSTALL_PREFIX])
#message(WPN114-QML-MODULE will be installed at: $$[QT_INSTALL_QML]/WPN114/Audio)

QMLDIR_FILES += $$PWD/qml/qmldir
DESTDIR = $$[QT_INSTALL_QML]/WPN114/Audio

for(FILE,QMLDIR_FILES) {
    QMAKE_POST_LINK += $$quote(cp $${FILE} $${DESTDIR}$$escape_expand(\n\t))
}

include($$PWD/wpn114-audio.pri)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
