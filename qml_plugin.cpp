#include "qml_plugin.hpp"
#include <source/qtwrapper.hpp>
#include <QQmlEngine>
#include <qqml.h>

void qml_plugin::registerTypes(const char *uri)
{
    Q_UNUSED    ( uri );

    //=============================================================================================
    // UNCREATABLE
    //=============================================================================================

    qmlRegisterUncreatableType<Dispatch, 1>
    ("WPN114.Audio", 1, 1, "Dispatch", "Uncreatable");

    qmlRegisterUncreatableType<node, 1>
    ("WPN114.Audio", 1, 1, "Node", "Uncreatable");

    qmlRegisterUncreatableType<signal, 1>
    ("WPN114.Audio", 1, 1, "Signal", "Uncreatable");

    qmlRegisterUncreatableType<pin, 1>
    ("WPN114.Audio", 1, 1, "Pin", "Uncreatable");

    qmlRegisterUncreatableType<connection, 1>
    ("WPN114.Audio", 1, 1, "Connection", "Uncreatable");

    //=============================================================================================
    // MODULES
    //=============================================================================================

    qmlRegisterType<Output, 1>("WPN114.Audio", 1, 1, "Audiostream");
    qmlRegisterType<Sinetest, 1>("WPN114.Audio", 1, 1, "Sinetest");
    qmlRegisterType<VCA, 1>("WPN114.Audio", 1, 1, "VCA");
}
