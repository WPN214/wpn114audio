#include "graph.hpp"
#include <QtDebug>
#include <vector>
#include <cmath>

using namespace wpn114;

// --------------------------------------------------------------------------------------------
inline sample_t**
allocate_buffer(nchannels_t nchannels, vector_t nframes)
// we make no assumption as to how many channels each input/output may have
// by the time the graph is ready/updated, so we can't really template it upfront
// we also want to allocate the whole thing in a single block
// --------------------------------------------------------------------------------------------
{
    sample_t** block = static_cast<sample_t**>(
                  malloc(
                  sizeof(sample_t*)*nchannels +
                  sizeof(sample_t )*nframes*nchannels));

    for (nchannels_t n = 0; n < nchannels; ++n)
         block[n] = block[nchannels+n*nframes];

    return block;
}

// --------------------------------------------------------------------------------------------
inline void
reset_buffer(sample_t** buffer, nchannels_t nchannels, vector_t nframes)
// sets whole buffer to zero
// --------------------------------------------------------------------------------------------
{
    for (nchannels_t c = 0; c < nchannels; ++c)
         memset(&buffer[c], 0, sizeof(sample_t)*nframes);
}

// --------------------------------------------------------------------------------------------
Socket::Socket(Node* parent, Type type, polarity_t polarity, uint8_t index, uint8_t nchannels) :
// C++ constructor, called from the macro-declarations
// we immediately store a pointer in parent Node's input/output socket vector
// --------------------------------------------------------------------------------------------
    m_parent      (parent),
    m_polarity    (polarity),
    m_index       (index),
    m_type        (type),
    m_nchannels   (nchannels)
{
    parent->register_socket(*this);
}

// --------------------------------------------------------------------------------------------
void
Socket::assign(QVariant variant)
// --------------------------------------------------------------------------------------------
{
    if (variant.canConvert<Socket>())
    {
        // implicit connection
        auto socket = variant.value<Socket>();

        switch(m_polarity) {
        case INPUT: Graph::connect(*this, socket); break;
        case OUTPUT: Graph::connect(socket, *this);
        }
    }

    else if (variant.canConvert<Connection>())
    {
        // assigning an explicit connection
        // hmm that can be problematic...
        auto connection = variant.value<Connection>();
    }

    else if (variant.canConvert<qreal>())
    {
        // fill buffer with value
        qreal v = variant.value<qreal>();

    }

    else if (variant.canConvert<QVariantList>())
        for (auto& v : variant.value<QVariantList>())
             assign(v);

}

// --------------------------------------------------------------------------------------------
inline void
Socket::set_mul(qreal mul)
// this overrides the mul for all connections based on this socket
// --------------------------------------------------------------------------------------------
{
    for (auto& connection : m_connections)
         connection->set_mul(mul);
}

// --------------------------------------------------------------------------------------------
inline void
Socket::set_add(qreal add)
// this overrides the add for all connections based on this socket
// --------------------------------------------------------------------------------------------
{
    for (auto& connection : m_connections)
         connection->set_add(add);
}

// --------------------------------------------------------------------------------------------
inline Routing
Socket::routing() const { return m_connections[0]->routing(); }

// --------------------------------------------------------------------------------------------
inline void
Socket::set_routing(Routing r) { m_connections[0]->set_routing(r); }

// --------------------------------------------------------------------------------------------
inline void
Socket::set_muted(bool muted)
// mutes all connections, zero output
// --------------------------------------------------------------------------------------------
{
    for (const auto& connection : m_connections)
         connection->m_muted = muted;
}

// --------------------------------------------------------------------------------------------
bool
Socket::connected(Socket const& s) const
// TODO for feedback connections
// --------------------------------------------------------------------------------------------
{
    for (const auto& connection : m_connections)
        ;
}

// --------------------------------------------------------------------------------------------
bool
Socket::connected(Node const& n) const
// TODO for feedback connections
// --------------------------------------------------------------------------------------------
{
    for (const auto& connection: m_connections)
        ;
}

// --------------------------------------------------------------------------------------------
inline void
Graph::register_node(Node& node) noexcept
// --------------------------------------------------------------------------------------------
{
    QObject::connect(s_instance, &Graph::rateChanged, &node, &Node::on_rate_changed);
    s_nodes.push_back(&node);
}

// --------------------------------------------------------------------------------------------
inline Connection&
Graph::connect(Socket& source, Socket& dest, Routing matrix)
// --------------------------------------------------------------------------------------------
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
// --------------------------------------------------------------------------------------------
{
    return connect(*source.default_outputs(), *dest.default_inputs(), matrix);
}

// --------------------------------------------------------------------------------------------
inline Connection&
Graph::connect(Node& source, Socket& dest, Routing matrix)
// --------------------------------------------------------------------------------------------
{
    return connect(*source.default_outputs(), dest, matrix);
}

// --------------------------------------------------------------------------------------------
inline Connection&
Graph::connect(Socket& source, Node& dest, Routing matrix)
// --------------------------------------------------------------------------------------------
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
Graph::run(Node& target) noexcept
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

    switch(socket->polarity()) {
    case OUTPUT: m_source = socket; break;
    case INPUT: m_dest = socket;
    }
}

// --------------------------------------------------------------------------------------------
void
Connection::componentComplete() { Graph::add_connection(*this); }

// --------------------------------------------------------------------------------------------
void
Connection::pull(vector_t nframes) noexcept
// --------------------------------------------------------------------------------------------
{
    if (!m_feedback && !m_source->parent_node().processed())
        m_source->parent_node().process(nframes);

    auto sbuf = m_source->buffer();
    auto dbuf = m_dest->buffer();

    if (m_routing.null())
        for (nchannels_t c = 0; c < m_nchannels; ++c)
            for (vector_t f = 0; f < nframes; ++f)
                 dbuf[c][f] += sbuf[c][f] * m_mul + m_add;
    else
    {
        for (nchannels_t r = 0; r < m_routing.ncables(); ++r)
            for (vector_t f = 0; f < nframes; ++f)
                dbuf[m_routing[r][0]][f] += m_mul*
                sbuf[m_routing[r][1]][f] + m_add;
    }
}
