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
class connection;
struct graph_properties
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
        pin ( node& parent, std::string label, size_t nchannels, bool def );

        bool connected();
        bool connected(node&);
        bool connected(node::pin&);

        protected:
        void allocate(size_t size);

        private:
        node& _parent;
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
        std::string label;
    };

    //---------------------------------------------------------------------------------------
    struct pool
    // a stream vector, for each pin
    //---------------------------------------------------------------------------------------
    {
        stream::scoped_accessor& operator[](std::string);
        std::vector<pinstream_ref> _streams;
    };

    pin& inpin();
    pin& inpin(size_t index);
    pin& inpin(std::string ref);

    pin& outpin();
    pin& outpin(size_t index);
    pin& outpin(std::string ref);

    //---------------------------------------------------------------------------------------
    protected:
    graph_properties _properties;

    void decl_input     ( std::string label, size_t nchannels, bool def );
    void decl_output    ( std::string label, size_t nchannels, bool def );

    virtual void initialize  ( graph_properties properties ) noexcept = 0;
    virtual void rwrite      ( node::pool& in, node::pool& out, size_t sz ) noexcept = 0;

    private:
    pool make_pools  ( std::vector<pin>& pinvector, size_t sz ) noexcept;
    void configure   ( graph_properties properties ) noexcept;
    void process     ( ) noexcept;

    bool connected    ( node const& other ) const noexcept;

    size_t _pos;
    bool _intertwined = false;
    std::vector<pin> _uppins;
    std::vector<pin> _dnpins;


};// node
// ==========================================================================================
class connection
// ==========================================================================================
{
    friend class node;
    friend class graph;

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
    connection      ( node::pin& source, node::pin& dest, matrix mtx );
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
};

// ==========================================================================================
class graph
// ==========================================================================================
{
    public:
    friend class node;

    static void initialize();

    static connection&
    connect(node::pin& source,
            node::pin& dest,
            connection::matrix mtx = {});

    static connection&
    connect(node& source,
            node& dest,
            connection::matrix mtx = {});

    static void disconnect(node::pin& source,
                           node::pin& dest);

    static void
    configure(signal_t rate,
             size_t vector_size,
             size_t vector_feedback_size);

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
    sinetest();

    void initialize(graph_properties properties) noexcept override;
    void rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept override;

    private:
    stream _wtable;
    size_t _phase;
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

}
}
