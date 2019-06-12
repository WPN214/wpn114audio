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

std::vector<Node*>
Graph::s_nodes;

using namespace wpn114;

// ------------------------------------------------------------------------------------------------
sample_t**
wpn114::allocate_buffer(nchannels_t nchannels, vector_t nframes)
// we make no assumption as to how many channels each input/output may have
// by the time the graph is ready/updated, so we can't really template it upfront
// ------------------------------------------------------------------------------------------------
{
    sample_t** block = new sample_t*[nchannels];
    for (nchannels_t n = 0; n < nchannels; ++n)
        block[n] = new sample_t[nframes];

    return block;
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
    m_nchannels   (nchannels),
    m_value       (0)
{
    parent->register_socket(*this);
}

// ------------------------------------------------------------------------------------------------
void
Socket::assign(QVariant variant)
// ------------------------------------------------------------------------------------------------
{
    if (variant.canConvert<qreal>())
        set_value(variant.value<qreal>());

    else if (variant.canConvert<Socket*>())
    {
        Socket* s = variant.value<Socket*>();

        switch(s->polarity()) {
        case INPUT: Graph::connect(*this, *s); break;
        case OUTPUT: Graph::connect(*s, *this);
        }
    }

    else if (variant.canConvert<Connection>()) {
        auto connection = variant.value<Connection>();
        Graph::add_connection(connection);
    }

    else if (variant.canConvert<QVariantList>())
        for (auto& v : variant.value<QVariantList>())
             assign(v);
}

// ------------------------------------------------------------------------------------------------
WPN_TODO void
Socket::set_nchannels(nchannels_t nchannels)
// todo: async call when graph is running
// ------------------------------------------------------------------------------------------------
{
    m_nchannels = nchannels;
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
Socket::routing() const noexcept
{
    return m_routing;
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Socket::set_routing(QVariantList r) noexcept
{
    m_routing = r;
}

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
    qDebug() << "[GRAPH] registering node:" << node.name();

    QObject::connect(s_instance, &Graph::rateChanged, &node, &Node::on_rate_changed);
    s_nodes.push_back(&node);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE Connection&
Graph::connect(Socket& source, Socket& dest, Routing matrix)
// there's still the Audio/Midi Type to check,
// the multichannel expansion allocation
// & debug the connection with a pretty message
// ------------------------------------------------------------------------------------------------
{
    assert(source.polarity() == OUTPUT && dest.polarity() == INPUT);
    assert(source.type() == dest.type());

    auto con = Connection(source, dest, matrix);

    if (dest.parent_node().connected(source.parent_node()))
        con.set_feedback(true);

    s_connections.push_back(con);   
    Connection& connection = s_connections.back();

    source.add_connection(connection);
    dest.add_connection(connection);

    qDebug() << "[GRAPH] connected Node" << source.parent_node().name()
             << "(socket:" << source.name().append(")")
             << "with Node" << dest.parent_node().name()
             << "(socket:" << dest.name().append(")");

    for (nchannels_t n = 0; n < matrix.ncables(); ++n) {
        qDebug() << "[GRAPH] ---- routing: channel" << QString::number(matrix[n][0])
                 << ">> channel" << QString::number(matrix[n][1]);
    }

    return connection;
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE Connection&
Graph::connect(Node& source, Node& dest, Routing matrix)
// might be midi and not audio...
// ------------------------------------------------------------------------------------------------
{
    return connect(*source.default_socket(OUTPUT),
                   *dest.default_socket(INPUT), matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE Connection&
Graph::connect(Node& source, Socket& dest, Routing matrix)
// ------------------------------------------------------------------------------------------------
{
    return connect(*source.default_socket(dest.type(), OUTPUT), dest, matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE Connection&
Graph::connect(Socket& source, Node& dest, Routing matrix)
// ------------------------------------------------------------------------------------------------
{
    return connect(source, *dest.default_socket(INPUT), matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Graph::componentComplete()
// Graph is complete, all connections have been set-up
// we send signal to all registered Nodes to proceed to socket/pool allocation
// ------------------------------------------------------------------------------------------------
{
    Graph::debug("component complete, allocating nodes i/o");

    for (auto& node : s_nodes)
        node->on_graph_complete(s_properties);

    Graph::debug("i/o allocation complete");
    emit complete();
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE WPN_AUDIOTHREAD vector_t
Graph::run(Node& target) noexcept
// the main processing function
// Graph will process itself from target Node and upstream recursively
// ------------------------------------------------------------------------------------------------
{
    // take a little amount of time to process asynchronous graph update requests (TODO)
    WPN_TODO

    // process target, return outputs            
    vector_t nframes = s_properties.vector;
    target.process(nframes);

    for (auto& node : s_nodes)
         node->set_processed(false);

    return nframes;
}

// ------------------------------------------------------------------------------------------------
// CONNECTION
//-------------------------------------------------------------------------------------------------
WPN_INCOMPLETE
Connection::Connection(Socket& source, Socket& dest, Routing matrix) :
    m_source   (&source)
  , m_dest     (&dest)
  , m_routing  (matrix)
{
    m_nchannels = std::min(source.nchannels(), dest.nchannels());
}


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
WPN_AUDIOTHREAD void
Connection::pull(vector_t nframes) noexcept
// ------------------------------------------------------------------------------------------------
{    
    auto& source = m_source->parent_node();

    if (!source.processed())
        source.process(nframes);

    sample_t** sbuf = m_source->buffer();
    sample_t** dbuf = m_dest->buffer();

    sample_t  mul  = m_mul, add  = m_add;
    Routing routing = m_routing;

    if (m_muted.load()) {
        for (nchannels_t c = 0; c < m_dest->nchannels(); ++c)
             memset(dbuf, 0, sizeof(sample_t)*nframes);
        return;
    }

    if (routing.null())
    {
        for (nchannels_t c = 0; c < m_nchannels; ++c) {
            for (vector_t f = 0; f < nframes; ++f) {
                if (m_source->type() != Socket::Type::Midi_1_0)
                     dbuf[c][f] += sbuf[c][f] * mul + add;
                else dbuf[c][f] = sbuf[c][f];
            }
        }
    }
    else // routing non null
    {
        for (nchannels_t c = 0; c < routing.ncables(); ++c) {
            for (vector_t f = 0; f < nframes; ++f) {
                if (m_source->type() != Socket::Type::Midi_1_0)
                      dbuf[routing[c][1]][f] += sbuf[routing[c][0]][f] * mul + add;
                 else dbuf[routing[c][1]][f] = sbuf[routing[c][0]][f];
            }
        }
    }
}
