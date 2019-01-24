#include "audio.hpp"

// ------------------------------------------------------------------------------------------
// NODE-PIN
using namespace wpn114::signal;
// ------------------------------------------------------------------------------------------

node::pin::pin(node &parent, std::string label, size_t nchannels, bool def) :
    _parent(parent), _label(label), _default(def), _nchannels(nchannels)
{
    _connections.reserve(15);
}

inline void
node::pin::allocate(size_t size)
{
    _stream.allocate(_nchannels, size);
}

stream::scoped_accessor& node::pool::operator[](std::string ref)
{
    for ( auto& strm : _streams )
        if ( strm.label == ref )
             return strm._stream;
}

// ------------------------------------------------------------------------------------------
// NODE
// ------------------------------------------------------------------------------------------

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

void node::decl_input(std::string label, size_t nchannels, bool def)
{
    _uppins.emplace_back(*this, label, nchannels, def);
}

void node::decl_output(std::string label, size_t nchannels, bool def)
{
    _dnpins.emplace_back(*this, label, nchannels, def);
}

WPN_UNIMPLEMENTED
node::pin& node::inpin()
{
    // return default input pin
}

node::pin& node::inpin(size_t index)
{
    // return input pin at index
    return _uppins[index];
}

WPN_UNIMPLEMENTED
node::pin& node::inpin(std::string ref)
{

}

WPN_UNIMPLEMENTED
node::pin& node::outpin()
{

}

node::pin& node::outpin(size_t index)
{
    return _dnpins[index];
}

WPN_UNIMPLEMENTED
node::pin& node::outpin(std::string ref)
{

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

    // we draw from source's _stream
    // (streams have been synced at construct-time)
    WPN_OPTIMIZE // do not draw & pour if muted, just increment position
    auto s = _stream.draw(sz);

    if ( _muted )
         s.drain();
    else s *= _level;

    // then, we pour its stream into dest's
    // depending on matrix routing (todo)
    _stream.pour(sz);
}

connection::connection(node::pin& source, node::pin& dest, matrix mtx) :
    _source(source), _dest(dest), _matrix(mtx)
{
    _stream.add_upsync(_source._stream);
    _stream.add_dnsync(_dest._stream);
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

void graph::initialize()
{
    _connections.reserve(150);
    _nodes.reserve(100);
}

connection& graph::connect(node::pin& source, node::pin& dest, connection::matrix mtx)
{
    connection* con = new connection(source, dest, mtx);

    _connections.push_back          ( con );
    source._connections.push_back   ( con );
    dest._connections.push_back     ( con );

    WPN_UNIMPLEMENTED
    // parse matrix

    if ( dest._parent.connected(source._parent))
    {
        dest._parent._intertwined    = true;
        source._parent._intertwined  = true;
        con->_feedback               = true;
    }
    return *con;
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
        std::remove(_connections.begin(), _connections.end(), con);
        std::remove(source._connections.begin(), source._connections.end(), con);
        std::remove(dest._connections.begin(), dest._connections.end(), con);
    }

    WPN_EXCEPT // throw exception if connection not found
}

void graph::configure(signal_t rate, size_t vector_size, size_t fb_size)
{
    _properties.rate    = rate;
    _properties.vsz     = vector_size;
    _properties.fvsz    = fb_size;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// SINETEST
sinetest::sinetest() {}
/////////////////////////////////////////////////////////////////////////////////////////////

void sinetest::initialize(graph_properties properties) noexcept
{
    // initialize stream
    _wtable.allocate(1, 16384);
    auto acc = _wtable.accessor();

    // declare an input pin
    decl_input("FREQUENCY", 1, true);

    // declare an output pin
    decl_output("MAIN", 1, true); // ok
}

void sinetest::rwrite(node::pool& inputs, node::pool& outputs, size_t sz) noexcept
{
    // fetch frequency input & main output stream
    auto freq   =  inputs  [ "FREQUENCY" ];
    auto out    =  outputs [ "MAIN" ];

    // we make a stack-copy of the freq stream
    // in order to make more efficient calculations
    stack_stream<> op(freq);
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
    decl_input("MAIN", 1, true);
    decl_input("GAIN", 1, false);

    decl_output("MAIN", 1, true);
}

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
