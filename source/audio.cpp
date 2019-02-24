#include "audio.hpp"
#include <memory>
#include <cassert>

using namespace wpn114::signal;
// green changed to white, emerald to opal
// nothing was changed

template<typename T>
memallocator<T>::memallocator() {}

template<typename T>
T* memallocator<T>::allocate(size_t sz)
{
    return m_data = static_cast<T*>(
           malloc(sizeof(T)*sz)); // for now..
}

template<typename T>
void memallocator<T>::release()
{
    free(m_data);
}

template<typename T, size_t S>
stack_allocator<T,S>::stack_allocator() {}

template<typename T, size_t S>
T* stack_allocator<T,S>::allocate(size_t sz)
{
    assert( S == sz );
    return &m_data;
}

template<typename T, size_t S>
void stack_allocator<T,S>::release() {}

template<typename T>
T* null_allocator<T>::allocate(size_t)
{
    return nullptr;
}

template<typename T>
void null_allocator<T>::release() {}

//-------------------------------------------------------------------------------------------------
// COLLECTION
//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
collection<T,S,A>::collection() {}

template<typename T, typename S, typename A>
collection<T,S,A>::collection(size_t nframes) : m_size(nframes)
{
    allocate(nframes);
}

template<typename T, typename S, typename A>
void collection<T,S,A>::allocate(size_t nframes)
{
    m_data = m_allocator.allocate(nframes);
}

template<typename T, typename S, typename A>
collection<T,S,A>::~collection()
{
    m_allocator.release();
}

