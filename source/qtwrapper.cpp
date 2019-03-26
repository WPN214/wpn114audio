#include "qtwrapper.hpp"
#include <QtDebug>

//-------------------------------------------------------------------------------------------------
// C-BRIDGE
// SOCKET
//-------------------------------------------------------------------------------------------------

Socket::Socket(Node& n, polarity_t p, QString l, nchn_t c, bool d) :
    parent(n), label(l.toStdString().c_str())
{
    n.addSocket(p,l,c,d);
}

//-------------------------------------------------------------------------------------------------
// C-BRIDGE
// NODE
//-------------------------------------------------------------------------------------------------

Node::Node()
{
    // register to the graph
    cnode = Graph::registerNode(*this);
}

void Node::addSocket(polarity_t p, QString l, nchn_t c, bool d)
{
    auto node = Graph::lookup(*this);
    wpn_socket_declare(node, p, l.toStdString().c_str(), c, d);
}

//-------------------------------------------------------------------------------------------------
// C-BRIDGE
// GRAPH
//-------------------------------------------------------------------------------------------------

wpn_node* Graph::lookup(Node& n)
{
    return wpn_node_lookup(&m_graph, &n);
}

wpn_node* Graph::registerNode(Node& n)
{
    return wpn_graph_register(&m_graph, &n, nullptr, node_cfg, node_prc);
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

//-------------------------------------------------------------------------------------------------
// QML-BRIDGE
// CONNECTION
//-------------------------------------------------------------------------------------------------

Connection::Connection()
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

#define CSTR(_t) _t.toStdString().c_str()

void Node::setTarget(QQmlProperty const& target)
{
    // assigning
    auto node = qobject_cast<Node*>(target.object());
    assert(node);

    auto socket = wpn_socket_lookup(
                  node->cnode, CSTR(target.name()));

    assert(socket);

    auto& graph = Graph::instance();

    switch(socket->polarity)
    {
    case INPUT:
    {
        wpn_graph_nsconnect(&graph, cnode, socket, {});
        break;
    }
    case OUTPUT:
    {
        if ( socket->default_ )
        {
            node->setDispatch(Dispatch::Downwards);
            node->append_subnode(this);
        }
        else wpn_graph_snconnect(&graph, socket, cnode, {});
    }
    }
}

void Node::componentComplete()
{
    if ( m_subnodes.empty())
         return;

    // set up hierarchy
    // depending on dispatch property
    for ( auto& subnode : m_subnodes )
          subnode->setParent(this);

    switch ( m_dispatch )
    {
    case Dispatch::Values::Upwards:
    {
        // all subnodes connect to this Node
        for ( auto& subnode : m_subnodes )
            Graph::connect(subnode->chainout(), *this);
        break;
    }
    case Dispatch::Values::Downwards:
    {
        // chain the signal down the line
        auto& front = *m_subnodes.front();
        Graph::connect(*this, front.chainout(), {});

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
    // this is what is being returned as a qml value
    // it may be a reference to a specific Socket,
    // a constant value
    // or an array of either of them
    return QVariant(0);
}

void Node::sgwr(Socket& s, QVariant v)
{
    // parse the socket property assignment
    if ( v.canConvert<Socket*>())
    {
        Graph::disconnect(s);
        Graph::connect(s, *v.value<Socket*>());
    }
    else if ( v.canConvert<QVariantList>())
    {

    }
    else if ( v.canConvert<qreal>())
    {

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

}

void Output::setRate(sample_t rate)
{
    m_rate = rate;
}

void Output::setNchannels(quint16 nchannels)
{
    m_nchannels = nchannels;
    // TODO
}

void Output::setVector(quint16 vector)
{
    m_vector = vector;
}

void Output::setFeedback(quint16 feedback)
{
    m_feedback = feedback;
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
    Graph::initialize();
    emit startStream();
}

void Output::componentComplete()
{
    Node::componentComplete();

    RtAudio::Api api = RtAudio::UNSPECIFIED;
    if ( m_api == "JACK" )
         api = RtAudio::UNIX_JACK;
    else if ( m_api == "CORE" )
         api = RtAudio::MACOSX_CORE;
    else if ( m_api == "ALSA" )
         api = RtAudio::LINUX_ALSA;
    else if ( m_api == "PULSE" )
         api = RtAudio::LINUX_PULSE;

    RtAudio audio(api);
    RtAudio::StreamParameters parameters;
    RtAudio::DeviceInfo info;
    RtAudio::StreamOptions options;

    auto ndevices = audio.getDeviceCount();

    if ( !m_device.isEmpty())
    {
        for ( quint32 d = 0; d < ndevices; ++d )
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
    else parameters.deviceId = audio.getDefaultOutputDevice();

    auto selected_info = audio.getDeviceInfo(parameters.deviceId);

    qDebug() << "SELECTED API"
             << QString::fromStdString(audio.getApiName(audio.getCurrentApi()));
    qDebug() << "SELECTED_AUDIO_DEVICE"
             << QString::fromStdString(selected_info.name);

    parameters.nChannels    = m_nchannels;
    options.streamName      = "WPN114";
    options.flags           = RTAUDIO_SCHEDULE_REALTIME;
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
    Graph::configure(m_rate, m_vector, m_feedback);
    m_audiothread.start(QThread::TimeCriticalPriority);
}

void Output::configure(wpn_graph_properties) {}
void Output::rwrite(wpn_pool&, wpn_pool&, vector_t) {}

Output::~Output()
{
    emit exitStream();
    m_audiothread.quit();

    if ( !m_audiothread.wait(3000))
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
    m_vector = m_outmodule.vector();

    try
    {
        m_stream->openStream(
            &m_parameters,                      // parameters
            nullptr,                            // dunno
            m_format,                           // format
            m_outmodule.rate(),                 // sample-rate
            &m_vector,                          // vector-size
            &rwrite,                            // audio callback
            static_cast<void*>(&m_outmodule),   // user-data
            &m_options,                         // options
            nullptr                             // dunno
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
    //outmod.audiostream().buffer_processed(time);

    auto strm = Graph::run(outmod);
    strm.interleaved(static_cast<sample_t*>(out));

    return 0;
}

#define SAMPLE_RATE cnode->properties.rate

void Sinetest::configure(wpn_graph_properties properties) {}
void VCA::configure(wpn_graph_properties properties) {}

Sinetest::Sinetest()
{

}

#define foreach_channel(_ch, _lim) for ( nchn_t _ch = 0; _ch < _lim; ++_ch)
#define foreach_sample(_s, _lim) for ( size_t _s = 0; _s < _lim; ++_s)
#define sample(_cacc, _s) _cacc.data [_s]

typedef channel_accessor c_acc;

void Sinetest::rwrite(wpn_pool& inputs, wpn_pool& outputs, vector_t sz)
{
    auto frequency  = strmextr(inputs, "frequency");
    auto out        = strmextr(outputs, "outputs");
    auto wt         = sstream_access(&m_wtable, 0, 0);
    size_t pos      = m_pos;

    foreach_sample(s, sz)
    {
        pos += frequency->data[s]/SAMPLE_RATE*16384;
        out->data[s] = wt.data[pos];
        wpnwrap(pos, 16384);
    }

    m_pos = pos;
}

VCA::VCA() {}

void VCA::rwrite(wpn_pool& inputs, wpn_pool& outputs, vector_t sz)
{
    auto input  = strmextr(inputs, "inputs");
    auto gain   = strmextr(inputs, "gain");
    auto out    = strmextr(outputs, "outputs");

    foreach_sample(s, sz)
        out->data[s] = input->data[s] * gain->data[s];
}

Delay::Delay()
{

}

void Delay::configure(wpn_graph_properties properties)
{
    m_dline = hstream_alloc(1, NON_INTERLEAVED, properties.rate * 5);
}

void Delay::rwrite(wpn_pool& inputs, wpn_pool& outputs, vector_t nframes)
{
    auto input  = strmextr(inputs, "inputs");
    auto delay  = strmextr(inputs, "delay");
    auto mix    = strmextr(inputs, "mix");
    auto out    = strmextr(outputs, "outputs");

    auto dline_r = hstream_access(m_dline, m_rpos, nframes);
    auto dline_w = hstream_access(m_dline, m_wpos, nframes);

}



