#include "qtwrapper.hpp"
#include <math.h>
#include <QtDebug>

#define WPN_TODO
#define WPN_REFACTOR
#define WPN_OPTIMIZE
#define WPN_DEPRECATED
#define WPN_UNSAFE
#define WPN_EXAMINE
#define WPN_REVISE
#define WPN_OK

//=================================================================================================
// CHANNEL
//=================================================================================================

WPN_TODO
channel::channel()
{

}

WPN_TODO
channel::channel(size_t sz, signal_t fill)
{

}

channel::slice channel::operator()(size_t begin, size_t sz, size_t pos)
{
    return channel::slice(*this, &m_data[begin],
                          &m_data[begin+sz],
                          &m_data[begin+pos]);
}

//-------------------------------------------------------------------------------------------------
// channel-slice
//-------------------------------------------------------------------------------------------------

channel::slice::slice(channel& parent) : m_parent(parent) { }

channel::slice::iterator channel::slice::begin()
{
    return channel::slice::iterator(m_begin);
}

channel::slice::iterator channel::slice::end()
{
    return channel::slice::iterator(m_end);
}

signal_t& channel::slice::operator[](size_t index)
{
    return m_begin[index];
}

size_t channel::slice::size() const
{
    return m_size;
}

//-------------------------------------------------------------------------------------------------

// copy-merge contents of other into this
channel::slice& channel::slice::operator<<(channel::slice const& other)
{
    auto sz = std::min(m_size, other.m_size);
    for ( size_t n = 0; n < sz; ++n )
          m_begin[n] += other.m_begin[n];

    return *this;
}

// same, but reverted
channel::slice& channel::slice::operator>>(channel::slice& other)
{
    other << *this; return *this;
}

// merge (drain other afterwards)
channel::slice& channel::slice::operator<<=(channel::slice& other)
{
    *this << other; other.drain(); return *this;
}

channel::slice& channel::slice::operator>>=(channel::slice& other)
{
    other <<= *this; return *this;
}

//-------------------------------------------------------------------------------------------------

channel::slice& channel::slice::operator<<(const signal_t v)
{
    *m_pos++ = v; return *this;
}

channel::slice& channel::slice::operator>>(signal_t& v)
{
    v = *m_pos++; return *this;
}

//-------------------------------------------------------------------------------------------------

channel::slice& channel::slice::operator*=(const signal_t v)
{
    for ( auto& sample : *this )
          sample *= v;
    return *this;
}

channel::slice& channel::slice::operator/=(const signal_t v)
{
    for ( auto& sample : *this )
          sample /= v;
    return *this;
}

channel::slice& channel::slice::operator+=(const signal_t v)
{
    for ( auto& sample : *this )
          sample += v;
    return *this;
}

channel::slice& channel::slice::operator-=(const signal_t v)
{
    for ( auto& sample : *this )
          sample -= v;
    return *this;
}

//-------------------------------------------------------------------------------------------------
#define FOREACH_SAMPLE_OPERATOR(_o)                                         \
    auto sz = std::min(m_size, other.m_size);                               \
    for ( size_t n = 0; n < sz; ++n )                                       \
          m_begin[n] _o other.m_begin[n];                                   \
    return *this;
//-------------------------------------------------------------------------------------------------

channel::slice& channel::slice::operator*=(channel::slice const& other)
{
    FOREACH_SAMPLE_OPERATOR(*=);
}

channel::slice& channel::slice::operator/=(channel::slice const& other)
{
    FOREACH_SAMPLE_OPERATOR(/=);
}

channel::slice& channel::slice::operator+=(channel::slice const& other)
{
    FOREACH_SAMPLE_OPERATOR(+=);
}

channel::slice& channel::slice::operator-=(channel::slice const& other)
{
    FOREACH_SAMPLE_OPERATOR(-=);
}

//-------------------------------------------------------------------------------------------------
// CHANNEL/ANALYSIS
//-------------------------------------------------------------------------------------------------

signal_t channel::slice::min()
{
    signal_t m = 0;
    for   (  auto& sample : *this )
             m = std::min(m, sample);
    return   m;
}

signal_t channel::slice::max()
{
    signal_t m = 0;
    for   (  auto& sample : *this )
             m = std::max(m, sample);
    return   m;
}

signal_t channel::slice::rms()
{
    signal_t m = 0;
    for ( auto s : *this )
          m += s;

    return std::sqrt(1.0/m_size/m);
}

//-------------------------------------------------------------------------------------------------
// CHANNEL/GLOBAL OPERATIONS
//-------------------------------------------------------------------------------------------------

