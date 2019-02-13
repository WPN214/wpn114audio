#include "qml_plugin.hpp"
#include <source/qtwrapper.hpp>
#include <QQmlEngine>
#include <qqml.h>

void qml_plugin::registerTypes(const char *uri)
{
    Q_UNUSED    ( uri );

    qmlRegisterUncreatableType<Dispatch, 1>
            ("WPN114.Audio", 1, 1, "Dispatch", "Uncreatable");

    qmlRegisterUncreatableType<Node, 1>
            ("WPN114.Audio", 1, 1, "Node", "Uncreatable");


}
