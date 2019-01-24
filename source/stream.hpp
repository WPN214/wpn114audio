#ifndef STREAM_TPP
#define STREAM_TPP

#include <vector>
#include <memory>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <cmath>
// ===========================================================================================
#define WPN_UNSAFE
#define WPN_UNIMPLEMENTED
#define WPN_DEPRECATED
#define WPN_OPTIMIZE
#define WPN_REFACTOR
#define WPN_EXCEPT
#define WPN_EXAMINE
// ===========================================================================================
#define WPN_STACK_CHANNEL_MAX_SIZE 512
#define WPN_COMPILE_FLOAT_PRECISION double
using signal_t = WPN_COMPILE_FLOAT_PRECISION;
enum class level_t { linear = 0, dbfs = 1 };
// ===========================================================================================
namespace wpn114
{
namespace signal
{
// ===========================================================================================
#define ATODB(_a) std::log10(_a)*20
#define DBTOA(_d) std::pow(10, _d*.05)
#define LININTERP(_x,_a,_b) _a+_x*(_b-_a)



// ===========================================================================================
// heap-allocated channel
class channel
// ===========================================================================================
{
    friend class scoped_accessor;

    public:
    class scoped_accessor;

    // constructors
    channel();
    channel(size_t size, signal_t fill = 0);
    channel(channel const&);
    channel(channel&&) = delete;

    // assignment operators
    channel& operator=(channel const&);
    channel& operator=(channel&&) = delete;

    // accessors (~= slices in Rust/Dlang)
    operator scoped_accessor();
    scoped_accessor accessor(size_t begin = 0, size_t size = 0, size_t pos = 0);

    protected:
    signal_t  _rate;
    signal_t* _data;
    size_t  _size;

    public:
    // ======================================================================================
    class scoped_accessor /* channel::scoped_accessor */
    // ======================================================================================
    {
        friend class channel;
        public:

        scoped_accessor();
        class iterator;
        iterator begin();
        iterator end();

        signal_t& operator[](size_t);
        size_t size() const;
        scoped_accessor& operator=(signal_t const);
        scoped_accessor& operator++();

        // stream operators --------------------------------
        scoped_accessor& operator<<(scoped_accessor const&);
        scoped_accessor& operator>>(scoped_accessor &);
        scoped_accessor& operator<<(signal_t const);
        scoped_accessor& operator>>(signal_t&) const;

        // merge operators ---------------------------------
        scoped_accessor& operator<<=(scoped_accessor &);
        scoped_accessor& operator>>=(scoped_accessor &);
        scoped_accessor& operator<<=(signal_t const);
        scoped_accessor& operator>>=(signal_t&);

        // arithm. -----------------------------------------
        scoped_accessor& operator+=(scoped_accessor const&);
        scoped_accessor& operator-=(scoped_accessor const&);
        scoped_accessor& operator*=(scoped_accessor const&);
        scoped_accessor& operator/=(scoped_accessor const&);

        scoped_accessor& operator+=(const signal_t);
        scoped_accessor& operator-=(const signal_t);
        scoped_accessor& operator*=(const signal_t);
        scoped_accessor& operator/=(const signal_t);

        // signal-related -------------------
        signal_t min(level_t = level_t::linear);
        signal_t max(level_t = level_t::linear);
        signal_t rms(level_t = level_t::linear);

        void drain();
        void normalize();

        private:
        scoped_accessor(channel&, signal_t* begin, signal_t* end, signal_t* pos );
        scoped_accessor* minsz(scoped_accessor& lhs, scoped_accessor const& rhs);
        channel& _parent;
        signal_t* _begin;
        signal_t* _end;
        signal_t* _pos;
        size_t _size;

        public:
        //----------------------------------------------------------------------------------
        class iterator :
        public std::iterator<std::input_iterator_tag, signal_t>
        //----------------------------------------------------------------------------------
        {
            public:
            iterator(signal_t* data);
            iterator& operator++();
            signal_t& operator*();
            bool operator==(const iterator& s);
            bool operator!=(const iterator& s);

            private:
            signal_t* _data;
        };
        // --------------------------------------------------------------------------------
    };
};

// ========================================================================================
class stream
// a vector of channels
// ========================================================================================
{
    public:
    stream();
    stream(size_t nchannels);
    stream(size_t nchannels, size_t channel_size, signal_t fill = 0);
    stream(stream const&);
    stream(stream&&) = delete;

    stream& operator=(stream const&);
    stream& operator=(stream&&) = delete;

    void allocate(size_t nchannels, size_t size);
    void allocate(size_t size);

    class scoped_accessor;
    operator scoped_accessor();
    scoped_accessor accessor(size_t begin = 0, size_t size = 0, size_t pos = 0);

    channel& operator[](size_t);

    // -----------------------------------------------
    // syncs
    //
    // -----------------------------------------------
    struct sync
    {
        std::shared_ptr<stream> _stream;
        size_t size;
        size_t pos;
    };

    void add_sync(stream&, sync&);
    void add_upsync(stream&);
    void add_dnsync(stream&);

    scoped_accessor draw(size_t);
    void pour(size_t);

    size_t size() const;
    size_t nchannels() const;

    protected:
    std::vector<channel*> _channels;
    size_t _size;
    sync _upsync;
    sync _dnsync;

    size_t _nchannels;

    public:
    // ======================================================================================
    class scoped_accessor
    // a temporary reference to a stream, or to a fragment of a stream
    // only accessors are allowed to read/write from a stream
    // ======================================================================================
    {
        public:        
        scoped_accessor();
        scoped_accessor(scoped_accessor const&);
        scoped_accessor(scoped_accessor&&) = delete;

        scoped_accessor& operator=(scoped_accessor const&);

        class iterator;
        iterator begin();
        iterator end();
        channel::scoped_accessor operator[](size_t);
        operator bool();

        // arithm. -------------------------
        scoped_accessor& operator=(signal_t);
        scoped_accessor& operator+=(signal_t);
        scoped_accessor& operator-=(signal_t);
        scoped_accessor& operator*=(signal_t);
        scoped_accessor& operator/=(signal_t);

        scoped_accessor& operator<<(scoped_accessor const&);
        scoped_accessor& operator>>(scoped_accessor&);

        void lookup(scoped_accessor&, scoped_accessor const&);

        size_t size() const;
        size_t nchannels() const;
        void drain();

        private:
        scoped_accessor(stream& parent, size_t begin, size_t size, size_t pos);
        stream& _parent;
        size_t _begin;
        size_t _size;
        size_t _pos;

        std::vector<channel::scoped_accessor> _accessors;

        public:
        //-----------------------------------------------------------------------------------
        class iterator :
        public std::iterator<std::input_iterator_tag, signal_t>
        //-----------------------------------------------------------------------------------
        {
            public:
            iterator(std::vector<channel::scoped_accessor>::iterator data);
            iterator& operator++();
            channel::scoped_accessor operator*();

            bool operator==(const iterator& s);
            bool operator!=(const iterator& s);

            private:
            std::vector<channel::scoped_accessor>::iterator _data;
        };
        //-----------------------------------------------------------------------------------
    };
};

// ===========================================================================================
// STACK CHANNEL, no heap allocation is needed for this one
// this is used for in-node process calculations
template <typename T = double, size_t S = 512>
class stack_channel : public channel
// ===========================================================================================
{
    public:
    stack_channel()
    {
        _data = _sdata;
        _size = S;
    }

    stack_channel(size_t size, T fill = 0)
    {
        _size = size;
        *this = fill;
    }

    private:
    T _sdata[S];
};

// ===========================================================================================
// STACK STREAM, no heap allocation is needed for this one
// this is used for in-node process calculations
template<typename T = signal_t, size_t N = 2, size_t S = 512>
class stack_stream : public stream
// ===========================================================================================
{
    public:
    stack_stream()
    {
        for ( size_t n = 0; n < N; ++n )
            _channels.push_back(_schannels[n]);
    }

    stack_stream(size_t size, T fill = 0)
    {

    }

    private:
    stack_channel<T,S> _schannels[N];
};


}
}

#endif // STREAM_TPP
