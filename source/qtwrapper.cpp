#include "qtwrapper.hpp"
#include <QtDebug>

#define CSTR(_t) _t.toLocal8Bit().data()
//-------------------------------------------------------------------------------------------------
// C-BRIDGE
// SOCKET
//-------------------------------------------------------------------------------------------------
Socket::Socket() :
    parent(nullptr), nchannels(0), default_(false)
{

}

Socket::Socket(Node& n, polarity_t p, std::string l, nchn_t c, bool d) :
    parent(&n), label(l), polarity(p), nchannels(c), default_(d)
{
    // TODO, optimize, do not store these chunks of data here,
    // they become obsolete after parent node is registered to the graph
    n.addSocket(*this);
}

Socket::Socket(Socket const& copy) :
    parent(copy.parent), label(copy.label), polarity(copy.polarity),
    nchannels(copy.nchannels), default_(copy.default_)
{

}

QVariant Socket::routing() const {
    return QVariant(0);
}

void Socket::setRouting(QVariant v)
{

}

Socket::~Socket() {}

//-------------------------------------------------------------------------------------------------
// C-BRIDGE
// NODE
//-------------------------------------------------------------------------------------------------

Node::Node()
{
}

void Node::addSocket(Socket& s)
{
    m_sockets.push_back(&s);
}

void node_dcl(wpn_node* node)
{
    Node* cppnode = static_cast<Node*>(node->udata);
    strcpy(node->label, cppnode->nreference().c_str());

    for ( const auto& socket : cppnode->sockets() )
    {
        wpn_socket_declare(node,
            socket->polarity,
            socket->label.c_str(),
            socket->nchannels,
            socket->default_);

        socket->csocket = wpn_socket_lookup_p(node,
                          socket->polarity,
                          socket->label.c_str());

        if ( socket->pending_value > 0 ||
             socket->pending_value < 0 )
        {
            auto acc = hstream_access(socket->csocket->stream, 0, 0);
            stream_fillv(&acc, socket->pending_value);
        }
    }

    if (cppnode->m_target_callback)
        cppnode->m_target_callback->parseTarget();
}

inline void node_cfg(wpn_graph_properties p, void* udata)
{
    static_cast<Node*>(udata)->configure(p);
}

inline void node_prc(wpn_pool* i, wpn_pool* o, void* u, vector_t sz)
{
    static_cast<Node*>(u)->rwrite(*i, *o, sz);
}

//-------------------------------------------------------------------------------------------------
// C-BRIDGE
// GRAPH
//-------------------------------------------------------------------------------------------------

wpn_graph Graph::m_graph;


wpn_graph& Graph::instance()
{
    return m_graph;
}

Graph::Graph()
{
    m_graph = wpn_graph_create(44100, 512, 64);
}

Graph::~Graph()
{
    wpn_graph_free(&m_graph);
}

void Graph::setRate(sample_t rate)
{
    m_graph.properties.rate = rate;
}

void Graph::setVector(quint16 vector)
{
    m_graph.properties.vecsz = vector;
}

void Graph::setFeedback(quint16 feedback)
{
    m_graph.properties.vecsz_fb = feedback;
}

wpn_node* Graph::lookup(Node& n)
{
    return wpn_node_lookup(&m_graph, &n);
}

wpn_node* Graph::registerNode(Node& n)
{
    return wpn_graph_register(&m_graph, &n, node_dcl, node_cfg, node_prc, nullptr);
}

inline wpn_connection&
Graph::connect(Node& source, Node& dest, wpn_routing routing)
{
    return *wpn_graph_nconnect(&m_graph, source.cnode, dest.cnode, routing);
}

inline wpn_connection&
Graph::connect(Socket& source, Socket& dest, wpn_routing routing)
{
    return *wpn_graph_sconnect(&m_graph, source.csocket, dest.csocket, routing);
}

inline wpn_connection&
Graph::connect(Node& source, Socket& dest, wpn_routing routing)
{
    return *wpn_graph_nsconnect(&m_graph, source.cnode, dest.csocket, routing);
}

