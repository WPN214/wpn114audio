#include "qml_plugin.hpp"
#include <source/graph.hpp>
#include <source/io/external.hpp>
#include <source/basics/sinetest.hpp>
#include <source/basics/vca.hpp>
#include <source/basics/midi-transpose.hpp>
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

    qmlRegisterType<Routing, 1>
    ("WPN114.Audio", 1, 1, "Routing");

    //=============================================================================================
    // MODULES
    //=============================================================================================

    qmlRegisterType<Graph, 1>
    ("WPN114.Audio", 1, 1, "Graph");

    qmlRegisterType<External, 1>
    ("WPN114.Audio", 1, 1, "External");

    qmlRegisterType<Sinetest, 1>
    ("WPN114.Audio", 1, 1, "Sinetest");

    qmlRegisterType<VCA, 1>
    ("WPN114.Audio", 1, 1, "VCA");

    qmlRegisterType<MidiTransposer, 1>
    ("WPN114.Audio", 1, 1, "MIDITransposer");

}
