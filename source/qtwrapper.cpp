#include "qtwrapper.hpp"
#include <math.h>

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

WPN_TODO
void channel::slice::lookup(slice& source, const slice& head, bool increment)
{

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

WPN_TODO
stream::stream()
{

}

WPN_TODO
stream::stream(size_t nchannels)
{

}

WPN_TODO
stream::stream(size_t nchannels, size_t chsz, signal_t fill)
{

}

WPN_TODO
void stream::allocate(size_t nchannels, size_t size)
{

}

WPN_TODO
stream::~stream()
{

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

WPN_TODO
void stream::add_sync(stream& target, sync& s)
{

}

WPN_TODO
void stream::add_upsync(stream& target)
{

}

WPN_TODO
void stream::add_dnsync(stream& target)
{

}

WPN_TODO
stream::slice stream::draw(size_t sz)
{

}

WPN_TODO
void stream::pour(size_t sz)
{

}

size_t stream::size() const
{
    return m_size;
}

size_t stream::nchannels() const
{
    return m_nchannels;
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
    //?
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
        channel.lookup(sch, head[n], increment); // wtf?
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
// NODE::PIN
//=================================================================================================

node::pin::pin(node& parent, enum polarity p, std::string label, size_t nchannels, bool def) :
    m_parent(parent),
    m_polarity(p),
    m_label(label),
    m_nchannels(nchannels),
    m_default(def)
{
    parent.add_pin(*this);
}

node& node::pin::parent()
{
    return m_parent;
}

std::string node::pin::label() const
{
    return m_label;
}

bool node::pin::is_default() const
{
    return m_default;
}

polarity node::pin::polarity() const
{
    return m_polarity;
}

inline void node::pin::allocate(size_t sz)
{
    m_stream.allocate(m_nchannels, sz);
}

inline void node::pin::add_connection(connection& con)
{
    m_connections.push_back(&con);
}

inline void node::pin::remove_connection(connection& con)
{
    m_connections.removeOne(&con);
}

bool node::pin::connected() const
{
    return m_connections.size();
}

template<typename T>
bool node::pin::connected(T& target) const
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
// NODE
//=================================================================================================

inline void node::add_pin(node::pin &pin)
{
    switch(pin.polarity())
    {
    case polarity::input: m_uppins.push_back(&pin); break;
    case polarity::output: m_dnpins.push_back(&pin); break;
    }
}

WPN_TODO inline signal
node::sgrd(node::pin& p)
{


}

WPN_TODO inline void
node::sgwr(node::pin& p, signal v)
{
    if ( v.is_sgnt())
    {

    }
    else if ( v.is_qvariant())
    {

    }

    else if ( v.is_pin())
    {

    }
}

node::pool::pool(std::vector<node::pin*>& vector, size_t pos, size_t sz)
{
    for ( auto& pin : vector )
    {
        node::pstream ps = {
            pin->label(),
            pin->stream()(pos, sz, 0)
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

WPN_TODO
template<typename T> bool
node::connected(T& other) const
{
    return false;
}

//-------------------------------------------------------------------------------------------------

WPN_TODO
node::pin& node::inpin()
{
    assert( 0 );
}

WPN_TODO
node::pin& node::inpin(QString ref)
{
    assert( 0 );
}

WPN_TODO
node::pin& node::outpin()
{
    assert( 0 );
}

node::pin& node::outpin(QString ref)
{
    assert( 0 );
}

//-------------------------------------------------------------------------------------------------

WPN_TODO
void node::initialize(graph_properties properties)
{
    m_properties = properties;
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

connection::connection(node::pin& source, node::pin& dest, pattern pattern) :
    m_source(source), m_dest(dest), m_pattern(pattern) { }

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
    return &m_source.parent() == &target;
}

template<> bool
connection::is_source(node::pin& target) const
{
    return &m_source == &target;
}

template<> bool
connection::is_dest(node& target) const
{
    return &m_dest.parent() == &target;
}

template<> bool
connection::is_dest(node::pin& target) const
{
    return &m_dest == &target;
}

//=================================================================================================
// GRAPH
//=================================================================================================

connection& graph::connect(node::pin& source, node::pin& dest, connection::pattern pattern)
{
    connection con(source, dest, pattern);
    s_connections.push_back(con);

    return s_connections.last();
}

connection& graph::connect(node& source, node& dest, connection::pattern pattern)
{
    connection con(source.outpin(), dest.inpin(), pattern);
    s_connections.push_back(con);

    return s_connections.last();
}

connection& graph::connect(node& source, node::pin& dest, connection::pattern pattern)
{
    connection con(source.outpin(), dest, pattern);
    s_connections.push_back(con);

    return s_connections.last();
}

connection& graph::connect(node::pin& source, node& dest, connection::pattern pattern)
{
    connection con(source, dest.inpin(), pattern);
    s_connections.push_back(con);

    return s_connections.last();
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
// SIGNAL
//=================================================================================================


//=================================================================================================
// OUTPUT
//=================================================================================================

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
