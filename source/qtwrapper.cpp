#include "qtwrapper.hpp"
#include <math.h>
#include <QtDebug>

//=================================================================================================
// ALLOCATORS
//=================================================================================================

template<typename T, size_t Sz>
stack_allocator<T, Sz>::stack_allocator() {}

template<typename T, size_t Sz>
T* stack_allocator<T, Sz>::begin()
{
    return &m_data;
}

template<typename T, size_t Sz>
T* stack_allocator<T, Sz>::end()
{
    return m_data[Sz-1];
}

template<typename T, size_t Sz>
T* stack_allocator<T, Sz>::operator[](size_t index)
{
    return &m_data[index];
}

template<typename T>
mallocator<T>::mallocator() {}

template<typename T>
T* mallocator<T>::allocate(size_t sz)
{
    m_size = sz;
    return m_data = static_cast<T*>(
           malloc(sizeof(T)*sz)); // for now..
}

template<typename T>
T* mallocator<T>::begin()
{
    return m_data;
}

template<typename T>
T* mallocator<T>::end()
{
    return &m_data[m_size-1];
}

template<typename T>
T* mallocator<T>::operator[](size_t index)
{
    return &m_data[index];
}

template<typename T>
void mallocator<T>::release()
{
    free(m_data);
}

template<typename T>
null_allocator<T>::null_allocator() {}

template<typename T>
T* null_allocator<T>::begin()
{
    return nullptr;
}

template<typename T>
T* null_allocator<T>::end()
{
    return nullptr;
}

//=================================================================================================
// CHANNEL
//=================================================================================================

template<typename T, typename Allocator>
channel<T, Allocator>::channel() {}

template<typename T, typename Allocator>
channel<T, Allocator>::channel(size_t sz, T fill)
{
    m_data = m_allocator.allocate(sizeof(T)*sz);
    m_size = sz;
    *static_cast<slice*>(this) <<= fill;
}

template<typename T, typename Allocator>
channel<T, Allocator>::channel(T* begin, size_t sz, T fill)
{
    m_data = begin;
    m_size = sz;
    *static_cast<slice*>(this) <<= fill;
}