void channel::slice::drain()
{
    memset(m_begin, 0, sizeof(signal_t)*m_size);
}

WPN_TODO void
channel::slice::normalize()
{

}

//-------------------------------------------------------------------------------------------------
// CHANNEL/MISCELLANEOUS
//-------------------------------------------------------------------------------------------------

WPN_EXAMINE
void channel::slice::lookup(slice& source, slice& head, bool increment)
{
    assert(m_size == source.size() == head.size());

    if ( increment )
    {
        for (size_t n = 0; n < m_size; ++n)
        {
            signal_t idx = head[n];
            size_t   y = floor(idx);
            signal_t x = static_cast<signal_t>(idx-y);
            signal_t e = lininterp(x, source[y], source[y+1]);

            m_begin[n] = e;
        }
    }
}

//-------------------------------------------------------------------------------------------------
// CHANNEL/ITERATORS
//-------------------------------------------------------------------------------------------------
channel::slice::iterator::iterator(signal_t *data) : m_data(data) { }

channel::slice::iterator& channel::slice::iterator::operator++()
{
    m_data++; return *this;
}

signal_t& channel::slice::iterator::operator*()
{
    return *m_data;
}

bool channel::slice::iterator::operator==(channel::slice::iterator const& other)
{
    return m_data == other.m_data;
}

bool channel::slice::iterator::operator!=(channel::slice::iterator const& other)
{
    return m_data != other.m_data;
}


//=================================================================================================
// STREAM
//=================================================================================================

stream::stream() : m_size(0), m_nchannels(0)
{

}

WPN_REVISE
stream::stream(size_t nchannels, size_t chsz, signal_t fill) :
    m_size(chsz), m_nchannels(nchannels)
{
    allocate(nchannels, chsz);
    operator()() <<= fill;
}

WPN_REVISE
void stream::append(stream& other)
{
    for ( auto& ch : other.m_channels )
          m_channels.push_back(ch);
}

WPN_REVISE
void stream::append(channel& ch)
{
    m_channels.push_back(&ch);
}

WPN_REVISE
void stream::allocate(size_t nchannels, size_t size)
{
    for ( auto& ch : m_channels )
          ch = new channel(size);
}
stream::~stream()
{
    for ( auto& ch : m_channels )
          delete ch;
}

size_t stream::size() const
{
    return m_size;
}

size_t stream::nchannels() const
{
    return m_nchannels;
}

channel& stream::operator[](size_t index)
{
    return *m_channels[index];
}

stream::operator slice()
{
    return stream::slice(*this, 0, m_size, 0);
}

stream::slice stream::operator()(size_t begin, size_t sz, size_t pos)
{
    return stream::slice(*this, begin, sz, pos);
}

//-------------------------------------------------------------------------------------------------
// STREAM SYNCS
//-------------------------------------------------------------------------------------------------

WPN_REVISE
void stream::add_upsync(stream& target)
{
    m_upsync._stream = &target;
    m_upsync.size = target.size();
    m_upsync.pos = 0;
}

WPN_REVISE
void stream::add_dnsync(stream& target)
{
    m_dnsync._stream = &target;
    m_dnsync.size = target.size();
    m_dnsync.pos = 0;
}

WPN_REVISE
stream::slice stream::draw(size_t sz)
{
    auto acc = operator()(m_upsync.pos, sz);
    auto& up = *m_upsync._stream;
    acc << up(m_upsync.pos, sz);

    m_upsync.pos += sz;
    if ( m_upsync.pos >= m_upsync.size )
         m_upsync.pos -= m_upsync.size;

    return acc;
}

WPN_REVISE
void stream::pour(size_t sz)
{
    auto acc = operator()(m_dnsync.pos, sz);
    auto& dn = *m_dnsync._stream;
    acc << dn(m_dnsync.pos, sz);

    m_dnsync.pos += sz;
    if ( m_dnsync.pos >= m_dnsync.size )
         m_dnsync.pos -= m_dnsync.size;
}

WPN_REVISE
void stream::draw_skip(size_t sz)
{
    m_upsync.pos += sz;
    if ( m_upsync.pos >= m_upsync.size )
         m_upsync.pos -= m_upsync.size;
}

WPN_REVISE
void stream::pour_skip(size_t sz)
{
    m_dnsync.pos += sz;
    if ( m_dnsync.pos >= m_dnsync.size )
         m_dnsync.pos -= m_dnsync.size;
}