inline wpn_connection&
Graph::connect(Socket &source, Node &dest, wpn_routing routing)
{
    return *wpn_graph_snconnect(&m_graph, source.csocket, dest.cnode, routing);
}

inline void Graph::disconnect(Socket& s, wpn_routing routing)
{
    wpn_graph_sdisconnect(&m_graph, s.csocket, routing);
}

inline wpn_pool* Graph::run(Node& node)
{
    return wpn_graph_run(&m_graph, &node);
}

QQmlListProperty<Node> Graph::subnodes()
{
    return QQmlListProperty<Node>(
        this, this,
        &Graph::append_subnode,
        &Graph::nsubnodes,
        &Graph::subnode,
        &Graph::clear_subnodes);
}

Node* Graph::subnode(int index) const
{
    return m_subnodes[index];
}

void Graph::append_subnode(Node* n)
{
    m_subnodes.push_back(n);
}

int Graph::nsubnodes() const
{
    return m_subnodes.count();
}

void Graph::clear_subnodes()
{
    m_subnodes.clear();
}

void Graph::append_subnode(QQmlListProperty<Node>* l, Node* n)
{
    reinterpret_cast<Graph*>(l->data)->append_subnode(n);
}

void Graph::clear_subnodes(QQmlListProperty<Node>* l)
{
    reinterpret_cast<Graph*>(l->data)->clear_subnodes();
}

Node* Graph::subnode(QQmlListProperty<Node>* l, int index)
{
    return reinterpret_cast<Graph*>(l->data)->subnode(index);
}

int Graph::nsubnodes(QQmlListProperty<Node>* l)
{
    return reinterpret_cast<Graph*>(l->data)->nsubnodes();
}

//-------------------------------------------------------------------------------------------------
// QML-BRIDGE
// CONNECTION
//-------------------------------------------------------------------------------------------------

Connection::Connection()
{

}

Connection::Connection(Connection const& copy)
{

}

qreal Connection::db(qreal v)
{
    return dbtoa(v);
}

void Connection::setTarget(const QQmlProperty& property)
{

}

void Connection::componentComplete()
{

}

void Connection::setPattern(Pattern pattern)
{
    m_pattern = pattern;
}

void Connection::setRouting(QVariant routing)
{
    m_routing = routing;
}

void Connection::setSource(Socket *source)
{
    m_source = source;
}

void Connection::setDest(Socket *dest)
{
    m_dest = dest;
}

void Connection::setLevel(qreal level)
{
    m_cconnection->level = level;
}

void Connection::setMuted(bool muted)
{
    m_cconnection->muted = muted;
}

void Connection::setActive(bool active)
{
    m_cconnection->active = active;
}

//-------------------------------------------------------------------------------------------------
// QML-BRIDGE
// NODE
//-------------------------------------------------------------------------------------------------

Node& Node::chainout()
{
    // returns the chain's last output Node
    switch ( m_dispatch )
    {
    case Dispatch::Values::Upwards:
    {
        return *this;
    }
    case Dispatch::Values::Downwards:
    {
        if ( !m_subnodes.count() )
             return *this;
        else return *m_subnodes.back();
    }
    }
}

void Node::setTarget(QQmlProperty const& target)
{
    auto node = qobject_cast<Node*>(target.object());
    node->m_target_callback = this;

    for ( const auto& socket : node->sockets())
        if ( QString::fromStdString(socket->label) == target.name())
             m_target = socket;
}

void Node::parseTarget()
{
    auto socket = m_target->csocket;
    assert(socket);

    auto& graph = Graph::instance();

    switch(socket->polarity)
    {
    case INPUT:
    {
        wpn_graph_nsconnect(&graph, cnode, socket, parseRouting());
        break;
    }
    case OUTPUT:
    {
        if ( socket->default_ )
        {
            m_target->parent->setDispatch(Dispatch::Downwards);
            m_target->parent->append_subnode(this);
        }

        else wpn_graph_snconnect(&graph, socket, cnode, parseRouting());
    }
    }
}

wpn_routing Node::parseRouting()
{
    wpn_routing routing {};

    if (m_routing.isEmpty())
        return routing;

    for ( int n = 0; n < m_routing.size(); n += 2)
    {
        routing.cables[n][0] = m_routing[n].toInt();
        routing.cables[n][1] = m_routing[n+1].toInt();
        routing.ncables++;
    }

    return routing;
}