template<typename T, typename Allocator>
channel<T, Allocator>::operator slice()
{
    return operator()();
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice
channel<T, Allocator>::operator()(size_t begin, size_t sz, size_t pos)
{
    return slice(*this, m_data+begin, m_data+begin+sz,
                        m_data+begin+pos);
}

//-------------------------------------------------------------------------------------------------
// channel-slice
//-------------------------------------------------------------------------------------------------
template<typename T, typename Allocator>
channel<T, Allocator>::slice::slice(channel& parent) :
    m_parent(parent)
{
    m_begin  = m_pos = parent.m_data;
    m_size   = parent.m_size;
    m_end    = &parent.m_data[m_size];
}

template<typename T, typename Allocator>
channel<T, Allocator>::slice::slice(channel& parent, T* begin, T* end, T* pos) :
    m_parent(parent), m_size(parent.m_size),
    m_begin(begin), m_end(end), m_pos(pos)
{

}

// copy constructor
template<typename T, typename Allocator>
channel<T, Allocator>::slice::slice(slice const& cp) :
    m_parent(cp.m_parent),
    m_begin(cp.m_begin),
    m_end(cp.m_end),
    m_pos(cp.m_pos),
    m_size(cp.m_size) { }

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice::iterator
channel<T, Allocator>::slice::begin()
{
    return channel::slice::iterator(m_begin);
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice::iterator
channel<T, Allocator>::slice::end()
{
    return channel::slice::iterator(m_end);
}

template<typename T, typename Allocator>
channel<T, Allocator>::slice::operator T*()
{
    // cast slice to T pointer
    return m_begin;
}

template<typename T, typename Allocator>
T& channel<T, Allocator>::slice::operator[](size_t index)
{
    return m_begin[index];
}

template<typename T, typename Allocator>
size_t channel<T, Allocator>::slice::size() const
{
    return m_size;
}

//-------------------------------------------------------------------------------------------------
// CHANNEl: stream operators
//-------------------------------------------------------------------------------------------------
template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator<<(channel::slice const& other)
{
    // copy-merge contents of other into this
    // equivalent to operator +=
    return operator+=(other);
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator>>(channel::slice& other)
{
    // same, but reverted
    other << *this;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator<<=(channel::slice& other)
{
    // move-merge (copy and drain other afterwards)
    *this << other;
    other.drain();
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator>>=(channel::slice& other)
{
    // same, but reverted
    other <<= *this;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator<<(const T v)
{
    if ( m_pos == m_end )
         return *this;
    else *m_pos++ = v;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator>>(T& v)
{
    v = *m_pos++;
    return *this;
}

//-------------------------------------------------------------------------------------------------
// CHANNEl: arithmetics
//-------------------------------------------------------------------------------------------------
#define FOREACH_SAMPLE_OPERATOR(_o)                                                                \
    auto sz = std::min(m_size, other.m_size);                                                      \
    for ( size_t n = 0; n < sz; ++n )                                                              \
          m_begin[n] _o other.m_begin[n];                                                          \
    return *this;
//-------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator*=(const T v)
{
    for ( auto& sample : *this )
          sample *= v;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator/=(const T v)
{
    for ( auto& sample : *this )
          sample /= v;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator+=(const T v)
{
    for ( auto& sample : *this )
          sample += v;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator-=(const T v)
{
    for ( auto& sample : *this )
          sample -= v;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator<<=(const T v)
{
    for ( auto& sample : *this )
          sample = v;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator*=(channel::slice const& other)
{
    FOREACH_SAMPLE_OPERATOR(*=);
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator/=(channel::slice const& other)
{
    FOREACH_SAMPLE_OPERATOR(/=);
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator+=(channel::slice const& other)
{
    FOREACH_SAMPLE_OPERATOR(+=);
}

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice&
channel<T, Allocator>::slice::operator-=(channel::slice const& other)
{
    FOREACH_SAMPLE_OPERATOR(-=);
}

//-------------------------------------------------------------------------------------------------
// CHANNEL: analysis
//-------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
T channel<T, Allocator>::slice::min()
{
    T m = 0;
    for  ( auto& sample : *this )
           m = std::min(m, sample);
    return m;
}

template<typename T, typename Allocator>
T channel<T, Allocator>::slice::max()
{
    T m = 0;
    for  ( auto& sample : *this )
           m = std::max(m, sample);
    return m;
}

template<typename T, typename Allocator>
T channel<T, Allocator>::slice::rms()
{
    T m = 0;
    for ( auto s : *this )
          m += s;
    return std::sqrt( 1.0/m_size/m );
}

//-------------------------------------------------------------------------------------------------
// CHANNEL: global operations
//-------------------------------------------------------------------------------------------------
template<typename T, typename Allocator>
void channel<T, Allocator>::slice::drain()
{
    memset(m_begin, 0, sizeof(T)*m_size);
}

WPN_TODO
template<typename T, typename Allocator>
void channel<T, Allocator>::slice::normalize()
{

}

template<typename T, typename Allocator> void
channel<T, Allocator>::slice::lookup(slice& source, slice& head, bool increment)
{
    assert(m_size == source.size() == head.size());

    if ( increment )
    {
        for (size_t n = 0; n < m_size; ++n)
        {
            T idx = head[n];
            size_t y = floor(idx);
            T x = static_cast<T>(idx-y);
            T e = lininterp(x, source[y], source[y+1]);

            m_begin[n] = e;
        }
    }
}

//-------------------------------------------------------------------------------------------------
// CHANNEL: slice iterators
//-------------------------------------------------------------------------------------------------
template<typename T, typename Allocator>
channel<T, Allocator>::slice::iterator::iterator(T* data) :
    m_data(data) { }

template<typename T, typename Allocator>
typename channel<T, Allocator>::slice::iterator&
channel<T, Allocator>::slice::iterator::operator++()
{
    m_data++;
    return *this;
}

template<typename T, typename Allocator>
T& channel<T, Allocator>::slice::iterator::operator*()
{
    return *m_data;
}

template<typename T, typename Allocator>
bool channel<T, Allocator>::slice::iterator::operator==(slice::iterator const& other)
{
    return m_data == other.m_data;
}

template<typename T, typename Allocator>
bool channel<T, Allocator>::slice::iterator::operator!=(slice::iterator const& other)
{
    return m_data != other.m_data;
}

//=================================================================================================
// STREAM
//=================================================================================================

template<typename T, typename Allocator>
stream<T, Allocator>::stream()
{
    // default constructor, without pre-allocation
}

template<typename T, typename Allocator>
stream<T, Allocator>::stream(size_t nchannels, size_t nsamples, T fill) :
    m_nsamples(nsamples), m_nchannels(nchannels), m_nframes(nsamples*nchannels)
{
    // full constructor, direct allocation
    m_data = m_allocator.allocate(sizeof(T)*nchannels*nsamples);
    static_cast<slice*>(this) <<= fill;

    // we build channels
    for ( size_t nch = 0; nch < nchannels; ++nch)
          m_channels.emplace_back(
                     m_data[nch*nsamples],
                     nsamples, 0);
}

template<typename T, typename Allocator>
void stream<T, Allocator>::append(stream& other)
{
    // reallocate? append pointer?
    WPN_TODO
}

template<typename T, typename Allocator>
void stream<T, Allocator>::append(channel<T, Allocator>& ch)
{
    WPN_TODO
}

template<typename T, typename Allocator>
void stream<T, Allocator>::allocate(size_t nchannels, size_t nsamples)
{
    // for post-allocation
    m_data = m_allocator.allocate(sizeof(T)*nchannels*nsamples);
    m_nchannels = nchannels;
    m_nsamples  = nsamples;
    m_nframes   = nchannels*nsamples;

    for ( size_t nch = 0; nch < nchannels; ++nch)
          m_channels.emplace_back(
                     m_data[nch*nsamples],
                     nsamples, 0);
}

template<typename T, typename Allocator>
stream<T, Allocator>::~stream()
{
    m_allocator.release();
}

template<typename T, typename Allocator>
size_t stream<T, Allocator>::nsamples() const
{
    return m_nsamples;
}

template<typename T, typename Allocator>
size_t stream<T, Allocator>::nchannels() const
{
    return m_nchannels;
}

template<typename T, typename Allocator>
size_t stream<T, Allocator>::nframes() const
{
    return m_nframes;
}

WPN_EXAMINE
template<typename T, typename Allocator>
stream<T, Allocator>::operator[](size_t index)
{

}

template<typename T, typename Allocator>
stream<T, Allocator>::operator slice()
{
    return stream::slice(*this, 0, m_nsamples, 0);
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice
stream<T, Allocator>::operator()(size_t begin, size_t sz, size_t pos)
{
    return stream::slice(*this, begin, sz, pos);
}

//-------------------------------------------------------------------------------------------------
// STREAM SYNCS
//-------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
inline void stream<T, Allocator>::add_sync(sync& target, stream& s)
{
    target._stream = &s;
    target.size = s.size();
    target.pos = 0;
}

template<typename T, typename Allocator>
inline typename stream<T, Allocator>::slice
stream<T, Allocator>::synchronize(sync& target, size_t sz)
{
    auto acc    = operator()(target.pos, sz);
    auto& strm  = *target._stream;

    acc << strm(target.pos, sz);
    synchronize_skip(target, sz);

    return acc;
}
template<typename T, typename Allocator>
inline void
stream<T, Allocator>::synchronize_skip(sync& target, size_t sz)
{
    target.pos += sz;
    wrap(target.pos, target.size);
}

template<typename T, typename Allocator>
void stream<T, Allocator>::add_upsync(stream& target)
{
    add_sync(m_upsync, target);
}

template<typename T, typename Allocator>
void stream<T, Allocator>::add_dnsync(stream& target)
{
    add_sync(m_dnsync, target);
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice
stream<T, Allocator>::sync_draw(size_t sz)
{
    return synchronize(m_upsync, sz);
}

template<typename T, typename Allocator>
void stream<T, Allocator>::sync_pour(size_t sz)
{
    synchronize(m_dnsync, sz);
}

template<typename T, typename Allocator>
void stream<T, Allocator>::upsync_skip(size_t sz)
{
    synchronize_skip(m_upsync, sz);
}

template<typename T, typename Allocator>
void stream<T, Allocator>::dnsync_skip(size_t sz)
{
    synchronize_skip(m_dnsync, sz);
}

//-------------------------------------------------------------------------------------------------
// STREAM SLICE
//-------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
stream<T, Allocator>::slice::slice(
    stream& parent, size_t begin, size_t size, size_t pos) :
    m_parent(parent), m_begin(begin), m_size(size),
    m_pos(pos)
{

}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice::iterator
stream<T, Allocator>::slice::begin()
{
    return stream::slice::iterator(m_cslices.begin());
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice::iterator
stream<T, Allocator>::slice::end()
{
    return stream::slice::iterator(m_cslices.end());
}

template<typename T, typename Allocator>
typename channel<T, null_allocator<T>>::slice
stream<T, Allocator>::slice::operator[](size_t index)
{
    return m_cslices[index];
}

template<typename T, typename Allocator>
stream<T, Allocator>::slice::operator bool()
{
    // not quite sure what to do of this...
    return false;
}

template<typename T, typename Allocator>
size_t stream<T, Allocator>::slice::size() const
{
    return m_size;
}

template<typename T, typename Allocator>
size_t stream<T, Allocator>::slice::nchannels() const
{
    return m_cslices.size();
}

template<typename T, typename Allocator>
stream<T, Allocator>::slice::operator T*()
{
    return interleaved();
}

WPN_TODO
template<typename T, typename Allocator>
T* stream<T, Allocator>::slice::interleaved()
{
    return nullptr;
}

template<typename T, typename Allocator>
void stream<T, Allocator>::slice::interleaved(T* dest)
{
    // this being a column major function
    // we will have to optimize the memory allocation
    for ( size_t n = 0; n < m_size; ++n )
        for ( auto& channel : m_cslices )
              *dest++ = channel[n];
}

//-------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator+=(T v)
{
    for ( auto& channel : m_cslices )
          channel += v;
    return *this;
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator-=(T v)
{
    for ( auto& channel : m_cslices )
          channel -= v;
    return *this;
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator*=(T v)
{
    for ( auto& channel : m_cslices )
          channel *= v;
    return *this;
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator/=(T v)
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

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator+=(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(+=);
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator-=(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(-=);
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator*=(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(*=);
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator/=(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(/=);
}

//-------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator+=(typename schannel::slice const& other)
{
    for ( auto& channel : m_cslices)
          channel += other;
    return *this;
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator-=(typename schannel::slice const& other)
{
    for ( auto& channel : m_cslices)
          channel -= other;
    return *this;
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator*=(typename schannel::slice const& other)
{
    for ( auto& channel : m_cslices)
          channel *= other;
    return *this;
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator/=(typename schannel::slice const& other)
{
    for ( auto& channel : m_cslices)
          channel /= other;
    return *this;

}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator<<=(T const v)
{
    for ( auto& channel: m_cslices )
          channel <<= v;
    return *this;
}

//-------------------------------------------------------------------------------------------------

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator<<(stream::slice const& other)
{
    FOREACH_CHANNEL_OPERATOR(<<)
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator>>(stream::slice& other)
{
    other << *this; return *this;
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator<<=(stream::slice& other)
{
    FOREACH_CHANNEL_OPERATOR(<<=)
}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice&
stream<T, Allocator>::slice::operator>>=(stream::slice& other)
{
    other <<= *this; return *this;
}

//-------------------------------------------------------------------------------------------------
template<typename T, typename Allocator>
void stream<T, Allocator>::slice::drain()
{
    for ( auto& cslice : m_cslices )
          cslice.drain();
}

WPN_TODO
template<typename T, typename Allocator>
void stream<T, Allocator>::slice::normalize()
{

}

template<typename T, typename Allocator>
void stream<T, Allocator>::slice::lookup(stream::slice& source,
                                         stream::slice& head,
                                         bool increment)
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

template<typename T, typename Allocator>
stream<T, Allocator>::slice::iterator::iterator(std::vector<schannel::slice>::iterator data) :
    m_data(data) {}

template<typename T, typename Allocator>
typename stream<T, Allocator>::slice::iterator&
stream<T, Allocator>::slice::iterator::operator++()
{
    m_data++;
    return *this;
}

template<typename T, typename Allocator>
typename channel<T, null_allocator<T>>::slice
stream<T, Allocator>::slice::iterator::operator*()
{
    return *m_data;
}

template<typename T, typename Allocator>
bool stream<T, Allocator>::slice::iterator::operator==(stream::slice::iterator const& s)
{
    return m_data == s.m_data;
}

template<typename T, typename Allocator>
bool stream<T, Allocator>::slice::iterator::operator!=(stream::slice::iterator const& s)
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

std::vector<std::shared_ptr<connection>> pin::connections()
{
    return m_connections;
}

stream<signal_t, mallocator<signal_t>>&
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

inline std::vector<pin*>&
node::pvector(polarity p)
{
    switch(p)
    {
    case polarity::input:
         return m_uppins;
    case polarity::output:
         return m_dnpins;
    }
}

inline void
node::add_pin(pin &pin)
{
    auto& pvec = pvector(pin.get_polarity());
    pvec.push_back(&pin);
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
    for ( auto& pin : pvector(other.get_polarity()))
        if ( pin->connected(other))
             return true;
    return false;
}

//-------------------------------------------------------------------------------------------------

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
          if (pin->get_label() == ref.toStdString())
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
          if ( pin->get_label() == ref.toStdString())
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

void node::setTarget(QQmlProperty const& p)
{
    // when a node is bound to another node's signal property
    auto _node = qobject_cast<node*>(p.object());
    assert(_node);

    auto _pin = _node->inpin(p.name());
    if ( !_pin ) _pin = _node->outpin(p.name());

    assert(_pin);
    graph::disconnect(*_pin);
    graph::connect(*this, *_pin);
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

WPN_REVISE
stream<signal_t, mallocator<signal_t>>::slice
node::collect(polarity p)
{
    std::vector<pin*>* target;
    stream s;

    switch(p)
    {
    case polarity::input:
    {
        target = &m_uppins; break;
    }
    case polarity::output:
    {
        target = &m_dnpins;
    }
    }

    for ( auto& pin : *target )
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
    wrap(m_stream_pos, m_properties.vsz);
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
std::vector<connection> graph::s_connections;
std::vector<node*> graph::s_nodes;
graph_properties graph::s_properties;

connection& graph::connect(pin& source, pin& dest, connection::pattern pattern)
{
    // TODO: store and move node references into this
    // nodes are owned by the graph
    s_connections.emplace_back(source, dest, pattern);
    return s_connections.back();
}

connection& graph::connect(node& source, node& dest, connection::pattern pattern)
{
    s_connections.emplace_back(*source.outpin(), *dest.inpin(), pattern);
    return s_connections.back();
}

connection& graph::connect(node& source, pin& dest, connection::pattern pattern)
{
    s_connections.emplace_back(*source.outpin(), dest, pattern);
    return s_connections.back();
}

connection& graph::connect(pin& source, node& dest, connection::pattern pattern)
{
    s_connections.emplace_back(source, *dest.inpin(), pattern);
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
    for ( auto& node : s_nodes )
          node->initialize(s_properties);

    for ( auto& connection : s_connections )
          connection.allocate(s_properties.vsz);
}

stream::slice graph::run(node& target)
{
    target.process();
    return target.collect(polarity::input);
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

void Output::start()
{
    graph::initialize();
    emit startStream();
}

void Output::componentComplete()
{
    WPN_REFACTOR
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
