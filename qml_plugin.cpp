#include "qml_plugin.hpp"
#include <wpn114audio/graph.hpp>
#include <wpn114audio/spatial.hpp>
#include <source/io/external.hpp>
#include <source/basics/audio/sinetest.hpp>
#include <source/basics/audio/vca.hpp>
#include <source/basics/audio/clock.hpp>
#include <source/basics/audio/peakrms.hpp>
#include <source/basics/midi/velocity-table.hpp>
#include <source/basics/midi/transposer.hpp>
#include <source/basics/midi/rwriter.hpp>
#include <QQmlEngine>
#include <qqml.h>

void qml_plugin::registerTypes(const char* uri)
{
    Q_UNUSED(uri)

    qmlRegisterUncreatableType<Dispatch, 1>
    ("WPN114.Audio", 1, 1, "Dispatch", "Uncreatable");

    qmlRegisterUncreatableType<Node, 1>
    ("WPN114.Audio", 1, 1, "Node", "Uncreatable");

    qmlRegisterUncreatableType<Spatial, 1>
    ("WPN114.Audio", 1, 1, "Spatial", "Uncreatable");

    qmlRegisterUncreatableType<SpatialProcessor, 1>
    ("WPN114.Audio", 1, 1, "SpatialProcessor", "Uncreatable");

    qmlRegisterUncreatableType<Port, 1>
    ("WPN114.Audio", 1, 1, "Port", "Uncreatable");

    qmlRegisterUncreatableType<Routing, 1>
    ("WPN114.Audio", 1, 1, "Routing", "Uncreatable");

    //=============================================================================================
    // MODULES
    //=============================================================================================

    qmlRegisterType<Graph, 1>
    ("WPN114.Audio", 1, 1, "Graph");

    qmlRegisterType<Connection, 1>
    ("WPN114.Audio", 1, 1, "Connection");

    qmlRegisterType<External, 1>
    ("WPN114.Audio", 1, 1, "External");

    qmlRegisterType<Input, 1>
    ("WPN114.Audio", 1, 1, "Input");

    qmlRegisterType<Output, 1>
    ("WPN114.Audio", 1, 1, "Output");

    qmlRegisterType<Sinetest, 1>
    ("WPN114.Audio", 1, 1, "Sinetest");

    qmlRegisterType<VCA, 1>
    ("WPN114.Audio", 1, 1, "VCA");

    qmlRegisterType<PeakRMS, 1>
    ("WPN114.Audio", 1, 1, "PeakRMS");

    qmlRegisterType<StereoPanner, 1>
    ("WPN114.Audio", 1, 1, "StereoPanner");

    qmlRegisterType<Gateway, 1>
    ("WPN114.Audio", 1, 1, "MIDIGateway");

    qmlRegisterType<Transposer, 1>
    ("WPN114.Audio", 1, 1, "MIDITransposer");

    qmlRegisterType<VelocityMap, 1>
    ("WPN114.Audio", 1, 1, "VelocityMap");

    qmlRegisterType<Clock, 1>
    ("WPN114.Audio", 1, 1, "SimpleClock");

}
