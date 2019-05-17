#include "qml_plugin.hpp"
#include <source/qtwrapper.hpp>
#include <QQmlEngine>
#include <qqml.h>

void qml_plugin::registerTypes(const char* uri)
{
    Q_UNUSED(uri)

    qmlRegisterUncreatableType<Dispatch, 1>
    ("WPN114.Audio", 1, 1, "Dispatch", "Uncreatable");

    qmlRegisterUncreatableType<Node, 1>
    ("WPN114.Audio", 1, 1, "Node", "Uncreatable");

    qmlRegisterUncreatableType<Socket, 1>
    ("WPN114.Audio", 1, 1, "Socket", "Uncreatable");

    qmlRegisterType<Connection, 1>
    ("WPN114.Audio", 1, 1, "Connection");

    //=============================================================================================
    // MODULES
    //=============================================================================================

    qmlRegisterType<Graph, 1>     ("WPN114.Audio", 1, 1, "Audiograph");
    qmlRegisterType<JackIO, 1>    ("WPN114.Audio", 1, 1, "JackIO");
    qmlRegisterType<Sinetest, 1>  ("WPN114.Audio", 1, 1, "Sinetest");
    qmlRegisterType<VCA, 1>       ("WPN114.Audio", 1, 1, "VCA");
}