void Node::componentComplete()
{
    // register node
    // graph will then call the socket-register
    // and configure callbacks
    cnode = Graph::registerNode(*this);

    // set up hierarchy
    // depending on dispatch property
    if ( m_subnodes.empty())
         return;

    for ( auto& subnode : m_subnodes )
          subnode->setParent(this);

    switch ( m_dispatch )
    {
    case Dispatch::Values::Upwards:
    {
        // all subnodes connect to this Node
        for ( auto& subnode : m_subnodes )
        {
            auto& source = subnode->chainout();
            Graph::connect(source, *this, source.parseRouting());
        }
        break;
    }
    case Dispatch::Values::Downwards:
    {
        // chain the signal down the line
        auto& front = *m_subnodes.front();
        Graph::connect(*this, front.chainout());

        for ( int n = 0; n < m_subnodes.count()-1; ++n)
        {
            auto& source = m_subnodes[n]->chainout();
            auto& dest = m_subnodes[n+1]->chainout();

            Graph::connect(source, dest);
        }
    }
    }
}

QVariant Node::sgrd(Socket& s)
{
    return QVariant::fromValue(s);
}

void Node::sgwr(Socket& s, QVariant v)
{
    // parse the socket property assignment
    if ( v.canConvert<Socket*>())
    {
        // single-assignment case
        // replacing any other connection
        // for multiple connections, we'd use the Connection class instead
        Graph::disconnect(s);
        Graph::connect(s, *v.value<Socket*>());
    }
    else if ( v.canConvert<Connection*>())
    {
        auto connection = v.value<Connection*>();
        if (s.polarity == INPUT) {
            auto source = connection->source();

            // routing TODO
            auto routing = connection->routing();
            Graph::connect(*source, s);
        } else {
            auto dest = connection->dest();
            auto routing = connection->routing();
            Graph::connect(s, *dest);
        }
    }
    else if ( v.canConvert<QVariantList>())
    {
        // TODO!
    }
    else if ( v.canConvert<qreal>())
    {
        auto value = v.toReal();
        // this call might occur before component is complete
        // in which case socket hasn't been declared to the graph yet
        if ( !s.csocket ) {
              s.pending_value = value;
              return;
        }

        auto acc = hstream_access(s.csocket->stream, 0, 0);
        stream_fillv(&acc, value);
    }
}

void Node::setMuted(bool muted)
{
    m_muted = muted;
}

void Node::setActive(bool active)
{
    m_active = active;
}

void Node::setLevel(qreal level)
{
    m_level = level;
}

void Node::setParent(Node* parent)
{
    m_parent = parent;
}

void Node::setDispatch(Dispatch::Values v)
{
    m_dispatch = v;
}

void Node::setRouting(QVariantList routing)
{
    m_routing = routing;
}

qreal Node::db(qreal v)
{
    // this is not an a->db function
    // it is a useful, readable and declarative way to signal that the value
    // used within qml is of the db unit.
    // consequently, it returns the value as a normal linear amplitude value.
    // in general, to ease-up calculations, the db should be calculated once and upfront
    return dbtoa(v);
}

QQmlListProperty<Node> Node::subnodes()
{
    return QQmlListProperty<Node>(
        this, this,
        &Node::append_subnode,
        &Node::nsubnodes,
        &Node::subnode,
        &Node::clear_subnodes);
}

Node* Node::subnode(int index) const
{
    return m_subnodes[index];
}

void Node::append_subnode(Node* n)
{
    m_subnodes.push_back(n);
}

int Node::nsubnodes() const
{
    return m_subnodes.count();
}

void Node::clear_subnodes()
{
    m_subnodes.clear();
}

void Node::append_subnode(QQmlListProperty<Node>* l, Node* n)
{
    reinterpret_cast<Node*>(l->data)->append_subnode(n);
}

void Node::clear_subnodes(QQmlListProperty<Node>* l)
{
    reinterpret_cast<Node*>(l->data)->clear_subnodes();
}

