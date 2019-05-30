#include "qtwrapper.hpp"
#include <QtDebug>
#include <vector>
#include <cmath>

Socket::Socket(Node* parent, polarity_t polarity, uint8_t index, uint8_t nchannels) :
    m_parent      (parent),
    m_polarity    (polarity),
    m_index       (index),
    m_nchannels   (nchannels)
{
    parent->register_socket(*this);
}

void
Socket::set_muted(bool muted)
{
    for (const auto& con : m_connections)
         con->m_muted = muted;
}

void
Socket::set_active(bool active)
{
    for (const auto& con : m_connections)
         con->m_active = active;
}

void
Socket::set_level(qreal level)
{
    for (const auto& con : m_connections)
         con->m_level = level;
}

bool
Socket::connected(const Socket& s) const
{
    for (const auto& con : m_connections)
        ;
}

bool
Socket::connected(const Node& n) const
{
    for (const auto& con: m_connections)
        ;
}

//-------------------------------------------------------------------------------------------------
// GRAPH
//-------------------------------------------------------------------------------------------------

inline Connection&
Graph::connect(Node& source, Node& dest, Routing matrix)
{
    return connect(*source.default_outputs(), *dest.default_inputs(), matrix);
}

inline Connection&
Graph::connect(Node& source, Socket& dest, Routing matrix)
{
    return connect(*source.default_outputs(), dest, matrix);
}

inline Connection&
Graph::connect(Socket& source, Node& dest, Routing matrix)
{
    return connect(source, *dest.default_inputs(), matrix);
}

void
Graph::componentComplete()
{

}

inline void
Graph::initialize()
{

}

inline pool&
Graph::run(Node& target)
{
    target.process();
    return target.outputs();
}

//-------------------------------------------------------------------------------------------------
// CONNECTION
//-------------------------------------------------------------------------------------------------

void
Connection::setTarget(const QQmlProperty& target)
{
    auto socket = target.read().value<Socket*>();

    switch(socket->polarity())
{
    case OUTPUT:  m_source = socket; break;
    case INPUT:   m_dest = socket;
    }
}

void
Connection::componentComplete()
{
    // allocates/reallocates Connection buffer
    // allocates/reallocates source/dest buffers as well
    // depending on both sockets numchannels properties
    // this means Connection has been explicitely instantiated from QML
    // we have to push it back to the graph
    Graph::add_connection(this);

    // TODO: manage multichannel expansion

    m_source->allocate();
    m_dest->allocate();

    nchannels_t nchannels = std::max(m_source->nchannels(), m_dest->nchannels());
    m_buffer = new qreal*[nchannels];
}

void
Connection::pull()
{
    if (!m_active)
        return;

    if (!m_feedback) {
        m_source->parent_node()->process();
    }

    // get source buffer
    // get dest buffer
    // copy source into this buffer
    // apply gain
    // pour into dest buffer, given Connection's routing

}

//-------------------------------------------------------------------------------------------------
// NODE
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// SINETEST
//-------------------------------------------------------------------------------------------------

Sinetest::Sinetest()
{

}

void
Sinetest::initialize(Graph::properties& properties)
{

}

void
Sinetest::rwrite(pool& inputs, pool& outputs, vector_t nframes)
{
    auto frequency = inputs[Frequency];
    auto out = outputs[Outputs];



}

//-------------------------------------------------------------------------------------------------
// VCA
//-------------------------------------------------------------------------------------------------

VCA::VCA()
{

}

void
VCA::rwrite(pool& inputs, pool& outputs, vector_t nframes)
{
    auto in   = inputs[Inputs];
    auto out  = outputs[Outputs];
    auto gain = inputs[Gain];

    FOR_NCHANNELS(c, Outputs)
        FOR_NFRAMES(n, nframes)
            out[c][n] = in[c][n] * gain[0][n];
        END
    END
}

//-------------------------------------------------------------------------------------------------
// IOJACK
//-------------------------------------------------------------------------------------------------

IOJack::IOJack()
{

}