//-------------------------------------------------------------------------------------------------
// STREAM SLICE
//-------------------------------------------------------------------------------------------------

stream::slice::iterator stream::slice::begin()
{
    return stream::slice::iterator(m_cslices.begin());
}

stream::slice::iterator stream::slice::end()
{
    return stream::slice::iterator(m_cslices.end());
}

channel::slice stream::slice::operator[](size_t index)
{
    return m_cslices[index];
}

stream::slice::operator bool()
{
    // not quite sure what to do of this...
    return false;
}

size_t stream::slice::size() const
{
    return m_size;
}

size_t stream::slice::nchannels() const
{
    return m_cslices.size();
}

//-------------------------------------------------------------------------------------------------

stream::slice& stream::slice::operator+=(signal_t v)
{
    for ( auto& channel : m_cslices )
          channel += v;
    return *this;
}

stream::slice& stream::slice::operator-=(signal_t v)
{
    for ( auto& channel : m_cslices )
          channel -= v;
    return *this;
}

stream::slice& stream::slice::operator*=(signal_t v)
{
    for ( auto& channel : m_cslices )
          channel *= v;
    return *this;
}

stream::slice& stream::slice::operator/=(signal_t v)
{
    for ( auto& channel : m_cslices )
          channel /= v;
    return *this;
}

//-------------------------------------------------------------------------------------------------
#define FOREACH_CHANNEL_OPERATOR(_op) \
    auto sz = std::min(m_parent.m_nchannels, other.m_parent.m_nchannels); \
    for ( size_t n = 0; n < sz; ++n ) \
          m_cslices[n] _op other.m_cslices[n]; \
    return *this;
//-------------------------------------------------------------------------------------------------

stream::slice& stream::slice::operator+=(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(+=);
}

stream::slice& stream::slice::operator-=(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(-=);
}

stream::slice& stream::slice::operator*=(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(*=);
}

stream::slice& stream::slice::operator/=(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(/=);
}

//-------------------------------------------------------------------------------------------------

stream::slice& stream::slice::operator+=(channel::slice const& other)
{
    for ( auto& channel : m_cslices)
          channel += other;
    return *this;
}

stream::slice& stream::slice::operator-=(channel::slice const& other)
{
    for ( auto& channel : m_cslices)
          channel -= other;
    return *this;
}

stream::slice& stream::slice::operator*=(channel::slice const& other)
{
    for ( auto& channel : m_cslices)
          channel *= other;
    return *this;
}

stream::slice& stream::slice::operator/=(channel::slice const& other)
{
    for ( auto& channel : m_cslices)
          channel /= other;
    return *this;

}

stream::slice& stream::slice::operator<<=(signal_t const v)
{
    for ( auto& channel: m_cslices )
          channel <<= v;
    return *this;
}

//-------------------------------------------------------------------------------------------------

stream::slice& stream::slice::operator<<(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(<<)
}

stream::slice& stream::slice::operator>>(stream::slice& other)
{
    other << *this; return *this;
}

stream::slice& stream::slice::operator<<=(stream::slice& other)
{
    FOREACH_CHANNEL_OPERATOR(<<=)
}

stream::slice& stream::slice::operator>>=(stream::slice& other)
{
    other <<= *this; return *this;
}

//-------------------------------------------------------------------------------------------------
void stream::slice::drain()
{
    for ( auto& cslice : m_cslices )
          cslice.drain();
}

WPN_TODO
void stream::slice::normalize()
{

}

void stream::slice::lookup(stream::slice& source,
                           stream::slice& head, bool increment)
{
    assert(source.size() == head.size() == m_size);
    size_t n = 0;
    for ( auto& channel : m_cslices )
    {
        auto sch = source[n];
        auto hch = head[n];
        channel.lookup(sch, hch, increment);
        ++n;
    }
}

//-------------------------------------------------------------------------------------------------

stream::slice::iterator::iterator(std::vector<channel::slice>::iterator data) :
    m_data(data) {}

stream::slice::iterator&
stream::slice::iterator::operator++()
{
    m_data++; return *this;
}

channel::slice
stream::slice::iterator::operator*()
{
    return *m_data;
}

bool stream::slice::iterator::operator==(stream::slice::iterator const& s)
{
    return m_data == s.m_data;
}

bool stream::slice::iterator::operator!=(stream::slice::iterator const& s)
{
    return m_data != s.m_data;
}


//=================================================================================================
// pin
//=================================================================================================