template<typename T, typename S, typename A>
S const& collection<T,S,A>::operator[](size_t index) const
{
    return m_data[index];
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::iterator
collection<T,S,A>::begin()
{
    return iterator(m_data);
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::iterator
collection<T,S,A>::end()
{
    return iterator(&m_data[m_size]);
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor
collection<T,S,A>::operator()(size_t bg, size_t sz, size_t pos)
{
    return access(bg, sz, pos);
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor
collection<T,S,A>::access(size_t bg, size_t sz, size_t pos)
{
    return accessor(iterator(&m_data[bg]),
                    iterator(&m_data[bg+sz]),
                    iterator(&m_data[bg+pos]), sz);
}

//-------------------------------------------------------------------------------------------------
// COLLECTION::ITERATOR
//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
collection<T,S,A>::iterator::iterator(S* pos) : m_pos(pos) {}

template<typename T, typename S, typename A>
typename collection<T,S,A>::iterator&
collection<T,S,A>::iterator::operator++()
{
    m_pos++;
    return *this;
}

template<typename T, typename S, typename A>
S& collection<T,S,A>::iterator::operator*()
{
    return *m_pos;
}

template<typename T, typename S, typename A>
bool collection<T,S,A>::iterator::operator==(iterator const& other)
{
    return m_pos == other.m_pos;
}

template<typename T, typename S, typename A>
bool collection<T,S,A>::iterator::operator!=(iterator const& other)
{
    return !operator==(other);
}

//-------------------------------------------------------------------------------------------------
// COLLECTION::ACCESSOR
//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
collection<T,S,A>::accessor::accessor(
        collection<T,S,A>::iterator begin,
        collection<T,S,A>::iterator end,
        collection<T,S,A>::iterator pos,
        size_t sz) : m_begin(begin), m_end(end), m_pos(pos), m_size(sz) {}

template<typename T, typename S, typename A>
size_t collection<T,S,A>::accessor::size() const
{
    return m_size;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor::iterator
collection<T,S,A>::accessor::begin()
{
    return iterator(&*m_begin);
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor::iterator
collection<T,S,A>::accessor::end()
{
    return iterator(&*m_end);
}

template<typename T, typename S, typename A>
S& collection<T,S,A>::accessor::operator[](size_t index)
{
    return (*m_begin)[index];
}

//-------------------------------------------------------------------------------------------------
// ACCESSOR::ITERATOR
//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor::iterator&
collection<T,S,A>::accessor::iterator::operator++()
{
    m_pos++; return *this;
}

template<typename T, typename S, typename A>
S& collection<T,S,A>::accessor::iterator::operator*()
{
    return *m_pos;
}

template<typename T, typename S, typename A>
bool collection<T,S,A>::accessor::iterator::operator==(iterator const& other)
{
    return m_pos == other.m_pos;
}

template<typename T, typename S, typename A>
bool collection<T,S,A>::accessor::iterator::operator!=(iterator const& other)
{
    return !operator==(other);
}

//-------------------------------------------------------------------------------------------------
// ACCESSOR:OPERATORS
//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator=(T const v)
{
    for ( auto& sub : *this )
          sub = v;

    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator+=(T const v)
{
    for ( auto& sub : *this )
          sub += v;

    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator-=(T const v)
{
    for ( auto& sub : *this )
          sub -= v;

    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator*=(T const v)
{
    for ( auto& sub : *this )
          sub *= v;

    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator/=(T const v)
{
    for ( auto& sub : *this )
          sub /= v;

    return *this;
}

//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator+=(accessor const& other)
{
    size_t sz = std::min(m_size, other.m_size);
    for ( size_t n = 0; n < sz; ++n)
          operator[](n) += other[n];

    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator-=(accessor const& other)
{
    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator*=(accessor const& other)
{
    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator/=(accessor const& other)
{
    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator=(accessor const& other)
{
    return *this;
}

template<typename T, typename S, typename A>
typename collection<T,S,A>::accessor&
collection<T,S,A>::accessor::operator=(accessor&& other)
{
    return *this;
}

//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
T collection<T,S,A>::accessor::min() const
{
    T m = 0;
    for ( auto& sub : *this )
          m = std::min(m, sub);

    return m;
}

template<typename T, typename S, typename A>
T collection<T,S,A>::accessor::max() const
{
    T m = 0;
    for ( auto& sub : *this )
          m = std::max(m, sub);

    return m;
}

template<typename T, typename S, typename A>
T collection<T,S,A>::accessor::rms() const
{

}

//-------------------------------------------------------------------------------------------------
template<typename T, typename S, typename A>
void collection<T,S,A>::accessor::drain()
{
    memset(&*m_begin, 0, sizeof(T)*m_size);
}

template<typename T, typename S, typename A>
void collection<T,S,A>::accessor::normalize()
{

}

template<typename T, typename S, typename A>
void collection<T,S,A>::accessor::lookup(accessor& source,
                                         accessor& head,
                                         bool increment )
{

}

//-------------------------------------------------------------------------------------------------
// SAMPLE
//-------------------------------------------------------------------------------------------------
template<typename T, typename A>
sample<T,A>::sample() {}

template<typename T, typename A>
sample<T,A>::sample(size_t nchannels) : collection<T,T,A>(nchannels) {}

template<typename T, typename A>
size_t sample<T,A>::nchannels() const
{
    return this->m_size;
}

//-------------------------------------------------------------------------------------------------
// CHANNEL
//-------------------------------------------------------------------------------------------------

template<typename T, typename A>
channel<T,A>::channel() {}

template<typename T, typename A>
channel<T,A>::channel(size_t nsamples) : collection<T,T,A>(nsamples) {}

template<typename T, typename A>
size_t channel<T,A>::nsamples() const
{
    return this->m_size;
}

//-------------------------------------------------------------------------------------------------
// STREAM_BASE
//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
stream_base<T,S,A>::stream_base() {}

template<typename T, typename S, typename A>
stream_base<T,S,A>::stream_base(size_t nchannels, size_t nsamples) :
    collection<T,S,A>(nchannels*nsamples),
    m_nchannels(nchannels),
    m_nsamples(nsamples) {}

template<typename T, typename S, typename A>
size_t stream_base<T,S,A>::nchannels() const
{
    return m_nchannels;
}

template<typename T, typename S, typename A>
size_t stream_base<T,S,A>::nsamples() const
{
    return m_nsamples;
}

template<typename T, typename S, typename A>
size_t stream_base<T,S,A>::nframes() const
{
    return this->m_size;
}

template<typename T, typename S, typename A>
S& stream_base<T,S,A>::operator[](size_t index) const
{
    return m_subdata[index];
}

//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
typename stream_base<T,S,A>::iterator
stream_base<T,S,A>::begin()
{
    return iterator(m_subdata.begin());
}

template<typename T, typename S, typename A>
typename stream_base<T,S,A>::iterator
stream_base<T,S,A>::end()
{
    return iterator(m_subdata.end());
}

template<typename T, typename S, typename A>
typename stream_base<T,S,A>::_accessor
stream_base<T,S,A>::access(size_t begin, size_t sz, size_t pos)
{

}

//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
void stream_base<T,S,A>::sync_upstream(stream_base& upstream)
{

}

template<typename T, typename S, typename A>
void stream_base<T,S,A>::sync_dnstream(stream_base& dnstream)
{

}

template<typename T, typename S, typename A>
typename stream_base<T,S,A>::accessor
stream_base<T,S,A>::draw_upstream(size_t sz)
{

}

template<typename T, typename S, typename A>
void stream_base<T,S,A>::pour_downstream(size_t sz)
{

}

template<typename T, typename S, typename A>
void stream_base<T,S,A>::fwd_upstream(size_t sz)
{

}

template<typename T, typename S, typename A>
void stream_base<T,S,A>::fwd_downstream(size_t sz)
{

}

//-------------------------------------------------------------------------------------------------
// STREAM_BASE::ACCESSOR
//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
stream_base<T,S,A>::accessor::accessor(typename S::accessor* begin)
{

}

template<typename T, typename S, typename A>
T stream_base<T,S,A>::accessor::min() const
{

}

template<typename T, typename S, typename A>
T stream_base<T,S,A>::accessor::max() const
{

}

template<typename T, typename S, typename A>
T stream_base<T,S,A>::accessor::rms() const
{

}

template<typename T, typename S, typename A>
void stream_base<T,S,A>::accessor::drain()
{

}

template<typename T, typename S, typename A>
void stream_base<T,S,A>::accessor::normalize()
{

}

template<typename T, typename S, typename A>
void stream_base<T,S,A>::accessor::lookup(accessor& source,
                                          accessor& head,
                                          bool increment )
{

}

//-------------------------------------------------------------------------------------------------

template<typename T, typename S, typename A>
void stream_base<T,S,A>::synchronize(sync& target, stream_base& s)
{

}

template<typename T, typename S, typename A>
typename stream_base<T,S,A>::accessor
stream_base<T,S,A>::sync_operate(sync& target, size_t sz)
{

}

template<typename T, typename S, typename A>
void stream_base<T,S,A>::synchronize_skip(sync& target, size_t sz)
{

}

//-------------------------------------------------------------------------------------------------
// INTERLEAVED_STREAM
//-------------------------------------------------------------------------------------------------

template<typename T, typename A>
interleaved_stream<T,A>::interleaved_stream() {}

template<typename T, typename A>
interleaved_stream<T,A>::interleaved_stream(size_t nchannels, size_t nsamples) :
    stream_base<T, sample<T, null_allocator<T>>, A>(nchannels, nsamples)
{
    // build samples
}

template<typename T, typename A>
void interleaved_stream<T,A>::allocate(size_t nchannels, size_t nsamples)
{

}

template<typename T, typename A>
interleaved_stream<T,A>::operator channeled_stream<T, null_allocator<T>>()
{
    channeled_stream<T, null_allocator<T>> stream;
}

template<typename T, typename A>
typename interleaved_stream<T,A>::_channeled_stream
interleaved_stream<T,A>::deinterleave()
{
    return static_cast<_channeled_stream>(*this);
}

//-------------------------------------------------------------------------------------------------
// CHANNELED_STREAM
//-------------------------------------------------------------------------------------------------

template<typename T, typename A>
channeled_stream<T,A>::channeled_stream() {}

template<typename T, typename A>
channeled_stream<T,A>::channeled_stream(size_t nchannels, size_t nsamples) :
    stream_base<T, channel<T, null_allocator<T>>, A>(nchannels, nsamples)
{
    // build channels
}

template<typename T, typename A>
void channeled_stream<T,A>::allocate(size_t nchannels, size_t nsamples)
{

}

template<typename T, typename A>
channeled_stream<T,A>::operator interleaved_stream<T,null_allocator<T>>()
{
    interleaved_stream<T, null_allocator<T>> stream;
}

template<typename T, typename A>
typename channeled_stream<T,A>::_interleaved_stream
channeled_stream<T,A>::interleave()
{
    return static_cast<_interleaved_stream>(*this);
}
