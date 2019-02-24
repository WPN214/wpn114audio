
#include "graph.hpp"
#include <math.h>
#include <QtDebug>

//=================================================================================================
// pin
//=================================================================================================
using namespace wpn114::signal;

pin::pin(node& parent, enum polarity_t p, std::string label, size_t nchannels, bool def) :
    m_parent(parent),
    m_polarity(p),
    m_label(label),
    m_nchannels(nchannels),
    m_default(def)
{
    parent.add_pin(*this);
}

node& pin::parent_node()
{
    return m_parent;
}

std::string pin::label() const
{
    return m_label;
}

bool pin::is_default() const
{
    return m_default;
}

polarity_t pin::polarity() const
{
    return m_polarity;
}

std::vector<std::shared_ptr<connection>> pin::connections()
{
    return m_connections;
}

pin::_stream&
pin::get_stream()
{
    return m_stream;
}

inline void
pin::allocate(size_t sz)
{
    m_stream.allocate(m_nchannels, sz);
}

inline void
pin::add_connection(connection& con)
{
    m_connections.push_back(std::make_shared<connection>(con));
}

inline void
pin::remove_connection(connection& con)
{
    std::remove(m_connections.begin(),
                m_connections.end(),
                std::make_shared<connection>(con));
}

bool pin::connected() const
{
    return m_connections.size();
}

template<typename T>
bool pin::connected(T& target) const
{
    switch( m_polarity )
    {
    case polarity_t::input:
    {
        for ( const auto& connection : m_connections )
            if ( connection->is_dest(target))
                 return true;
        break;
    }
    case polarity_t::output:
    {
        for ( const auto& connection : m_connections )
            if ( connection->is_source(target))
                 return true;
        break;
    }
    }
    return false;
}

//=================================================================================================
// NODE
//=================================================================================================

inline std::vector<pin*>&
node::pvector(polarity_t p)
{
    switch(p)
    {
    case polarity_t::input:
         return m_uppins;
    case polarity_t::output:
         return m_dnpins;
    }
}

inline void
node::add_pin(pin& p)
{
    auto& pvec = pvector(p.polarity());
    pvec.push_back(&p);
}

inline QVariant
node::sgrd(pin& p)
{
    return QVariant::fromValue(&p);
}

void
node::sgwr(pin& p, QVariant v)
{
    if ( v.canConvert<pin*>())
    {
        graph::disconnect(p);
        graph::connect(p, *v.value<pin*>());
    }
    else if ( v.canConvert<QVariantList>())
    {

    }
    else if ( v.canConvert<qreal>())
    {
        p.get_stream()() <<= v.value<qreal>();
    }
}

WPN_TODO
QVariant node::connection(QVariant svar, qreal level, QVariant map)
{
    // establish an explicit connection
    // with level and map parameters
    return QVariant(0);
}

qreal node::db(qreal v)
{
    // this is not an a->db function
    // it is a useful, readable and declarative way to signal that the value
    // used within qml is of the db unit.
    // consequently, it returns the value as a normal linear amplitude value.
    // in general, to ease-up calculations, the db should be calculated once and upfront
    return dbtoa(v);
}

node::pool::pool(pinvector& vector, size_t pos, size_t sz)
{
    for ( auto& pin : vector )
    {
        node::pstream ps = {
            pin->label(),
            pin->get_stream()(pos, sz, 0)
        };

        streams.push_back(ps);
    }
}

stream<signal_t, mallocator<signal_t>>::slice&
node::pool::operator[](std::string str)
{
    for ( auto& stream : streams )
          if ( stream.label == str )
               return stream.slice;
    assert(0);
}

//-------------------------------------------------------------------------------------------------

template<> bool
node::connected(node& other)
{
    for (auto& pin : m_dnpins)
        if (pin->connected(other))
            return true;

    for (auto& pin : m_uppins)
        if (pin->connected(other))
            return true;
    return false;
}

template<> bool
node::connected(pin& other)
{
    for ( auto& pin : pvector(other.polarity()))
        if ( pin->connected(other))
             return true;
    return false;
}

//-------------------------------------------------------------------------------------------------
pin* node::iopin(QString ref)
{
    if ( pin* p = inpin(ref))
         return p;
    else return outpin(ref);
}

