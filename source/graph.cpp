#include <QtDebug>
#include <vector>
#include <cmath>
#include <wpn114audio/graph.hpp>

Graph*
Graph::s_instance;

namespace wpn114 {
// ------------------------------------------------------------------------------------------------
template<> audiobuffer_t
allocate_buffer(nchannels_t nchannels, vector_t nframes)
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
template<> midibuffer_t
allocate_buffer(nchannels_t nchannels, vector_t nframes)
// ------------------------------------------------------------------------------------------------
{
    midibuffer** block = new midibuffer*[nchannels];

    for (nchannels_t n = 0; n < nchannels; ++n)
         block[n] = new midibuffer(sizeof(sample_t)*nframes);

    return block;
}
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
    m_value       (0),
    m_default     (is_default)
{
    parent->register_port(*this);
}

// ------------------------------------------------------------------------------------------------
void
Port::allocate(vector_t nframes)
// ------------------------------------------------------------------------------------------------
{
    if   (m_type == Port::Midi_1_0)
         m_buffer.midi = wpn114::allocate_buffer<midibuffer_t>(m_nchannels, nframes);
    // we allocate the same buffer size (in bytes) for the midibuffer
    else m_buffer.audio = wpn114::allocate_buffer<audiobuffer_t>(m_nchannels, nframes);
}

// ------------------------------------------------------------------------------------------------
void
Port::assign(Port* p)
// this is called from QML when another port is explicitely assigned to this one
// we make an implicit connection between the two
// ------------------------------------------------------------------------------------------------
{
    switch(p->polarity()) {
        case Polarity::Input: Graph::instance().connect(*this, *p); break;
        case Polarity::Output: Graph::instance().connect(*p, *this);
    }
}

// ------------------------------------------------------------------------------------------------
void
Port::assign(QVariant v)
// ------------------------------------------------------------------------------------------------
{    
    if (v.canConvert<Node*>())
    {
        auto node = v.value<Node*>();

        switch(m_polarity) {
        case Polarity::Input:
            Graph::instance().connect(node->chainout(), *this);
            break;
        case Polarity::Output:
             Graph::instance().connect(*this, *node);
        }
    }

    else if (v.canConvert<Port*>())
        assign(v.value<Port*>());

    else if (v.canConvert<Connection>())
    {
        WPN_TODO
    }

    else if (v.canConvert<QVariantList>())
        for (auto& element : v.value<QVariantList>())
             assign(element);
}

// ------------------------------------------------------------------------------------------------
void
Port::set_nchannels(nchannels_t nchannels)
// todo: async call when graph is running
// ------------------------------------------------------------------------------------------------
{
    m_nchannels = nchannels;
    for (auto& connection : m_connections)
         connection->update();
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
QVariantList
Port::routing() const noexcept { return m_routing; }

// ------------------------------------------------------------------------------------------------
WPN_INCOMPLETE void
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
// returns true if Port is connected to target Port p
// ------------------------------------------------------------------------------------------------
{
    for (const auto& connection : m_connections)
    {
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
// returns true if this Port is connected to any Port in Node n
// returns false otherwise
// ------------------------------------------------------------------------------------------------
{
    for (const auto& connection: m_connections)
    {
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
// these are implemented here as not to break ODR

template<> audiobuffer_t
Port::buffer() noexcept { return m_buffer.audio; }

template<> midibuffer_t
Port::buffer() noexcept { return m_buffer.midi; }

// ------------------------------------------------------------------------------------------------
void
Port::reset()
// ------------------------------------------------------------------------------------------------
{
    if (m_type == Port::Midi_1_0)
        for (nchannels_t n = 0; n < m_nchannels; ++n)
             m_buffer.midi[n]->clear();
}

// ------------------------------------------------------------------------------------------------
void
Graph::register_node(Node& node) noexcept
// we should maybe add a vectorChanged signal-slot connection to this
// ------------------------------------------------------------------------------------------------
{
    qDebug() << "[GRAPH] registering node:" << node.name();

    QObject::connect(s_instance, &Graph::rateChanged, &node, &Node::on_rate_changed);
    m_nodes.push_back(&node);
}

// ------------------------------------------------------------------------------------------------
void
Port::add_connection(Connection* con)
{
    con->update();
    m_connections.push_back(con);
}

#include <source/io/external.hpp>

// ------------------------------------------------------------------------------------------------
Graph::Graph()
// ------------------------------------------------------------------------------------------------
{
    s_instance = this;
    m_external = new External;
}

// ------------------------------------------------------------------------------------------------
Connection&
Graph::connect(Port& source, Port& dest, Routing matrix)
// ------------------------------------------------------------------------------------------------
{
    assert(source.polarity() == Polarity::Output && dest.polarity() == Polarity::Input);
    assert(source.type() == dest.type());

    m_connections.emplace_back(source, dest, matrix);

    qDebug() << "[GRAPH] connected Node" << source.parent_node().name()
             << "(Port:" << source.name().append(")")
             << "with Node" << dest.parent_node().name()
             << "(Port:" << dest.name().append(")");

    for (nchannels_t n = 0; n < matrix.ncables(); ++n) {
        qDebug() << "[GRAPH] ---- routing: channel" << QString::number(matrix[n][0])
                 << ">> channel" << QString::number(matrix[n][1]);
    }

    return m_connections.back();
}

// ------------------------------------------------------------------------------------------------
Connection&
Graph::connect(Node& source, Node& dest, Routing matrix)
// ------------------------------------------------------------------------------------------------
{
    // find matching input/output pair
    Port* s_port = nullptr, *d_port = nullptr;

    if (!(s_port = source.default_port(Port::Audio, Polarity::Output)) ||
        !(d_port = dest.default_port(Port::Audio, Polarity::Input))) {
          s_port = source.default_port(Port::Midi_1_0, Polarity::Output);
          d_port = dest.default_port(Port::Midi_1_0, Polarity::Input);
    }

    return connect(*s_port, *d_port, matrix);
}

// ------------------------------------------------------------------------------------------------
Connection&
Graph::connect(Node& source, Port& dest, Routing matrix)
// ------------------------------------------------------------------------------------------------
{
    return connect(*source.default_port(dest.type(), Polarity::Output), dest, matrix);
}

// ------------------------------------------------------------------------------------------------
Connection&
Graph::connect(Port& source, Node& dest, Routing matrix)
// ------------------------------------------------------------------------------------------------
{
    return connect(source, *dest.default_port(source.type(), Polarity::Input), matrix);
}

// ------------------------------------------------------------------------------------------------
WPN_EXAMINE void
Graph::componentComplete()
// Graph is complete, all connections have been set-up
// we send signal to all registered Nodes to proceed to Port/pool allocation
// ------------------------------------------------------------------------------------------------
{   
    for (auto& connection : m_connections) {
        // when pushing in Graph::connect, wild undefined behaviour bugs appear..
        // better to do it in one run here
        connection.source()->add_connection(&connection);
        connection.dest()->add_connection(&connection);
    }

    Graph::debug("component complete, allocating nodes i/o");

    for (auto& node : m_nodes)
    {
        node->on_graph_complete(m_properties);
        // at this point, all io should have been done
        // we register all Nodes that do not have any output
        // to the list of direct subnodes
        // they won't be processed by the Graph otherwise
        if (!node->m_output_ports.size() &&
                !m_subnodes.contains(node))
            m_subnodes.push_back(node);
    }

    Graph::debug("i/o allocation complete, setting up external configuration");
    m_external->componentComplete();

    emit complete();
}

// ------------------------------------------------------------------------------------------------
WPN_AUDIOTHREAD vector_t
Graph::run() noexcept
// ------------------------------------------------------------------------------------------------
{
    vector_t nframes = m_properties.vector;

    for (auto& subnode : m_subnodes)
        subnode->process(nframes);

    for (auto& node : m_nodes)
        node->set_processed(false);

    return nframes;
}

// ------------------------------------------------------------------------------------------------
WPN_AUDIOTHREAD vector_t
Graph::run(Node& target) noexcept
// the main processing function
// Graph will process itself from target Node and upstream recursively
// ------------------------------------------------------------------------------------------------
{
    // take a little amount of time to process asynchronous graph update requests (TODO)
    WPN_TODO

    // process target, return outputs            
    vector_t nframes = m_properties.vector;
    target.process(nframes);

    for (auto& node : m_nodes)
         node->set_processed(false);

    return nframes;
}

#include <wpn114audio/spatial.hpp>

// ------------------------------------------------------------------------------------------------
Node::~Node()
// ------------------------------------------------------------------------------------------------
{
    if (m_spatial)
        delete m_spatial;

    Graph::instance().unregister_node(*this);
}

// ------------------------------------------------------------------------------------------------
Spatial*
Node::spatial()
// if qml requests access to spatial, we have to create it
// ------------------------------------------------------------------------------------------------
{
    if (!m_spatial) {
        qDebug() << m_name << "creating spatial attributes";

        m_spatial = new Spatial;
        // we make an implicit connection
        m_subnodes.push_front(m_spatial);
    }

    return m_spatial;
}

// ------------------------------------------------------------------------------------------------
void
Node::allocate_pools()
// ------------------------------------------------------------------------------------------------
{
    for (auto& port : m_input_ports)
        if (port->type() == Port::Audio)
             m_input_pool.audio.push_back(port->buffer<audiobuffer_t>());
        else m_input_pool.midi.push_back(port->buffer<midibuffer_t>());

    for (auto& port : m_output_ports)
        if (port->type() == Port::Audio)
             m_output_pool.audio.push_back(port->buffer<audiobuffer_t>());
        else m_output_pool.midi.push_back(port->buffer<midibuffer_t>());
}

// ------------------------------------------------------------------------------------------------
// CONNECTION
//-------------------------------------------------------------------------------------------------
Connection::Connection(Port& source, Port& dest, Routing matrix) :
    m_source   (&source),
    m_dest     (&dest),
    m_routing  (matrix)
{
    componentComplete();
}

// ------------------------------------------------------------------------------------------------
void
Connection::setTarget(const QQmlProperty& target)
// qml property binding, e. g.:
// Connection on 'Port' { level: db(-12); routing: [0, 1] }
// ------------------------------------------------------------------------------------------------
{
    auto port = target.read().value<Port*>();

    switch(port->polarity()) {
        case Polarity::Output: m_source = port; break;
        case Polarity::Input: m_dest = port;
    }

    componentComplete();
    Graph::instance().add_connection(*this);
}

// ------------------------------------------------------------------------------------------------
void
Connection::componentComplete()
{
    m_nchannels = std::min(m_source->nchannels(), m_dest->nchannels());
    m_mul = m_source->mul() * m_dest->mul();
    m_add = m_source->add() + m_dest->add();
    m_muted = m_source->muted() || m_dest->muted();
}

// ------------------------------------------------------------------------------------------------
void
Connection::update()
// ------------------------------------------------------------------------------------------------
{
    m_nchannels = std::min(m_source->nchannels(), m_dest->nchannels());
}

// ------------------------------------------------------------------------------------------------
WPN_AUDIOTHREAD void
Connection::pull(vector_t nframes) noexcept
// ------------------------------------------------------------------------------------------------
{       
    auto& source = m_source->parent_node();
    // if source hasn't been processed yet in the current Graph run
    // process it
    if (!source.processed())
        source.process(nframes);

    // if connection is muted return
    if (m_muted.load())
        return;

    // in the case of a MIDI connection
    if (m_source->type() == Port::Midi_1_0)
    {
        auto sbuf = m_source->buffer<midibuffer_t>();
        auto dbuf = m_dest->buffer<midibuffer_t>();

        Routing routing = m_routing;

        // append midi events to dest buffer
        // (without intermediate copy)
        if (routing.null())
            for (nchannels_t c = 0; c < m_nchannels; ++c)
                for (auto& mt : *sbuf[c])
                     dbuf[c]->push(mt);
        else
            for (nchannels_t c = 0; c < routing.ncables(); ++c)
                for (auto& mt : *sbuf[routing[c][0]])
                     dbuf[routing[c][1]]->push(mt);
        return;
    }

    // else Audio connection
    auto dbuf = m_dest->buffer<audiobuffer_t>();
    auto sbuf = m_source->buffer<audiobuffer_t>();

    sample_t mul = m_mul, add = m_add;
    Routing routing = m_routing;

    // if routing hasn't been explicitely set
    if (routing.null())
        for (nchannels_t c = 0; c < m_nchannels; ++c)
            for (vector_t f = 0; f < nframes; ++f)
                dbuf[c][f] += sbuf[c][f] * mul + add;           
    else
        for (nchannels_t c = 0; c < routing.ncables(); ++c)
            for (vector_t f = 0; f < nframes; ++f) {
                auto cable = routing[c];
                dbuf[cable[1]][f] += sbuf[cable[0]][f] * mul + add;
            }
}