pin::pin(node& parent, enum polarity p, std::string label, size_t nchannels, bool def) :
    m_parent(parent),
    m_polarity(p),
    m_label(label),
    m_nchannels(nchannels),
    m_default(def)
{
    parent.add_pin(*this);
}

node& pin::get_parent()
{
    return m_parent;
}

std::string pin::get_label() const
{
    return m_label;
}

bool pin::is_default() const
{
    return m_default;
}

polarity pin::get_polarity() const
{
    return m_polarity;
}

inline void pin::allocate(size_t sz)
{
    m_stream.allocate(m_nchannels, sz);
}

inline void pin::add_connection(connection& con)
{
    m_connections.push_back(&con);
}

inline void pin::remove_connection(connection& con)
{
    std::remove(m_connections.begin(),
                m_connections.end(), &con);
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
    case polarity::input:
    {
        for ( const auto& connection : m_connections )
            if ( connection->is_dest(target))
                 return true;
        break;
    }
    case polarity::output:
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
// QT-SIGNAL
//=================================================================================================

signal::signal(qreal v) : ureal(v), m_qtype(REAL) {}
signal::signal(QVariant v) : uvar(v), m_qtype(VAR) {}
signal::signal(pin& p) : upin(&p), m_qtype(PIN) {}

WPN_TODO
signal::signal(signal const& cp)
{

}

WPN_TODO
signal::signal(signal&& mv)
{

}

WPN_TODO
signal& signal::operator=(signal const& cp)
{

}

WPN_TODO
signal& signal::operator=(signal&& mv)
{

}

bool signal::is_pin() const
{
    return m_qtype == PIN;
}

bool signal::is_real() const
{
    return m_qtype == REAL;
}

bool signal::is_qvariant() const
{
    return m_qtype == VAR;
}

pin& signal::to_pin()
{
    return *upin;
}

QVariant signal::to_qvariant() const
{
    return uvar;
}

qreal signal::to_real() const
{
    return ureal;
}

//=================================================================================================
// NODE
//=================================================================================================

inline void node::add_pin(pin &pin)
{
    switch(pin.get_polarity())
    {
    case polarity::input: m_uppins.push_back(&pin); break;
    case polarity::output: m_dnpins.push_back(&pin); break;
    }
}

inline signal node::sgrd(pin& p)
{
    return signal(p);
}

WPN_TODO inline void
node::sgwr(pin& p, signal v)
{
    if ( v.is_real())
         p.get_stream()() <<= v.to_real();

    else if ( v.is_pin())
    {
        // disconnect all
        graph::disconnect(p);

        // connect to pin
        graph::connect(p, v.to_pin(), connection::pattern::Merge);
    }

    else if ( v.is_qvariant())
    {
        QVariant var = v.to_qvariant();
        // parse it
        WPN_TODO
    }
}

node::pool::pool(std::vector<pin*>& vector, size_t pos, size_t sz)
{
    for ( auto& pin : vector )
    {
        node::pstream ps = {
            pin->get_label(),
            pin->get_stream()(pos, sz, 0)
        };

        streams.push_back(ps);
    }
}

stream::slice& node::pool::operator[](std::string str)
{
    for ( auto& stream : streams )
          if ( stream.label == str )
               return stream.slice;
    assert(0);
}

//-------------------------------------------------------------------------------------------------

template<typename T> bool
node::connected(T& other) const
{
    if ( other.polarity() == polarity::input )
         for ( auto& pin: m_dnpins )
               if ( pin->connected(other) )
                    return true;

    if ( other.polarity() == polarity::output )
         for ( auto& pin: m_uppins )
               if ( pin->connected(other) )
                    return true;

    return false;
}

//-------------------------------------------------------------------------------------------------

pin& node::inpin()
{
    for ( auto& pin : m_uppins )
          if ( pin->is_default() )
               return *pin;
    assert( 0 );
}

pin& node::inpin(QString ref)
{
    for ( auto& pin : m_uppins )
          if (pin->get_label() == ref.toStdString())
              return *pin;
    assert( 0 );
}

pin& node::outpin()
{
    for ( auto& pin : m_dnpins )
          if ( pin->is_default() )
               return *pin;
    assert( 0 );
}

pin& node::outpin(QString ref)
{
    for ( auto& pin : m_dnpins )
          if (pin->get_label() == ref.toStdString())
              return *pin;
    assert( 0 );
}

//-------------------------------------------------------------------------------------------------

WPN_REVISE
stream::slice node::upstream()
{
    // gather all streams as one
    // return it as a slice
    // (without memory allocation)
    stream s;
    for ( auto& pin : m_uppins )
         s.append(pin->get_stream());

    return s();
}

WPN_REVISE
stream::slice node::dnstream()
{
    stream s;
    for ( auto& pin : m_dnpins )
         s.append(pin->get_stream());

    return s();

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

    if ( m_stream_pos >= m_properties.vsz )
         m_stream_pos -= m_properties.vsz;

}

//=================================================================================================
// CONNECTION
//=================================================================================================

void connection::pull(size_t sz)
{
    if ( !m_active )
         return;

    if ( m_feedback )
         while ( m_source.m_parent.m_stream_pos < sz )
                 m_source.m_parent.process();

    if ( m_muted )
    {
        auto slice = m_stream.draw(sz);
        slice *= m_level;
        m_stream.pour(sz);
    }
    else
    {
        m_stream.draw_skip(sz);
        m_stream.pour_skip(sz);
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

WPN_REVISE
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
    return &m_source.get_parent() == &target;
}

template<> bool
connection::is_source(pin& target) const
{
    return &m_source == &target;
}

template<> bool
connection::is_dest(node& target) const
{
    return &m_dest.get_parent() == &target;
}

template<> bool
connection::is_dest(pin& target) const
{
    return &m_dest == &target;
}

//=================================================================================================
// GRAPH
//=================================================================================================

connection& graph::connect(pin& source, pin& dest, connection::pattern pattern)
{
    s_connections.emplace_back(source, dest, pattern);
    return s_connections.back();
}

connection& graph::connect(node& source, node& dest, connection::pattern pattern)
{
    s_connections.emplace_back(source.outpin(), dest.inpin(), pattern);
    return s_connections.back();
}

connection& graph::connect(node& source, pin& dest, connection::pattern pattern)
{
    s_connections.emplace_back(source.outpin(), dest, pattern);
    return s_connections.back();
}

connection& graph::connect(pin& source, node& dest, connection::pattern pattern)
{
    s_connections.emplace_back(source, dest.inpin(), pattern);
    return s_connections.back();
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
               target = connection;

    if ( !target ) return;

    source.remove_connection(*target);
    dest.remove_connection(*target);
    std::remove(s_connections.begin(), s_connections.end(), *target);
}

void graph::configure(signal_t rate, size_t vector_size, size_t feedback_size)
{
    s_properties = { rate, vector_size, feedback_size };
}

stream::slice graph::run(node& target)
{
    target.process();
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

void Output::componentComplete()
{
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
            info = audio.getDeviceInfo(d);
            auto name = QString::fromStdString(info.name);

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

    qDebug() << "AUDIO_SELECTED_DEVICE"
             << QString::fromStdString(selected_info.name);

    parameters.nChannels    = m_nchannels;
    options.streamName      = "WPN114";
    options.flags           = RTAUDIO_SCHEDULE_REALTIME;
    options.priority        = 10;

    m_stream = std::make_unique<Audiostream>
               (*this, parameters, info, options);

    m_stream->moveToThread(&m_audiothread);

    QObject::connect(this, &Output::preconfigure, &*m_stream, &Audiostream::preconfigure);
    QObject::connect(this, &Output::start, &*m_stream, &Audiostream::start);
    QObject::connect(this, &Output::stop, &*m_stream, &Audiostream::stop);
    QObject::connect(this, &Output::restart, &*m_stream, &Audiostream::restart);
    QObject::connect(this, &Output::exit, &*m_stream, &Audiostream::exit);

    emit preconfigure();
    m_audiothread.start(QThread::TimeCriticalPriority);
}

void Output::configure(graph_properties properties)
{

}

void Output::rwrite(node::pool& inputs, node::pool& outputs, size_t sz)
{

}

//=================================================================================================
// AUDIOSTREAM
//=================================================================================================

Audiostream::Audiostream(Output& out,
    RtAudio::StreamParameters parameters,
    RtAudio::DeviceInfo info,
    RtAudio::StreamOptions options) :
    m_outmodule(out),
    m_device_info(info),
    m_options(options),
    m_stream(std::make_unique<RtAudio>())
{

}

void Audiostream::preconfigure()
{
    m_format = RTAUDIO_FLOAT64;
    m_vector = m_outmodule.vector();

    try
    {
        m_stream->openStream(
            &m_parameters,
            nullptr,
            m_format,
            m_outmodule.rate(),
            &m_vector,
            &rwrite,
            static_cast<void*>(&m_outmodule),
            &m_options,
            nullptr
        );
    }

    catch(RtAudioError const& e)
    {
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
