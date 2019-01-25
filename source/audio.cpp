#include "audio.hpp"

// ------------------------------------------------------------------------------------------
// NODE-PIN
using namespace wpn114::signal;
// ------------------------------------------------------------------------------------------

node::pin::pin(node &parent, polarity_t t, std::string_view label, size_t nchannels, bool def) :
    _parent(parent), _label(label), _default(def), _nchannels(nchannels),
    _polarity(t)
{
    _connections.reserve(15);
}

inline void
node::pin::allocate(size_t size)
{
    _stream.allocate(_nchannels, size);
}

void node::pin::add_connection(connection& con)
{
    _connections.push_back(&con);
}

void node::pin::remove_connection(connection& con)
{
    std::remove(_connections.begin(),
                _connections.end(),
                &con);
}

bool node::pin::connected()
{
    return _connections.size() > 0;
}

bool node::pin::connected(node& node)
{
    for ( const auto& con : _connections )
    {
        if  ( _polarity == input &&
             &con->_source._parent == &node )
             return true;

        else if ( _polarity == output &&
                  &con->_dest._parent == &node )
            return true;
    }

    return false;
}

bool node::pin::connected(node::pin& pin)
{
    for ( const auto& con : _connections )
    {
        if ( _polarity == input &&
            &con->_source == &pin )
            return true;

        else if ( _polarity == output &&
                  &con->_dest == &pin )
            return true;
    }
    return false;
}

stream::scoped_accessor& node::pool::operator[](std::string_view ref)
{
    for ( auto& strm : _streams )
        if ( strm.label == ref )
             return strm._stream;

    WPN_EXCEPT
}

// ------------------------------------------------------------------------------------------
// NODE
// ------------------------------------------------------------------------------------------

node::node()
{
    graph::_nodes.push_back(this);
}

WPN_UNIMPLEMENTED
bool node::connected(node const& other ) const noexcept
{
    return false;
}

inline node::pool
node::make_pools(std::vector<pin>& pinvec, size_t sz) noexcept
{
    node::pool pool;
    for ( auto& pin : pinvec )
        pool._streams.push_back({
                pin._stream.accessor(_pos, sz),
                pin._label
                });

    return pool;
}

void node::decl_input(std::string_view label, size_t nchannels, bool def)
{
    _uppins.emplace_back(*this, input, label, nchannels, def);
}

void node::decl_output(std::string_view label, size_t nchannels, bool def)
{
    _dnpins.emplace_back(*this, output, label, nchannels, def);
}

node::pin& node::inpin()
{
    for ( auto& pin : _uppins )
          if ( pin._default )
               return pin;

    WPN_EXCEPT
}

node::pin& node::inpin(size_t index)
{
    // return input pin at index
    return _uppins[index];
}

node::pin& node::inpin(std::string_view ref)
{
    for ( auto& pin: _uppins )
         if ( pin._label == ref )
              return pin;

    WPN_EXCEPT
}

node::pin& node::outpin()
{
    for ( auto& pin : _dnpins )
          if ( pin._default )
               return pin;

    WPN_EXCEPT
}

node::pin& node::outpin(size_t index)
{
    return _dnpins[index];
}

WPN_UNIMPLEMENTED
node::pin& node::outpin(std::string_view ref)
{
    for ( auto& pin: _dnpins )
         if ( pin._label == ref )
              return pin;

    WPN_EXCEPT
}

void node::configure(graph_properties properties) noexcept
{
    for ( auto& pin : _uppins )
          pin.allocate(properties.vsz);

    for ( auto& pin : _dnpins )
          pin.allocate(properties.vsz);

    initialize(properties);
}

void node::process() noexcept
{
    size_t sz = _intertwined ? _properties.vsz:
                               _properties.fvsz;

     // pull input connections
    for ( auto& pin : _uppins )
        for ( auto& con : pin._connections )
              con->pull(sz);

    // build input/output pools
    auto inpool  = make_pools(_uppins, sz);
    auto outpool = make_pools(_dnpins, sz);

    // process pools
    rwrite(inpool, outpool, sz);
    _pos += sz;
}

// ------------------------------------------------------------------------------------------
// CONNECTION
// ------------------------------------------------------------------------------------------

void connection::pull(size_t sz)
{
    if ( !_active )
         return;
    // if this is a feedback connection
    // we directly draw from source's stream
    // otherwise we call process until pin buffer is complete
    if ( !_feedback )
         while ( _source._parent._pos < sz )
                 _source._parent.process();

    if ( _muted )
    {
        WPN_UNIMPLEMENTED
        // do not draw & pour if muted, just increment position
        _stream.draw_skip(sz);
        _stream.pour_skip(sz);
    }

    else
    {
        // we draw from source's _stream
        // (streams have been synced at construct-time)
        auto s = _stream.draw(sz);
        s*= _level;

        WPN_UNIMPLEMENTED
        // then, we pour its stream into dest's
        // depending on matrix routing (todo)
        _stream.pour(sz);
    }
}

connection::connection(node::pin& source, node::pin& dest, pattern ptn) :
    _source(source), _dest(dest), _pattern(ptn)
{
    WPN_EXAMINE
    _stream.add_upsync( _source._stream);
    _stream.add_dnsync( _dest._stream);
}

