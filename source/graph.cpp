#include "graph.hpp"
#include <QtDebug>
#include <vector>
#include <cmath>

using namespace wpn114;

Graph*
Graph::s_instance;

std::vector<Connection>
Graph::s_connections;

Graph::properties
Graph::s_properties;

QVector<Node*>
Graph::m_subnodes;

//-------------------------------------------------------------------------------------------------
std::vector<Node*>
Graph::s_nodes;

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE inline sample_t**
allocate_buffer(nchannels_t nchannels, vector_t nframes)
// we make no assumption as to how many channels each input/output may have
// by the time the graph is ready/updated, so we can't really template it upfront
// we also want to allocate the whole thing in a single block
// ------------------------------------------------------------------------------------------------
{
    sample_t** block = static_cast<sample_t**>(
                  malloc(
                  sizeof(sample_t*)*nchannels +
                  sizeof(sample_t )*nframes*nchannels));

    for (nchannels_t n = 0; n < nchannels; ++n)
         block[n] = block[nchannels+n*nframes];

    return block;
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE inline void
reset_buffer(sample_t** buffer, nchannels_t nchannels, vector_t nframes)
// sets whole buffer to zero
// ------------------------------------------------------------------------------------------------
{
    for (nchannels_t c = 0; c < nchannels; ++c)
         memset(&buffer[c], 0, sizeof(sample_t)*nframes);
}

// ------------------------------------------------------------------------------------------------
Socket::Socket(Node* parent, QString name, Type type, polarity_t polarity,
uint8_t index, uint8_t nchannels) :
// C++ constructor, called from the macro-declarations
// we immediately store a pointer in parent Node's input/output socket vector
// ------------------------------------------------------------------------------------------------
    m_parent      (parent),
    m_name        (name),
    m_polarity    (polarity),
    m_index       (index),
    m_type        (type),
    m_nchannels   (nchannels)
{
    parent->register_socket(*this);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE void
Socket::assign(Socket* socket)
// ------------------------------------------------------------------------------------------------
{
    switch(m_polarity) {
    case INPUT: Graph::connect(*this, *socket); break;
    case OUTPUT: Graph::connect(*socket, *this);
    }
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Socket::set_nchannels(nchannels_t nchannels)
// ------------------------------------------------------------------------------------------------
{
    wpn114::allocate_buffer(nchannels, Graph::vector());
}

// ------------------------------------------------------------------------------------------------
WPN_OK void
Socket::set_mul(qreal mul)
// this overrides the mul for all connections based on this socket
// ------------------------------------------------------------------------------------------------
{
    for (auto& connection : m_connections)
         connection->set_mul(mul);
}

// ------------------------------------------------------------------------------------------------
WPN_OK void
Socket::set_add(qreal add)
// this overrides the add for all connections based on this socket
// ------------------------------------------------------------------------------------------------
{
    for (auto& connection : m_connections)
         connection->set_add(add);
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE QVariantList
Socket::routing() const noexcept { return m_connections[0]->routing(); }
// examine in concrete QML situations

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Socket::set_routing(QVariantList r) noexcept { m_connections[0]->set_routing(r); }
// examine in concrete QML situations

// ------------------------------------------------------------------------------------------------
WPN_OK void
Socket::set_muted(bool muted)
// mutes all connections, zero output
// ------------------------------------------------------------------------------------------------
{
    for (const auto& connection : m_connections)
         connection->m_muted = muted;
}

// ------------------------------------------------------------------------------------------------
WPN_CLEANUP bool
Socket::connected(Socket const& s) const noexcept
// ------------------------------------------------------------------------------------------------
{
    for (const auto& connection : m_connections)
    {
         if (m_polarity == INPUT &&
             connection->source() == &s)
             return true;
         else if (m_polarity == OUTPUT &&
              connection->dest() == &s)
             return true;
    }

    return false;
}

// ------------------------------------------------------------------------------------------------
WPN_CLEANUP bool
Socket::connected(Node const& n) const noexcept
// ------------------------------------------------------------------------------------------------
{
    for (const auto& connection: m_connections)
    {
        if (m_polarity == INPUT &&
            &connection->source()->parent_node() == &n)
            return true;
        else if (m_polarity == OUTPUT &&
            &connection->dest()->parent_node() == &n)
            return true;
    }

    return false;
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Graph::register_node(Node& node) noexcept
// we should maybe add a vectorChanged signal-slot connection to this
// we also might not need to store pointers...
// ------------------------------------------------------------------------------------------------
{
    QObject::connect(s_instance, &Graph::rateChanged, &node, &Node::on_rate_changed);
    s_nodes.push_back(&node);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE inline Connection&
Graph::connect(Socket& source, Socket& dest, Routing matrix)
// there's still the Audio/Midi Type to check,
// the multichannel expansion allocation
// & debug the connection with a pretty message
// ------------------------------------------------------------------------------------------------
{
    assert(source.polarity() == OUTPUT &&
           dest.polarity() == INPUT);

    auto con = Connection(source, dest, matrix);

    if (dest.parent_node().connected(source.parent_node()))
        con.set_feedback(true);

    s_connections.push_back(con);   

    qDebug() << "Graph: connected Node" << source.parent_node().name()
             << "(socket:" << source.name() << ")"
             << "with Node" << dest.parent_node().name()
             << "(socket:" << dest.name() << ")";

    for (nchannels_t n = 0; n < matrix.ncables(); ++n) {
        qDebug() << "routing: channel" << QString::number(matrix[n][0])
                 << ">> channel" << QString::number(matrix[n][1]);
    }

    return s_connections.back();
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE inline Connection&
Graph::connect(Node& source, Node& dest, Routing matrix)
// might be midi and not audio...
// ------------------------------------------------------------------------------------------------
{
    return connect(*source.default_audio_outputs(),
                   *dest.default_audio_inputs(), matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE inline Connection&
Graph::connect(Node& source, Socket& dest, Routing matrix)
// might be midi bis
// ------------------------------------------------------------------------------------------------
{
    return connect(*source.default_audio_outputs(), dest, matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE inline Connection&
Graph::connect(Socket& source, Node& dest, Routing matrix)
// midi...
// ------------------------------------------------------------------------------------------------
{
    return connect(source, *dest.default_audio_inputs(), matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Graph::componentComplete()
// not really sure what to do here just yet
// ------------------------------------------------------------------------------------------------
{

}

// ------------------------------------------------------------------------------------------------
WPN_INCORRECT inline pool&
Graph::run(Node& target) noexcept
// the main processing function
// Graph will process itself from target Node and upstream recursively
// ------------------------------------------------------------------------------------------------
{
    vector_t nframes = s_properties.vector;
    target.process(nframes);

    // clean up inputs buffers and
    // mark Nodes as unprocessed

    WPN_INCORRECT
    // WARNING, this is not good for the 'External' node
    // as it will hold the actual output in its input pool
    for (auto& node : s_nodes)
        node->reset_process(nframes);

    return target.outputs();
}

// ------------------------------------------------------------------------------------------------
// CONNECTION
//-------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Connection::setTarget(const QQmlProperty& target)
// qml property binding, e. g.:
// Connection on 'socket' { level: db(-12); routing: [0, 1] }
// not sure target will give us a Socket pointer though
// maybe a Socket copy...
// ------------------------------------------------------------------------------------------------
{
    auto socket = target.read().value<Socket*>();

    switch(socket->polarity()) {
    case OUTPUT: m_source = socket; break;
    case INPUT: m_dest = socket;
    }
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Connection::componentComplete() { Graph::add_connection(*this); }
// examine: when Connection is explicitely assigned to a Socket

// ------------------------------------------------------------------------------------------------
WPN_CLEANUP void
Connection::pull(vector_t nframes) noexcept
// ------------------------------------------------------------------------------------------------
{
    if (!m_feedback && !m_source->parent_node().processed())
        m_source->parent_node().process(nframes);

    auto sbuf = m_source->buffer();
    auto dbuf = m_dest->buffer();

    if (m_routing.null())
        for (nchannels_t c = 0; c < m_nchannels; ++c)
            for (vector_t f = 0; f < nframes; ++f) {
                if (m_source->type() != Socket::Type::Midi_1_0)
                     dbuf[c][f] += sbuf[c][f] * m_mul + m_add;
                else dbuf[c][f] += sbuf[c][f];
            }
    else
    {
        for (nchannels_t r = 0; r < m_routing.ncables(); ++r)
            for (vector_t f = 0; f < nframes; ++f) {
                if (m_source->type() != Socket::Type::Midi_1_0)
                dbuf[m_routing[r][0]][f] +=
                        m_mul* sbuf[m_routing[r][1]][f] + m_add;
                else
                dbuf[m_routing[r][0]][f] +=
                        sbuf[m_routing[r][1]][f];
            }
    }
}
