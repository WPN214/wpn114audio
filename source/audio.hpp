#pragma once

using signal_t = double;

#include <vector>
#include <memory>

namespace wpn114
{
namespace signal
{

#define lininterp(_x,_a,_b) _a+_x*(_b-_a)
#define atodb(_s) log10(_s)*20
#define dbtoa(_s) pow(10, _s*.05)
#define wrap(_a, _b) if( _a >=_b ) _a -= _b;

//------------------------------------------------------------------------------------------------
template<typename _Tp>
class allocator
//------------------------------------------------------------------------------------------------
{
    public:
    virtual _Tp* allocate(size_t) = 0;
    virtual void release() = 0;

    virtual ~allocator();
};

//------------------------------------------------------------------------------------------------
template<typename _Tp>
class memallocator final : public allocator<_Tp>
//------------------------------------------------------------------------------------------------
{
    _Tp* m_data = nullptr;

    public:
    memallocator();
    virtual _Tp* allocate(size_t) override;
    virtual void release() override;
};

//------------------------------------------------------------------------------------------------
template<typename _Tp, size_t _Sz>
class stack_allocator final : public allocator<_Tp>
//------------------------------------------------------------------------------------------------
{
    _Tp m_data[_Sz];

    public:
    stack_allocator();
    virtual _Tp* allocate(size_t) override;
    virtual void release() override;
};

//------------------------------------------------------------------------------------------------
template<typename _Tp>
class null_allocator final : public allocator<_Tp>
//------------------------------------------------------------------------------------------------
{
    virtual _Tp* allocate(size_t) override;
    virtual void release() override;
};

//------------------------------------------------------------------------------------------------
template<typename _Basetype, typename _Subtype, typename _Alloc>
class collection
// a collection of frames, channel/stream/sample agnostic
// it just holds an allocator to a contiguous block of data
// and is the base class for the upper-mentioned derived data structures
//------------------------------------------------------------------------------------------------
{
    protected:
    _Alloc m_allocator;
    _Basetype* m_data = nullptr;
    size_t m_size = 0;

    public:
    collection();
    collection(size_t nframes);

    void allocate(size_t nframes);

    virtual ~collection();

    class iterator;
    class accessor;

    // note: this is read-only,
    // in order to modify data, use the accessor instead
    // which has all the cool methods to do it properly
    virtual _Subtype const& operator[](size_t) const;

    virtual iterator begin();
    virtual iterator end();

    accessor operator()(size_t begin = 0, size_t size = 0, size_t pos = 0);
    virtual accessor access(size_t bg = 0, size_t size = 0, size_t pos = 0);

    //------------------------------------------------------------------------------------------------
    class iterator :
    public std::iterator<std::output_iterator_tag, collection>
    //------------------------------------------------------------------------------------------------
    {
        _Subtype* m_pos;

        public:
        iterator(_Subtype* pos);
        iterator& operator++();
        _Subtype& operator*();

        bool operator==(iterator const&);
        bool operator!=(iterator const&);
    };

    //------------------------------------------------------------------------------------------------
    class accessor
    // accessors are used to delimit, iterate over and modify
    // a SLICE of data from a collection
    //------------------------------------------------------------------------------------------------
    {
        protected:
        iterator m_begin, m_pos, m_end;
        size_t m_size = 0;

        public:
        accessor( iterator begin,
                  iterator end,
                  iterator pos,
                  size_t sz);

        size_t size() const;

        class iterator;
        iterator begin();
        iterator end();

        _Subtype& operator[](size_t);

        //---------------------------------------------------------------------------------------------
        class iterator : public std::iterator<std::output_iterator_tag, accessor>
        //---------------------------------------------------------------------------------------------
        {
            _Subtype* m_pos;

            public:
            iterator(_Subtype* pos);
            iterator& operator++();
            _Subtype& operator*();

            bool operator==(iterator const&);
            bool operator!=(iterator const&);
        };
        //---------------------------------------------------------------------------------------------

        // the basic operators are recursive
        // so there's no need for other collections to override them
        // but the lookup, drain, normalize should be overriden

        accessor& operator+=(_Basetype const);
        accessor& operator-=(_Basetype const);
        accessor& operator*=(_Basetype const);
        accessor& operator/=(_Basetype const);

        accessor& operator+=(accessor const&);
        accessor& operator-=(accessor const&);
        accessor& operator*=(accessor const&);
        accessor& operator/=(accessor const&);

        accessor& operator=(accessor const&);
        accessor& operator=(accessor&&);
        accessor& operator=(_Basetype const);

        virtual _Basetype min() const;
        virtual _Basetype max() const;
        virtual _Basetype rms() const;

        virtual void lookup( accessor& source,
                             accessor& head,
                             bool increment = false );

        virtual void drain();
        virtual void normalize();
    };
};

//------------------------------------------------------------------------------------------------
template<typename _Tp, typename _Alloc>
class sample : public collection<_Tp, _Tp, _Alloc>
// sample can either be a temporary reference to a sample in a channel, an interleaved stream
// or a stack-allocated single sample
//------------------------------------------------------------------------------------------------
{
    public:
    // iterator & accesor : handled by collection, no need to override them
    using iterator = typename collection<_Tp, _Tp, _Alloc>::iterator;
    using accessor = typename collection<_Tp, _Tp, _Alloc>::accessor;

    sample();
    sample(size_t nchannels);
    size_t nchannels() const;
};

//------------------------------------------------------------------------------------------------
template<typename _Tp, typename _Alloc>
class channel : public collection<_Tp, _Tp, _Alloc>
// a contiguous block of data representing a single audio channel
// it can be null_allocated if its part of a stream,
// stack_allocated for quicker operations
// or mallocated for larger sizes
//------------------------------------------------------------------------------------------------
{
    public:
    // iterator & accesor : handled by collection, no need to override them
    using iterator = typename collection<_Tp, _Tp, _Alloc>::iterator;
    using accessor = typename collection<_Tp, _Tp, _Alloc>::accessor;

    channel();
    channel(size_t nsamples);
    size_t nsamples() const;
};

//------------------------------------------------------------------------------------------------
template<typename _Basetype, typename _Subtype, typename _Alloc>
class stream_base : public collection<_Basetype, _Subtype, _Alloc>
// the base class for interleaved/non-interleaved streams
// a stream is basically a collection of samples organized by channel
// the organization itself depending on its interleaved/non-interleaved implementation
// (see classes below)
//------------------------------------------------------------------------------------------------
{
    using _collection = collection<_Basetype, _Subtype, _Alloc>;
    using _accessor = typename _collection::accessor;
    using iterator  = typename _collection::iterator;

    protected:
    std::vector<_Subtype> m_subdata;
    size_t m_nchannels = 0;
    size_t m_nsamples = 0;

    public:
    //------------------------------------------------------------------------------------------------
    struct sync
    // a ringbuffer-like implementation
    // allowing stream connections and synchronizations
    //------------------------------------------------------------------------------------------------
    {
        stream_base* _stream = nullptr;
        size_t size = 0;
        size_t pos  = 0;
    };

    stream_base();
    stream_base(size_t nchannels, size_t nsamples);

    size_t nchannels() const;
    size_t nsamples() const;
    size_t nframes() const;

    virtual _Subtype& operator[](size_t) const override;
    virtual iterator begin() override;
    virtual iterator end() override;

    //------------------------------------------------------------------------------------------------
    class accessor : public _accessor
    //------------------------------------------------------------------------------------------------
    {
        public:
        accessor(typename _Subtype::accessor* begin);

        virtual _Basetype min() const override;
        virtual _Basetype max() const override;
        virtual _Basetype rms() const override;

        virtual void lookup( accessor& source,
                             accessor& head,
                             bool increment = false ) override;

        virtual void drain() override;
        virtual void normalize() override;
    };
    //------------------------------------------------------------------------------------------------

    virtual void allocate(size_t nchannels, size_t nsamples) = 0;

    virtual _accessor access(size_t begin = 0, size_t size = 0, size_t pos = 0) override;

    void sync_upstream(stream_base&);
    void sync_dnstream(stream_base&);

    accessor draw_upstream(size_t);
    void pour_downstream(size_t);

    void fwd_upstream(size_t);
    void fwd_downstream(size_t);

    private:
    void synchronize(sync& target, stream_base& s);
    accessor sync_operate(sync& target, size_t sz);
    void synchronize_skip(sync& target, size_t sz);

};

//------------------------------------------------------------------------------------------------

template<typename _Tp, typename _Alloc>
class channeled_stream;

template<typename _Tp, typename _Alloc>
using non_interleaved_stream = channeled_stream<_Tp, _Alloc>;

//------------------------------------------------------------------------------------------------
// the role of the following derived classes is to build the subcollections,
// i.e. to arrange and order the streams of data into sample or channel collections
//------------------------------------------------------------------------------------------------
template<typename _Tp, typename _Alloc>
class interleaved_stream : public stream_base<
        _Tp, sample<_Tp, null_allocator<_Tp>>, _Alloc>
// interleaved streams are faster for slice manipulation and operations
// as the blocks of interleaved samples are aligned
//------------------------------------------------------------------------------------------------
{
    using _channeled_stream = channeled_stream<_Tp, null_allocator<_Tp>>;

    public:
    interleaved_stream();
    interleaved_stream(size_t nchannels, size_t nsamples);

    virtual void allocate(size_t nchannels, size_t nsamples) override;

    // reorder (cast) as a non-interleaved stream
    _channeled_stream deinterleave();
    operator _channeled_stream();
};

//------------------------------------------------------------------------------------------------
template<typename _Tp, typename _Alloc>
class channeled_stream : public stream_base<
        _Tp, channel<_Tp, null_allocator<_Tp>>, _Alloc>
//  channeled or non_interleaved streams are slower for global slice operations
// but faster for channel-by-channel operations (like spatialization) or pan-related effects
//------------------------------------------------------------------------------------------------
{
    using _interleaved_stream = interleaved_stream<_Tp, null_allocator<_Tp>>;

    public:
    channeled_stream();
    channeled_stream(size_t nchannels, size_t nsamples);

    virtual void allocate(size_t nchannels, size_t nsamples) override;

    // reorder (cast) as an interleaved stream
    _interleaved_stream interleave();
    operator _interleaved_stream();


};

}
}

using namespace wpn114::signal;

void test()
{
    using stream = channeled_stream<double, memallocator<double>>;
    stream s;
    auto acc = s();

    // that's one way
    for ( auto& ch : acc )
        for ( auto& s : ch )
              s *= 0.5;
    // or
    acc *= 0.5;
}
