#include "qtwrapper.hpp"
#include <QtDebug>
#include <vector>
#include <cmath>

// --------------------------------------------------------------------------------------------
Socket::Socket(Node* parent, polarity_t polarity, uint8_t index, uint8_t nchannels) :
// C++ constructor, called from the macro-declarations
// we immediately store a pointer in parent Node's input/output socket vector
// --------------------------------------------------------------------------------------------
    m_parent      (parent),
    m_polarity    (polarity),
    m_index       (index),
    m_nchannels   (nchannels)
{
    parent->register_socket(*this);
}

// --------------------------------------------------------------------------------------------
void
Socket::set_muted(bool muted)
// mutes all connections, zero output
// --------------------------------------------------------------------------------------------
{
    for (const auto& con : m_connections)
         con->m_muted = muted;
}

// --------------------------------------------------------------------------------------------
void
Socket::set_level(qreal level)
// --------------------------------------------------------------------------------------------
{
    for (const auto& con : m_connections)
         con->m_level = level;
}

// --------------------------------------------------------------------------------------------
bool
Socket::connected(const Socket& s) const
// TODO for feedback connections
// --------------------------------------------------------------------------------------------
{
    for (const auto& con : m_connections)
        ;
}

// --------------------------------------------------------------------------------------------
bool
Socket::connected(const Node& n) const
// TODO for feedback connections
// --------------------------------------------------------------------------------------------
{
    for (const auto& con: m_connections)
        ;
}

// --------------------------------------------------------------------------------------------
inline void
Graph::register_node(Node& node)
{
    QObject::connect(s_instance, &Graph::rateChanged, &node, &Node::on_rate_changed);
    s_nodes.push_back(&node);
}

// --------------------------------------------------------------------------------------------
inline Connection&
Graph::connect(Socket& source, Socket& dest, Routing matrix)
{
    auto con = Connection(source, dest, matrix);

    if (dest.parent_node().connected(source.parent_node()))
        con.set_feedback(true);

    s_connections.push_back(con);
    return s_connections.back();
}

// --------------------------------------------------------------------------------------------
inline Connection&
Graph::connect(Node& source, Node& dest, Routing matrix)
{
    return connect(*source.default_outputs(), *dest.default_inputs(), matrix);
}

// --------------------------------------------------------------------------------------------
inline Connection&
Graph::connect(Node& source, Socket& dest, Routing matrix)
{
    return connect(*source.default_outputs(), dest, matrix);
}

// --------------------------------------------------------------------------------------------
inline Connection&
Graph::connect(Socket& source, Node& dest, Routing matrix)
{
    return connect(source, *dest.default_inputs(), matrix);
}

// --------------------------------------------------------------------------------------------
void
Graph::componentComplete()
// not really sure what to do here just yet
// --------------------------------------------------------------------------------------------
{

}

// --------------------------------------------------------------------------------------------
inline pool&
Graph::run(Node& target)
// the main processing function
// Graph will process itself from target Node and upstream recursively
// --------------------------------------------------------------------------------------------
{
    vector_t nframes = s_properties.vector;
    target.process(nframes);

    // clean up inputs buffers and
    // mark Nodes as unprocessed
    for (auto& node : s_nodes)
        node->reset_process(nframes);

    return target.outputs();
}

//-------------------------------------------------------------------------------------------------
// CONNECTION
//-------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------
void
Connection::setTarget(const QQmlProperty& target)
// qml property binding, e. g.:
// Connection on 'socket' { level: db(-12); routing: [0, 1] }
// --------------------------------------------------------------------------------------------
{
    auto socket = target.read().value<Socket*>();

    switch(socket->polarity())
    {
    case OUTPUT:  m_source = socket; break;
    case INPUT:   m_dest = socket;
    }
}

// --------------------------------------------------------------------------------------------
void
Connection::componentComplete() { Graph::add_connection(*this); }

// --------------------------------------------------------------------------------------------
void
Connection::pull(vector_t nframes)
// --------------------------------------------------------------------------------------------
{
    if (!m_feedback && !m_source->parent_node().processed())
        m_source->parent_node().process(nframes);

    auto sbuf = m_source->buffer();
    auto dbuf = m_dest->buffer();

    if (m_routing.null())
        for (nchannels_t c = 0; c < m_nchannels; ++c)
            for (vector_t f = 0; f < nframes; ++f)
                 dbuf[c][f] += sbuf[c][f] * m_level;
    else
    {
        for (nchannels_t r = 0; r < m_routing.ncables(); ++r)
            for (vector_t f = 0; f < nframes; ++f)
                dbuf[m_routing[r][0]][f] += m_level*
                sbuf[m_routing[r][1]][f];
    }
}