void connection::allocate(size_t size)
{
    _stream.allocate(size);
}

void connection::mute()
{
    _muted = true;
}

void connection::unmute()
{
    _muted = false;
}

void connection::open()
{
    _active = true;
}

void connection::close()
{
    _active = false;
}

connection& connection::operator=(signal_t v)
{
    _level = v; return *this;
}

connection& connection::operator*=(signal_t v)
{
    _level *= v; return *this;
}

connection& connection::operator/=(signal_t v)
{
    _level /= v; return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// GRAPH
/////////////////////////////////////////////////////////////////////////////////////////////
std::vector<connection*> graph::_connections;
std::vector<node*> graph::_nodes;
graph_properties graph::_properties;

connection& graph::connect(node::pin& source, node::pin& dest,
                           connection::pattern ptn)
{    
    // verify that source is output and dest is input
    if ( source._polarity != output || dest._polarity != input )
    {
        // throw exception
        WPN_EXCEPT;
    }

    // connection's constructor will examine (silently)
    // if the connection pattern is correct or not
    connection* con = new connection(source, dest, ptn);

    _connections.push_back  ( con  );
    source.add_connection   ( *con );
    dest.add_connection     ( *con );

    if ( dest.connected( source ))
    {
        source._parent._intertwined  = true;
        dest._parent._intertwined    = true;
        con->_feedback               = true;
    }

    return *con;
}

connection& graph::connect(node& source, node& dest,
                           connection::pattern ptn)
{
    return graph::connect(source.outpin(), dest.inpin(), ptn);
}

void graph::disconnect(node::pin &source, node::pin &dest)
{
    connection* con = nullptr;

    for ( const auto& cn : _connections )
          if ( &cn->_source == &source &&
               &cn->_dest == &dest )
               con = cn;

    if ( con )
    {                
        source.remove_connection(*con);
        dest.remove_connection(*con);
    }

    WPN_EXCEPT // throw exception if connection not found
}

// convenience method for disconnecting default inputs/outputs
void graph::disconnect(node& source, node& dest)
{
    graph::disconnect(source.outpin(), dest.inpin());
}

void graph::configure(signal_t rate, size_t vector_size, size_t fb_size)
{
    _properties.rate    = rate;
    _properties.vsz     = vector_size;
    _properties.fvsz    = fb_size;

    _connections.reserve(150);
    _nodes.reserve(100);
}

int graph::run(node& node, size_t times)
{
    while ( times )
    {
        node.process();
        times--;
    }

    return 0;
}

WPN_UNIMPLEMENTED
int graph::clear()
{
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// SINETEST
sinetest::sinetest(signal_t f) : _frequency(f) {}
#define WPN_SINETEST_WT_SZ 16384
/////////////////////////////////////////////////////////////////////////////////////////////

void sinetest::initialize(graph_properties properties) noexcept
{
    // initialize stream
    _wtable.allocate(1, WPN_SINETEST_WT_SZ);
    auto acc = _wtable.accessor();

    // declare an input pin
    decl_input("FREQUENCY", 1, true);

    // declare an output pin
    decl_output("MAIN", 1, true);
}

void sinetest::rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept
{
    // fetch frequency input & main output stream
    auto freq   =  inputs  [ "FREQUENCY" ];
    auto out    =  outputs [ "MAIN" ];

    // we make a stack-copy of the freq stream
    // in order to make more efficient calculations
    stack_stream<> op(freq);

    if ( inpin().connected())
         op.accessor() *= _frequency;

    auto op_acc = op.accessor();

    // this will give us the incrementation positions
    // of the wavetable, depending on the frequency stream
    op_acc /= _properties.rate;
    op_acc *= _wtable.size();

    auto acc = _wtable.accessor(0, 0, _phase);
    out.lookup(acc, op_acc); // add interpolation mode to the arguments

    _phase += sz;
}

void vca::initialize(graph_properties properties) noexcept
{
    WPN_UNIMPLEMENTED
    // when nchannels is set to zero
    // that it is input-agnostic
    // this would be similar to multichannel expansion in sc
    // we can set the number of inputs/outputs at runtime
    decl_input( "MAIN", 0, true );
    decl_input( "GAIN", 1 );

    decl_output( "MAIN", 0, true );
}

vca::vca() {}

void vca::rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept
{
    auto in     = inputs["MAIN"];
    auto gain   = inputs["GAIN"];
    auto out    = outputs["MAIN"];

    if (inpin().connected())
    {
         gain *= _amplitude;
         in   *= gain;
         out   = in;
    }
    else out = in* _amplitude;
}

void rmodulator::initialize(graph_properties properties) noexcept
{
    // input declaration ----------
    decl_input( "MAIN", 0, true );
    decl_input( "FREQUENCY", 1  );
    decl_input( "WAVEFORM", 1   );

    // output declaration ---------
    decl_output( "MAIN", 0, true );
}

void rmodulator::rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept
{
    auto in  = inputs  [ "MAIN" ];
    auto out = outputs [ "MAIN" ];
}

void dummy_output::initialize(graph_properties properties) noexcept
{
    decl_input("MAIN", 1, true);
}

void dummy_output::rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept
{
    auto in = inputs["MAIN"];

    for ( size_t n = 0; n < sz; ++n )
          std::cout << in[0][n] << std::endl;
}
