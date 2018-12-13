#include "qml_plugin.hpp"

#include <source/audio.hpp>
#include <source/soundfile.hpp>

#include <QQmlEngine>
#include <qqml.h>

void qml_plugin::registerTypes(const char *uri)
{
    Q_UNUSED    ( uri );

    qmlRegisterUncreatableType<StreamNode, 1> ( "WPN114.Audio", 1, 0, "StreamNode","Uncreatable");
    qmlRegisterType<WorldStream, 1> ( "WPN114.Audio", 1, 0, "AudioStream" );
}
