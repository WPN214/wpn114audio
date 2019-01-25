#pragma once

#include "stream.hpp"
#include <thread>

namespace wpn114 {
namespace signal {

// ==========================================================================================
// THE DSP-CHAIN ALGORITHM
//
// 0--
//
// ==========================================================================================
// forwards:
class connection;
// ==========================================================================================
enum polarity_t { output = 0, input = 1 };
// ==========================================================================================
struct graph_properties
// ==========================================================================================
{
    signal_t rate;
    size_t vsz;
    size_t fvsz;
};
// ==========================================================================================
class node
// ==========================================================================================
{
    friend class connection;
    friend class graph;

    public:
    //---------------------------------------------------------------------------------------
    class pin
    //---------------------------------------------------------------------------------------
    {
        friend class node;
        friend class connection;
        friend class graph;

        public:
        pin (node& parent, polarity_t t, std::string_view label, size_t nchannels, bool def );

        bool connected();
        bool connected(node&);
        bool connected(node::pin&);

        protected:
        void allocate(size_t size);
        void add_connection(connection& con);
        void remove_connection(connection& con);

        private:
        node& _parent;
        polarity_t _polarity;
        std::vector<connection*> _connections;
        std::string _label;
        stream _stream;
        size_t _nchannels;
        bool _default;
    };
    //---------------------------------------------------------------------------------------
    struct pinstream_ref
    // a reference to the stream of a specific input/output pin
    //---------------------------------------------------------------------------------------
    {
        stream::scoped_accessor _stream;
        std::string const& label;
    };

    //---------------------------------------------------------------------------------------
    struct pool
    // a stream vector, for each pin
    //---------------------------------------------------------------------------------------
    {
        stream::scoped_accessor& operator[](std::string_view);
        std::vector<pinstream_ref> _streams;
    };

    node();

    size_t num_uppins() const;
    size_t num_dnpins() const;

    void num_uppins(size_t);
    void num_dnpins(size_t);

    pin& inpin();
    pin& inpin(size_t index);
    pin& inpin(std::string_view ref);

    pin& outpin();
    pin& outpin(size_t index);
    pin& outpin(std::string_view ref);

    //---------------------------------------------------------------------------------------
    protected:
    graph_properties _properties;

    // it should be string_views, not strings
    void decl_input(std::string_view label, size_t nchannels, bool def = false);
    void decl_output(std::string_view label, size_t nchannels, bool def = false);

    virtual void initialize  ( graph_properties properties ) noexcept = 0;
    virtual void rwrite      ( node::pool& in, node::pool& out, size_t sz ) noexcept = 0;

    private:
    pool make_pools   ( std::vector<pin>& pinvector, size_t sz ) noexcept;
    void configure    ( graph_properties properties ) noexcept;
    bool connected    ( node const& other ) const noexcept;

    void process() noexcept;

    size_t _pos;
    bool _intertwined = false;
    std::vector<pin> _uppins;
    std::vector<pin> _dnpins;

    WPN_UNIMPLEMENTED
    // this will be for later use
    // in the spatialization-rendering context
    signal_t _x = 0;
    signal_t _y = 0;
    signal_t _w = 0;
    signal_t _h = 0;

};// node
// ==========================================================================================
class connection
// ==========================================================================================
{
    friend class node;
    friend class graph;

    WPN_UNIMPLEMENTED
    enum class pattern
    {
        merge   = 0,
        append  = 1,
        split   = 2
    };

    public:
    using matrix = std::vector<std::pair<size_t,size_t>>;

    connection(connection const&) = delete;
    connection(connection&&) = delete;
    connection& operator=(connection const&) = delete;
    connection& operator=(connection&&) = delete;

    connection& operator*=(signal_t);
    connection& operator/=(signal_t);
    connection& operator=(signal_t);

    void mute    ( );
    void unmute  ( );
    void open    ( );
    void close   ( );

    protected:
    connection      ( node::pin& source, node::pin& dest, pattern ptn );
    void allocate   ( size_t size );
    void pull       ( size_t size );

    private:
    bool _feedback   = false;
    bool _active     = true;
    bool _muted      = false;
    bool _done       = false;
    signal_t _level  = 1;

    stream _stream;
    node::pin& _source;
    node::pin& _dest;
    matrix _matrix;
    pattern _pattern;
};

// ==========================================================================================
class graph
// ==========================================================================================
{
    public:
    friend class node;

    static connection&
    connect(node::pin& source, node::pin& dest,
            connection::pattern = connection::pattern::merge);

    static connection&
    connect(node& source, node& dest,
            connection::pattern = connection::pattern::merge);

    static connection&
    connect(node& source, node::pin& dest,
            connection::pattern = connection::pattern::merge);

    static connection&
    connect(node::pin& source, node& dest,
            connection::pattern = connection::pattern::merge);

    static void disconnect(node::pin& source, node::pin& dest);
    static void disconnect(node& source, node& dest);

    static void clear_connections(node& lhs, node& rhs);
    static int clear();

    static int run(node&, size_t = 1);

    static void
    configure(signal_t rate = 44100,
             size_t vector_size = 512,
             size_t vector_feedback_size = 64);

    private:
    static std::vector<connection*>
    _connections;

    static std::vector<node*>
    _nodes;

    static graph_properties
    _properties;
};

// ==========================================================================================
class sinetest : public node
// ==========================================================================================
{
    public:
    sinetest(signal_t f);

    void initialize(graph_properties properties) noexcept override;
    void rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept override;

    private:
    stream _wtable;
    size_t _phase;
    signal_t _frequency;
};
// ==========================================================================================
class vca : public node
// ==========================================================================================
{
    public:
    vca();
    void initialize(graph_properties properties) noexcept override;
    void rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept override;

    private:
    signal_t _amplitude = 1;
};

// ==========================================================================================
class rmodulator : public node
// ==========================================================================================
{
    public:
    void initialize(graph_properties properties) noexcept override;
    void rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept override;

    private:
    signal_t _frequency;
};
// ==========================================================================================
class dummy_output : public node
// ==========================================================================================
{
    public:
    void initialize(graph_properties properties) noexcept override;
    void rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept override;
};

}
}