Node* Node::subnode(QQmlListProperty<Node>* l, int index)
{
    return reinterpret_cast<Node*>(l->data)->subnode(index);
}

int Node::nsubnodes(QQmlListProperty<Node>* l)
{
    return reinterpret_cast<Node*>(l->data)->nsubnodes();
}

//-------------------------------------------------------------------------------------------------
// QML-BRIDGE
// NODE
//-------------------------------------------------------------------------------------------------

Output::Output()
{
    m_nreference = "Output";
}

void Output::setNchannels(quint16 nchannels)
{
    m_nchannels = nchannels;
    // TODO
}

void Output::setApi(QString api)
{
    m_api = api;
}

void Output::setDevice(QString device)
{
    m_device = device;
}

void Output::start()
{
    emit startStream();
}

void Output::componentComplete()
{
    Node::componentComplete();

    // allocate inputs
    auto inputs  = wpn_socket_lookup(cnode, "inputs");
    auto outputs = wpn_socket_lookup(cnode, "outputs");

    inputs->stream  = hstream_alloc(m_nchannels, NON_INTERLEAVED, cnode->properties.vecsz);
    outputs->stream = hstream_alloc(m_nchannels, NON_INTERLEAVED, cnode->properties.vecsz);

    RtAudio::Api api = RtAudio::UNSPECIFIED;

    if ( m_api == "JACK" )
         api = RtAudio::UNIX_JACK;
    else if ( m_api == "CORE" )
         api = RtAudio::MACOSX_CORE;
    else if ( m_api == "ALSA" )
         api = RtAudio::LINUX_ALSA;
    else if ( m_api == "PULSE" ) {
         api = RtAudio::LINUX_PULSE;
    }

    RtAudio audio(api);
    RtAudio::StreamParameters parameters;
    RtAudio::DeviceInfo info;
    RtAudio::StreamOptions options;

    auto ndevices = audio.getDeviceCount();

    if (!m_device.isEmpty())
    {
        for (quint32 d = 0; d < ndevices; ++d)
        {
            auto name = QString::fromStdString(
                        audio.getDeviceInfo(d).name);

            if (name.contains(m_device))
            {
                parameters.deviceId = d;
                break;
            }
        }
    }

    else
    {
        parameters.deviceId = audio.getDefaultOutputDevice();
    }

    auto selected_info = audio.getDeviceInfo(parameters.deviceId);

    qDebug() << "SELECTED API"
             << QString::fromStdString(audio.getApiName(audio.getCurrentApi()));
    qDebug() << "SELECTED_AUDIO_DEVICE"
             << QString::fromStdString(selected_info.name);

    parameters.nChannels    = m_nchannels;
    options.streamName      = "WPN114";
    options.flags           = RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_NONINTERLEAVED;
    options.priority        = 40;

    m_stream = std::make_unique<Audiostream>
               (*this, parameters, info, options);

    m_stream->moveToThread(&m_audiothread);

    QObject::connect(this, &Output::configureStream, &*m_stream, &Audiostream::preconfigure);
    QObject::connect(this, &Output::startStream, &*m_stream, &Audiostream::start);
    QObject::connect(this, &Output::stopStream, &*m_stream, &Audiostream::stop);
    QObject::connect(this, &Output::restartStream, &*m_stream, &Audiostream::restart);
    QObject::connect(this, &Output::exitStream, &*m_stream, &Audiostream::exit);

    emit configureStream();
    m_audiothread.start(QThread::TimeCriticalPriority);
}

void Output::configure(wpn_graph_properties) {}

void Output::rwrite(wpn_pool& inputs, wpn_pool& outputs, vector_t)
{
    auto in  = strmextr(&inputs, "inputs");
    auto out = strmextr(&outputs, "outputs");

    stream_cprp(in, out);
    // add master level
}

Output::~Output()
{
    emit exitStream();
    m_audiothread.quit();

    if (!m_audiothread.wait(3000))
    {
        m_audiothread.terminate();
        m_audiothread.wait();
        m_audiothread.deleteLater();
    }
}

//=================================================================================================
// AUDIOSTREAM
//=================================================================================================

