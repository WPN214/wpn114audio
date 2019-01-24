#include "stream.hpp"
#include <iomanip>
#include <utility>

// ==========================================================================================
// WPN114::SIGNAL::CHANNEL
using namespace wpn114::signal;
// ==========================================================================================

channel::channel() : _rate(0), _data(nullptr), _size(0) {}

channel::channel(size_t size, signal_t fill) : _rate(0)
{
    _data = new signal_t[size];
    std::fill_n(_data, size, fill);
}

WPN_UNSAFE channel::channel(channel const& copy) :
    _size(copy._size), _rate(copy._rate)
{
    std::copy_n(copy._data, _size, _data);
}

channel& channel::operator=(channel const& copy)
{
    _size = copy._size;
    _rate = copy._rate;

    std::copy_n(copy._data, _size, _data);
    return *this;
}

channel::operator scoped_accessor()
{
    return scoped_accessor(*this, _data, _data+_size, _data);
}

channel::scoped_accessor
channel::accessor(size_t begin, size_t size, size_t pos)
{
    return scoped_accessor(*this, _data+begin, _data+size, _data+begin+pos);
}

// ==========================================================================================
// WPN114::SIGNAL::CHANNEL::SCOPED_ACCESSOR
// ==========================================================================================
#define FOR_CHANNEL_SAMPLE(_n) for(size_t _n = 0; _n < _size; ++_n)
//-------------------------------------------------------------------------------------------

channel::scoped_accessor::scoped_accessor(channel& parent, signal_t* b, signal_t* e, signal_t* pos) :
    _parent(parent), _begin(b), _end(e), _pos(pos) {}

channel::scoped_accessor::iterator
channel::scoped_accessor::begin()
{
    return channel::scoped_accessor::iterator(_begin);
}

channel::scoped_accessor::iterator
channel::scoped_accessor::end()
{
    return channel::scoped_accessor::iterator(_end);
}

signal_t& channel::scoped_accessor::operator[](size_t index)
{
    return _begin[index];
}