pin* node::inpin()
{
    for ( auto& pin : m_uppins )
          if ( pin->is_default() )
               return pin;
    return nullptr;
}

pin* node::inpin(QString ref)
{
    for ( auto& pin : m_uppins )
          if (pin->label() == ref.toStdString())
              return pin;
    return nullptr;
}

pin* node::outpin()
{
    for ( auto& pin : m_dnpins )
          if ( pin->is_default() )
               return pin;
    return nullptr;
}

pin* node::outpin(QString ref)
{
    for ( auto& pin : m_dnpins )
          if ( pin->label() == ref.toStdString())
               return pin;
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

node* node::parent() const
{
    return m_parent;
}

void node::setParent(node* n)
{
    if ( m_parent == n ) return;
    m_parent = n;
}

void node::setDispatch(Dispatch::Values d)
{
    m_dispatch = d;
}

node& node::chainout()
{
    switch( m_dispatch )
    {
    case Dispatch::Values::Upwards:
    {
        return *this;
    }
    case Dispatch::Values::Downwards:
    {
        if ( !m_subnodes.size() )
             return *this;
        else return *m_subnodes.back();
    }
    }
}

void node::setTarget(QQmlProperty const& property)
{
    // when a node is bound to another node's signal property
    auto _node = qobject_cast<node*>(property.object());
    assert(_node);

    auto _pin = _node->iopin(property.name());
    assert(_pin);

    switch(_pin->polarity())
    {
    case polarity_t::input:
    {
        graph::connect(*this, *_pin);
        break;
    }
    case polarity_t::output:
    {
        if (_pin->is_default())
        {
            _node->setDispatch(Dispatch::Downwards);
            _node->append_subnode(this);
        }
        else graph::connect(*_pin, *this);
    }
    }
}

void node::componentComplete()
{
    if ( m_subnodes.empty()) return;

    for ( auto& subnode : m_subnodes )
          subnode->setParent(this);

    // node mustn't take care of the connections
    // to be established with its parent,
    // but only with its subnodes

    switch( m_dispatch )
    {
    case Dispatch::Values::Upwards:
    {
        for ( auto& subnode : m_subnodes )
              graph::connect(subnode->chainout(), *this);
        break;
    }
    case Dispatch::Values::Downwards:
    {
        auto& front = *m_subnodes.front();
        graph::connect(*this, front.chainout());

        for ( size_t n = 0; n < m_subnodes.size()-1; ++n)
        {
            auto& source = m_subnodes[n]->chainout();
            auto& dest = m_subnodes[n+1]->chainout();

            graph::connect(source, dest);
        }
    }
    }
}

QQmlListProperty<node> node::subnodes()
{
    return QQmlListProperty<node>(
        this, this,
        &node::append_subnode,
        &node::nsubnodes,
        &node::subnode,
        &node::clear_subnodes );
}

node* node::subnode(int index) const
{
    WPN_TODO // check size
    return m_subnodes[index];
}

void node::append_subnode(node* n)
{
    m_subnodes.push_back(n);
}

int node::nsubnodes() const
{
    return m_subnodes.size();
}

void node::clear_subnodes()
{
    m_subnodes.clear();
}

// ------------------------------------------------------------------------------------------------

void node::append_subnode(QQmlListProperty<node>* l, node * n)
{
    reinterpret_cast<node*>(l->data)->append_subnode(n);
}

void node::clear_subnodes(QQmlListProperty<node>* l)
{
    reinterpret_cast<node*>(l->data)->clear_subnodes();
}

node* node::subnode(QQmlListProperty<node>* l, int i)
{
    return reinterpret_cast<node*>(l->data)->subnode(i);
}

int node::nsubnodes(QQmlListProperty<node>* l)
{
    return reinterpret_cast<node*>(l->data)->nsubnodes();
}

// ------------------------------------------------------------------------------------------------

WPN_TODO
typename mstream::slice
node::collect(polarity_t p)
{
    pinvector* target;

    // this is used for the output module
    // we get input pin's stream
    // get a vector-size slice from it at stream_pos
    return m_uppins[0]->get_stream()(m_stream_pos, m_properties.vsz, 0);
}

void node::initialize(graph_properties properties)
{
    m_properties = properties;

    for ( auto& pin : m_uppins )
          pin->allocate(properties.vsz);

    for ( auto& pin : m_dnpins )
          pin->allocate(properties.vsz);
}

void node::process()
{
    qDebug() << QString::fromStdString(nreference())
             << "NODE::PROCESS";

    size_t sz = m_intertwined     ?
                m_properties.vsz  :
                m_properties.fvsz ;

    for ( auto& pin : m_uppins )
        for ( auto& connection : pin->connections())
              connection->pull(sz);

    node::pool uppool(m_uppins, m_stream_pos, sz),
               dnpool(m_dnpins, m_stream_pos, sz);

    rwrite(uppool, dnpool, sz);

    m_stream_pos += sz;
    wrap(m_stream_pos, m_properties.vsz);
}

//=================================================================================================
// CONNECTION
//=================================================================================================

void connection::pull(size_t sz)
{
    qDebug() << "PULLING CONNECTION BETWEEN"
             << QString::fromStdString(m_source.parent_node().nreference())
             << QString::fromStdString(m_dest.parent_node().nreference());

    if ( !m_active )
         return;

    if ( !m_feedback )
         while ( m_source.m_parent.m_stream_pos < sz )
                 m_source.m_parent.process();

    if ( m_muted )
    {
        auto slice = m_stream.sync_draw(sz);
        slice *= m_level;
        m_stream.sync_pour(sz);
    }
    else
    {
        m_stream.upsync_skip(sz);
        m_stream.dnsync_skip(sz);
    }
}

//-----------------------------------------------------------------------------

connection::connection(pin& source, pin& dest, pattern pattern) :
    m_source(source), m_dest(dest), m_pattern(pattern) { }

WPN_TODO
connection::connection(connection const& cp) :
    m_source(cp.m_source), m_dest(cp.m_dest)
{

}

WPN_TODO
connection::connection(connection&& mv) :
    m_source(mv.m_source), m_dest(mv.m_dest)
{

}

WPN_TODO
connection& connection::operator=(connection const& cp)
{

}

WPN_TODO
connection& connection::operator=(connection&& mv)
{

}

bool connection::operator==(connection const& other)
{
    return ( &m_source == &other.m_source ) &&
           ( &m_dest == &other.m_dest );
}

void connection::allocate(size_t sz)
{
    auto nchannels = std::max(
                m_source.m_nchannels,
                m_dest.m_nchannels);

    m_stream.allocate(nchannels, sz);
}

//-----------------------------------------------------------------------------

void connection::open()
{
    m_active = true;
}

void connection::close()
{
    m_active = false;
}

void connection::mute()
{
    m_muted = true;
}

void connection::unmute()
{
    m_muted = false;
}

//-----------------------------------------------------------------------------

connection& connection::operator+=(signal_t v)
{
    m_level += v; return *this;
}

connection& connection::operator-=(signal_t v)
{
    m_level -= v; return *this;
}

connection& connection::operator*=(signal_t v)
{
    m_level *= v; return *this;
}

connection& connection::operator/=(signal_t v)
{
    m_level /= v; return *this;
}

//-----------------------------------------------------------------------------

template<> bool
connection::is_source(node& target) const
{
    return &m_source.parent_node() == &target;
}

template<> bool
connection::is_source(pin& target) const
{
    return &m_source == &target;
}

template<> bool
connection::is_dest(node& target) const
{
    return &m_dest.parent_node() == &target;
}

template<> bool
connection::is_dest(pin& target) const
{
    return &m_dest == &target;
}

//=================================================================================================
// GRAPH
//=================================================================================================
std::vector<connection> graph::s_connections;
std::vector<node*> graph::s_nodes;
graph_properties graph::s_properties;

void graph::register_node(node& n)
{
    // check if node is already registered
    if ( std::find(s_nodes.begin(), s_nodes.end(), &n) == s_nodes.end())
         s_nodes.push_back(&n);
}

connection& graph::connect(pin& source, pin& dest, connection::pattern pattern)
{
    graph::register_node(source.m_parent);
    graph::register_node(dest.m_parent);

    qDebug() << "CONNECTING SOURCE:"
             << QString::fromStdString(source.parent_node().nreference())
             << "AND DEST:"
             << QString::fromStdString(dest.parent_node().nreference());

    // TODO, check for duplicates

    s_connections.emplace_back(source, dest, pattern);
    auto& con = s_connections.back();
    source.add_connection(con);
    dest.add_connection(con);

    return con;
}

connection& graph::connect(node& source, node& dest, connection::pattern pattern)
{
    return graph::connect(*source.outpin(), *dest.inpin(), pattern);
}

connection& graph::connect(node& source, pin& dest, connection::pattern pattern)
{
    return graph::connect(*source.outpin(), dest, pattern);
}

connection& graph::connect(pin& source, node& dest, connection::pattern pattern)
{
    return graph::connect(source, *dest.inpin(), pattern);
}

void graph::disconnect(pin& target)
{
    for ( const auto& connection : target.m_connections )
          std::remove(s_connections.begin(), s_connections.end(), *connection);

    target.m_connections.clear();
}

void graph::disconnect(pin& source, pin& dest)
{
    connection* target = nullptr;

    for ( auto& connection : source.m_connections )
          if ( connection->is_source(source) &&
               connection->is_dest(dest))
               target = connection.get();

    if ( !target ) return;

    source.remove_connection(*target);
    dest.remove_connection(*target);
    std::remove(s_connections.begin(), s_connections.end(), *target);
}

void graph::configure(signal_t rate, size_t vector_size, size_t feedback_size)
{
    s_properties = { rate, vector_size, feedback_size };
}

void graph::initialize()
{
    qDebug() << "GRAPH_ALLOCATION"
             << QString::number(s_nodes.size());

    for ( auto& node : s_nodes )
          node->initialize(s_properties);

    for ( auto& connection : s_connections )
          connection.allocate(s_properties.vsz);
}

mstream::slice graph::run(node& target)
{
    target.process();
    return target.collect(polarity_t::input);
}

//=================================================================================================
// OUTPUT
//=================================================================================================

Output::Output()
{

}

void Output::setRate(signal_t rate)
{
    m_rate = rate;
}

void Output::setNchannels(quint16 nchannels)
{
    m_nchannels = nchannels;
    m_outputs.set_nchannels(nchannels);
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
    graph::initialize();
    emit startStream();
}

void Output::componentComplete()
{
    WPN_REFACTOR
    node::componentComplete();

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
    graph::configure(m_rate, m_vector, m_feedback);
    m_audiothread.start(QThread::TimeCriticalPriority);
}

void Output::configure(graph_properties properties) {}
void Output::rwrite(node::pool& inputs, node::pool& outputs, size_t sz) {}

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
    m_format = WPN_RT_PRECISION;
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

WPN_TODO
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

    auto strm = graph::run(outmod);
    strm.interleaved(static_cast<signal_t*>(out));

    return 0;
}

