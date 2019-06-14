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
Port::Port(Node* parent, QString name, Type type, Polarity polarity,
           bool is_default, uint8_t nchannels) :
// C++ constructor, called from the macro-declarations
// we immediately store a pointer in parent Node's input/output Port vector
// ------------------------------------------------------------------------------------------------
    m_parent      (parent),
    m_name        (name),
    m_polarity    (polarity),
    m_type        (type),
    m_nchannels   (nchannels),
    m_value       (0)
{
    parent->register_port(*this);
}

// ------------------------------------------------------------------------------------------------
void
Port::assign(Port* p)
// ------------------------------------------------------------------------------------------------
{
    switch(p->polarity()) {
        case Polarity::Input: Graph::connect(*this, *p); break;
        case Polarity::Output: Graph::connect(*p, *this);
    }
}

// ------------------------------------------------------------------------------------------------
WPN_TODO void
Port::set_nchannels(nchannels_t nchannels)
// todo: async call when graph is running
// ------------------------------------------------------------------------------------------------
{
    m_nchannels = nchannels;
}

// ------------------------------------------------------------------------------------------------
void
Port::set_mul(qreal mul)
// this overrides the mul for all connections based on this Port
// ------------------------------------------------------------------------------------------------
{
    m_mul = mul;
    for (auto& connection : m_connections)
         connection->set_mul(mul);
}

// ------------------------------------------------------------------------------------------------
void
Port::set_add(qreal add)
// this overrides the add for all connections based on this Port
// ------------------------------------------------------------------------------------------------
{
    m_add = add;
    for (auto& connection : m_connections)
         connection->set_add(add);
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE QVariantList
Port::routing() const noexcept
{
    return m_routing;
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Port::set_routing(QVariantList r) noexcept
{
    m_routing = r;
}

// ------------------------------------------------------------------------------------------------
void
Port::set_muted(bool muted)
// mutes all connections, zero output
// ------------------------------------------------------------------------------------------------
{
    for (const auto& connection : m_connections)
         connection->m_muted = muted;
}

// ------------------------------------------------------------------------------------------------
WPN_CLEANUP bool
Port::connected(Port const& s) const noexcept
// ------------------------------------------------------------------------------------------------
{
    for (const auto& connection : m_connections) {
         if (m_polarity == Polarity::Input &&
             connection->source() == &s)
             return true;
         else if (m_polarity == Polarity::Output &&
              connection->dest() == &s)
             return true;
    }

    return false;
}

// ------------------------------------------------------------------------------------------------
WPN_CLEANUP bool
Port::connected(Node const& n) const noexcept
// ------------------------------------------------------------------------------------------------
{
    for (const auto& connection: m_connections) {
        if (m_polarity == Polarity::Input &&
            &connection->source()->parent_node() == &n)
            return true;
        else if (m_polarity == Polarity::Output &&
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
Graph::connect(Port& source, Port& dest, Routing matrix)
// there's still the Audio/Midi Type to check,
// the multichannel expansion allocation
// & debug the connection with a pretty message
// ------------------------------------------------------------------------------------------------
{
    assert(source.polarity() == Polarity::Output && dest.polarity() == Polarity::Input);
    assert(source.type() == dest.type());

    auto con = Connection(source, dest, matrix);

    s_connections.push_back(con);   
    Connection& connection = s_connections.back();

    source.add_connection(connection);
    dest.add_connection(connection);

    qDebug() << "[GRAPH] connected Node" << source.parent_node().name()
             << "(Port:" << source.name().append(")")
             << "with Node" << dest.parent_node().name()
             << "(Port:" << dest.name().append(")");

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
    return connect(*source.default_port(Polarity::Output),
                   *dest.default_port(Polarity::Input), matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE Connection&
Graph::connect(Node& source, Port& dest, Routing matrix)
// ------------------------------------------------------------------------------------------------
{
    return connect(*source.default_port(dest.type(), Polarity::Output), dest, matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE Connection&
Graph::connect(Port& source, Node& dest, Routing matrix)
// ------------------------------------------------------------------------------------------------
{
    return connect(source, *dest.default_port(Polarity::Input), matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Graph::componentComplete()
// Graph is complete, all connections have been set-up
// we send signal to all registered Nodes to proceed to Port/pool allocation
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
Connection::Connection(Port& source, Port& dest, Routing matrix) :
    m_source   (&source)
  , m_dest     (&dest)
  , m_routing  (matrix)
{
    componentComplete();
}

// ------------------------------------------------------------------------------------------------
void
Connection::setTarget(const QQmlProperty& target)
// qml property binding, e. g.:
// Connection on 'Port' { level: db(-12); routing: [0, 1] }
// not sure target will give us a Port pointer though
// maybe a Port copy...
// ------------------------------------------------------------------------------------------------
{
    auto port = target.read().value<Port*>();

    switch(port->polarity()) {
        case Polarity::Output: m_source = port; break;
        case Polarity::Input: m_dest = port;
    }

    componentComplete();
}

// ------------------------------------------------------------------------------------------------
void
Connection::componentComplete()
{
    m_nchannels = std::min(m_source->nchannels(), m_dest->nchannels());
    m_mul = m_source->mul() * m_dest->mul();
    m_add = m_source->add() + m_dest->add();
    m_muted = m_source->muted() || m_dest->muted();

    Graph::add_connection(*this);
}

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
                if (m_source->type() != Port::Type::Midi_1_0)
                     dbuf[c][f] += sbuf[c][f] * mul + add;
                else dbuf[c][f] = sbuf[c][f];
            }
        }
    }
    else // routing non null
    {
        for (nchannels_t c = 0; c < routing.ncables(); ++c) {
            for (vector_t f = 0; f < nframes; ++f) {
                if (m_source->type() != Port::Type::Midi_1_0)
                      dbuf[routing[c][1]][f] += sbuf[routing[c][0]][f] * mul + add;
                 else dbuf[routing[c][1]][f] = sbuf[routing[c][0]][f];
            }
        }
    }
}