Audiostream::Audiostream(Output& out,
    RtAudio::StreamParameters parameters,
    RtAudio::DeviceInfo info,
    RtAudio::StreamOptions options) :
    m_parameters(parameters),
    m_outmodule(out),
    m_device_info(info),
    m_options(options),
    m_stream(std::make_unique<RtAudio>())
{

}

void Audiostream::preconfigure()
{
    m_format = WPN114_RT_PRECISION;
    m_vector = Graph::vector();

    try
    {
        m_stream->openStream(
            &m_parameters,                      // output parameters
            nullptr,                            // input parameters
            m_format,                           // format
            Graph::rate(),                      // sample-rate
            &m_vector,                          // vector-size
            &rwrite,                            // audio callback
            static_cast<void*>(&m_outmodule),   // user-data
            &m_options,                         // options
            nullptr                             // error callback
        );
    }

    catch (RtAudioError const& e) {
        qDebug() << "OPENSTREAM_ERROR!"
                 << QString::fromStdString(e.getMessage());
    }
}

void Audiostream::start()
{
    try     { m_stream->startStream(); }
    catch   ( RtAudioError const& e )
    {
        qDebug() << "STARTSTREAM_ERROR!"
                 << QString::fromStdString(e.getMessage());
    }
}

void Audiostream::stop()
{
    try     { m_stream->stopStream(); }
    catch   ( RtAudioError const& e )
    {
        qDebug() << "STOPSTREAM_ERROR!"
                 << QString::fromStdString(e.getMessage());
    }
}

void Audiostream::restart()
{

}

void Audiostream::exit()
{
    try
    {
        m_stream->stopStream();
        m_stream->closeStream();
    }

    catch ( RtAudioError const& e)
    {
        qDebug() << "ERROR_ON_EXIT!"
                 << QString::fromStdString(e.getMessage());
    }
}

//=================================================================================================
// MAIN_AUDIO_CALLBACK
//=================================================================================================

int rwrite(void* out, void* in, unsigned int nframes,
           double time, RtAudioStreamStatus status,
           void* udata)
{
    Output& outmod = *static_cast<Output*>(udata);
    sample_t* output = static_cast<sample_t*>(out);

    //outmod.audiostream().buffer_processed(time);

    auto pool = Graph::run(outmod);
    wpn_stream* stream = static_cast<wpn_stream*>(vector_at(pool, 0));
    auto acc = stream->stream;

    for (size_t n = 0; n < acc.nsamples; ++n )
         output[n] = acc.data[n];

    return 0;
}

void VCA::configure(wpn_graph_properties properties) {}

Sinetest::Sinetest()
{
    m_nreference = "Sinetest";
}

#define foreach_channel(_ch, _lim) for ( nchn_t _ch = 0; _ch < _lim; ++_ch)
#define foreach_sample(_s, _lim) for ( size_t _s = 0; _s < _lim; ++_s)
#define sample(_cacc, _s) _cacc.data [_s]

typedef channel_accessor c_acc;

void Sinetest::configure(wpn_graph_properties properties)
{
    for (size_t s = 0; s < 16384; ++s)
         m_wtable.data[s] = sin(s/16384.0*M_PI*2.0);
}

void Sinetest::rwrite(wpn_pool& inputs, wpn_pool& outputs, vector_t sz)
{
    auto frequency  = strmextr(&inputs, "frequency");
    auto out        = strmextr(&outputs, "outputs");
    size_t pos      = m_pos;

    for (size_t s = 0; s < sz; ++s)
    {
        pos += frequency->data[s]/SAMPLE_RATE*16384;
        out->data[s] = m_wtable.data[pos];
        wpnwrap(pos, 16384);        
    }

    m_pos = pos;
}

VCA::VCA()
{
    m_nreference = "VCA";
}

void VCA::rwrite(wpn_pool& inputs, wpn_pool& outputs, vector_t sz)
{
    auto input  = strmextr(&inputs, "inputs");
    auto gain   = strmextr(&inputs, "gain");
    auto out    = strmextr(&outputs, "outputs");

    foreach_sample(s, sz)
        out->data[s] = input->data[s] * gain->data[s];
}