channel::scoped_accessor&
channel::scoped_accessor::operator=(signal_t const s)
{
    FOR_CHANNEL_SAMPLE(n) _begin[n] = s;
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator<<(scoped_accessor const& other)
{
    return *this += other;
}

channel::scoped_accessor&
channel::scoped_accessor::operator>>(scoped_accessor& other)
{
    other << *this;
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator<<=(scoped_accessor& other)
{
    // copy data into this
    *this += other;
    other = 0;
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator>>=(scoped_accessor& other)
{
    other <<= *this;
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator<<=(const signal_t s)
{
    operator=(s);
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator>>=(signal_t& s)
{
    // sums this into s
    FOR_CHANNEL_SAMPLE(n)
           s += _begin[n];
    return *this;
}

__always_inline channel::scoped_accessor*
channel::scoped_accessor::minsz(
        channel::scoped_accessor& lhs,
        channel::scoped_accessor const& rhs)
{
    if ( lhs._size < rhs._size )
         return &lhs;
    else return const_cast<scoped_accessor*>(&rhs);
}

// ------------------------------------------------------------------------------------------
#define WPN_CHANNEL_EQOPERATOR(_op)                         \
for ( size_t n = 0; n < minsz(*this, other)->_size; ++n )   \
      _begin[n] _op other._begin[n];
// ------------------------------------------------------------------------------------------

channel::scoped_accessor&
channel::scoped_accessor::operator+=(scoped_accessor const& other)
{
    WPN_CHANNEL_EQOPERATOR(+=);
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator-=(scoped_accessor const& other)
{
    WPN_CHANNEL_EQOPERATOR(-=);
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator*=(scoped_accessor const& other)
{
    WPN_CHANNEL_EQOPERATOR(*=);
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator/=(scoped_accessor const& other)
{
    WPN_CHANNEL_EQOPERATOR(/=);
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator+=(signal_t const s)
{
    FOR_CHANNEL_SAMPLE(n)
          _begin[n] += s;
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator-=(signal_t const s)
{
    FOR_CHANNEL_SAMPLE(n)
          _begin[n] -= s;
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator*=(signal_t const s)
{
    FOR_CHANNEL_SAMPLE(n)
          _begin[n] *= s;
    return *this;
}

channel::scoped_accessor&
channel::scoped_accessor::operator/=(signal_t const s)
{
    FOR_CHANNEL_SAMPLE(n)
          _begin[n] /= s;
    return *this;
}

// ------------------------------------------------------------------------------------------

signal_t channel::scoped_accessor::min(level_t t)
{
    signal_t min = 0;
    FOR_CHANNEL_SAMPLE(n)
             min = std::min(min, _begin[n]);

    WPN_EXAMINE
    if ( t == level_t::linear )
         return min;
    else return ATODB(min);
}

signal_t channel::scoped_accessor::max(level_t t)
{
    signal_t max = 0;
    FOR_CHANNEL_SAMPLE(n)
             max = std::max(max, _begin[n]);

    WPN_EXAMINE
    if ( t == level_t::linear)
         return max;
    else return ATODB(max);
}

signal_t channel::scoped_accessor::rms(level_t t)
{
    signal_t mean = 0, rms = 0;
    FOR_CHANNEL_SAMPLE(n)
             mean += _begin[n];

    rms = sqrt(1.0/_size*mean);

    WPN_EXAMINE
    if ( t == level_t::linear )
         return rms;
    else return ATODB(rms);
}

void channel::scoped_accessor::drain()
{
    std::memset(_begin, 0, _size);
}

void channel::scoped_accessor::normalize()
{
    signal_t m = max();
    FOR_CHANNEL_SAMPLE(n)
        _begin[n] *= 1.0/m;
}

size_t channel::scoped_accessor::size() const
{
    return _size;
}

// ==========================================================================================
// WPN114::SIGNAL::CHANNEL::SCOPED_ACCESSOR::ITERATOR
// ==========================================================================================

channel::scoped_accessor::iterator::iterator(signal_t* data) :
    _data(data) {}

channel::scoped_accessor::iterator&
channel::scoped_accessor::iterator::operator++()
{
    _data++; return *this;
}

signal_t& channel::scoped_accessor::iterator::operator*()
{
    return *_data;
}

bool channel::scoped_accessor::iterator::operator==(
        const channel::scoped_accessor::iterator& it)
{
    return _data == it._data;
}

bool channel::scoped_accessor::iterator::operator!=(
        const channel::scoped_accessor::iterator& it)
{
    return _data != it._data;
}

// ==========================================================================================
// WPN114::SIGNAL::STREAM
#define WPN_STREAM_RESERVE_DEFAULT 20
// ==========================================================================================

// default constructor, when nchannels and channel_size is not known upstream
stream::stream() : _size(0), _nchannels(0)
{
    _channels.reserve( WPN_STREAM_RESERVE_DEFAULT );
}

// nchannels is known, but we don't allocate yet
stream::stream(size_t nchannels) : _size(0), _nchannels(nchannels)
{
    _channels.reserve(nchannels);
}

stream::stream(size_t nchannels, size_t channel_size, signal_t fill) :
    _size(channel_size), _nchannels(nchannels)
{
    for ( size_t n = 0; n < nchannels; ++n)
        _channels.push_back(new channel(channel_size, fill));
}

WPN_UNIMPLEMENTED
stream::stream(stream const& copy)
{

}

WPN_UNIMPLEMENTED
stream& stream::operator=(stream const& copy)
{

}

size_t stream::size() const
{
    return _size;
}

size_t stream::nchannels() const
{
    return _nchannels;
}

channel& stream::operator[](size_t t)
{
    return *_channels[t];
}

// returns an accessor on the whole stream
stream::operator scoped_accessor()
{
    return stream::accessor(0, _size);
}

stream::scoped_accessor
stream::accessor(size_t b, size_t e, size_t pos)
{
    return stream::accessor(b, e, pos);
}

void stream::allocate(size_t nchannels, size_t size)
{
    _nchannels = nchannels;
    _size = size;

    while( nchannels )
    {
        _channels.push_back(new channel(size, 0));
        nchannels--;
    }
}

// in the case that we already know how many channels there will be
void stream::allocate(size_t size)
{
    for ( size_t n = 0; n < _nchannels; ++n )
        _channels.push_back(new channel(size, 0));
}

inline void
stream::add_sync(stream& s, sync& sy)
{
    sy._stream  = std::shared_ptr<stream>(&s);
    sy.size     = _size;
    sy.pos      = 0;
}

void stream::add_upsync(stream& s)
{
    add_sync(s, _upsync);
}

void stream::add_dnsync(stream& s)
{
    add_sync(s, _dnsync);
}

stream::scoped_accessor
stream::draw(size_t sz)
{
    auto acc = accessor(_upsync.pos, sz);       
    acc << _upsync._stream->accessor(_upsync.pos, sz);
    _upsync.pos += sz;
    return acc;
}

void stream::pour(size_t sz)
{
    auto acc = accessor(_dnsync.pos, sz);
    auto dn_acc = _dnsync._stream->accessor(_dnsync.pos, sz);
    acc >> dn_acc;
    _dnsync.pos += sz;
}

// ==========================================================================================
// WPN114::SIGNAL::STREAM::SCOPED_ACCESSOR
// ==========================================================================================

stream::scoped_accessor::scoped_accessor(stream& parent,
        size_t begin, size_t size, size_t pos) :
        _parent(parent), _begin(begin), _size(size), _pos(pos)
{
    for ( size_t n = 0; n < parent.nchannels(); ++n)
        _accessors.push_back(parent[n].accessor(begin, size));
}

stream::scoped_accessor::iterator
stream::scoped_accessor::begin()
{
    return stream::scoped_accessor::iterator(_accessors.begin());
}

stream::scoped_accessor::iterator
stream::scoped_accessor::end()
{
    return stream::scoped_accessor::iterator(_accessors.end());
}

channel::scoped_accessor
stream::scoped_accessor::operator[](size_t index)
{
    return _accessors[index];
}

size_t stream::scoped_accessor::nchannels() const
{
    return _accessors.size();
}

size_t stream::scoped_accessor::size() const
{
    return _size;
}

void stream::scoped_accessor::drain()
{
    for ( auto& channel : _accessors )
          channel.drain();
}

WPN_UNIMPLEMENTED
stream::scoped_accessor::operator bool()
{
    return true;
}

WPN_OPTIMIZE
stream::scoped_accessor&
stream::scoped_accessor::operator=(signal_t v)
{
    for ( auto& channel : _accessors )
          channel = v;
    return *this;
}

WPN_OPTIMIZE
stream::scoped_accessor&
stream::scoped_accessor::operator+=(signal_t v)
{
    for ( auto& channel : _accessors )
          channel += v;
    return *this;
}

WPN_OPTIMIZE
stream::scoped_accessor&
stream::scoped_accessor::operator*=(signal_t v)
{
    for ( auto& channel : _accessors )
          channel += v;
    return *this;
}

WPN_OPTIMIZE
stream::scoped_accessor&
stream::scoped_accessor::operator/=(signal_t v)
{
    for ( auto& channel : _accessors )
          channel /= v;
    return *this;
}

WPN_OPTIMIZE
stream::scoped_accessor&
stream::scoped_accessor::operator<<(scoped_accessor const& other)
{
    // merge streambuffers
    for ( size_t n = 0; n < other.nchannels(); ++n )
         _accessors[n] << other._accessors[n];

    return *this;
}

WPN_OPTIMIZE
stream::scoped_accessor&
stream::scoped_accessor::operator>>(scoped_accessor& other)
{
    other << *this;
    return *this;
}

WPN_UNIMPLEMENTED
void stream::scoped_accessor::lookup( scoped_accessor& source,
                                      const scoped_accessor & index)
{



}

// ----------------------------------------------------------------------------------------------
// STREAM::SCOPED_ACCESSOR::ITERATOR
stream::scoped_accessor::iterator::iterator(
        std::vector<channel::scoped_accessor>::iterator data) : _data(data) {}
// ----------------------------------------------------------------------------------------------

stream::scoped_accessor::iterator&
stream::scoped_accessor::iterator::operator++()
{
    _data++; return *this;
}

channel::scoped_accessor
stream::scoped_accessor::iterator::operator*()
{
    return *_data;
}

bool stream::scoped_accessor::iterator::operator==(iterator const& s)
{
    return s._data == _data;
}

bool stream::scoped_accessor::iterator::operator!=(iterator const& s)
{
    return s._data != _data;
}