//=================================================================================================
// SINETEST
//=================================================================================================

Sinetest::Sinetest()
{

}

void Sinetest::configure(graph_properties properties)
{
    m_wavetable.allocate(1, 16384);
    unsigned short i = 0;

    for ( auto& sample : m_wavetable()[0] )
          sample = sin(i++/16384*M_PI*2);
}

void Sinetest::rwrite(node::pool& upstream, node::pool& dnstream, size_t sz)
{
    auto& out    = dnstream["main"];
    auto& freq   = upstream["frequency"];
    auto wt      = m_wavetable();

    freq /= m_properties.rate;
    freq *= 16384;

    out.lookup(wt, freq, true);
}

//=================================================================================================
// VCA
//=================================================================================================

VCA::VCA()
{

}

void VCA::configure(graph_properties properties) {}

void VCA::rwrite(node::pool& upstream, node::pool& dnstream, size_t sz)
{
    auto in    = upstream["inputs"];
    auto gain  = upstream["gain"];
    auto out   = dnstream["outputs"];

    in *= gain;
    out << in;
}

//=================================================================================================
// PINKNOISE TESTING
//=================================================================================================

Pinktest::Pinktest()
{

}

void Pinktest::configure(graph_properties properties)
{

}

void Pinktest::rwrite(node::pool& inputs, node::pool& outputs, size_t sz)
{

}
