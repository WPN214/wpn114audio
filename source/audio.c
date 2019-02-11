#include "audio.h"
#include <string.h>
#include <assert.h>
//-------------------------------------------------------------------------------------------------
#define RTFM "READ_THE_FUCKING_MANUAL"
//-------------------------------------------------------------------------------------------------
#define WPN_STRUCT_INITIALIZE(_t, _v) _t _v =
//-------------------------------------------------------------------------------------------------
#define WPN_TODO
#define WPN_REFACTOR
#define WPN_OPTIMIZE
#define WPN_DEPRECATED
#define WPN_UNSAFE
#define WPN_EXAMINE
#define WPN_REVISE
#define WPN_OK
//-------------------------------------------------------------------------------------------------
#define FOREACH_CHANNEL_SAMPLE(_c, _n) for(usize _n = 0; _n < _c->size; ++_n)
typedef wpn_channel_accessor wpn_ch_acc;
typedef wpn_stream_accessor wpn_st_acc;
//-------------------------------------------------------------------------------------------------
static inline sgn_t
rms(sgn_t* dt, usize sz)
{
    sgn_t  mean = 0;
    for (  usize n = 0; n < sz; ++n )
           mean += dt[n];
    return sqrt(1.0/sz*mean);
}
//=================================================================================================
// CHANNELS
//=================================================================================================
// NOTE: about channel and stream allocation
// they both have flexible array members
// meaning they're actually not simple pointer carriers
// so they should not be copied/assigned in a real-time context
// the accessors are designed to do this
//=================================================================================================

WPN_OK wpn_channel*
wpn_channel_allocate(usize n, usize size, sgn_t rate)
{
    wpn_channel* ch = ( wpn_channel* )
        malloc((sizeof( wpn_channel )+
                sizeof( sgn_t ))*n);

    ch->rate = rate;
    ch->size = size;
    return ch;
}

// ----------------------------------------------------------------------
// ** CHANNEL-ACCESSOR
// ----------------------------------------------------------------------
// ** similar to slices in modern languages
// ** they represent a 'locked' portion of a channel
// ** that can be manipulated
// ----------------------------------------------------------------------

WPN_OK wpn_channel_accessor
wpn_channel_access(wpn_channel* channel, usize begin, usize size, usize pos)
{
    WPN_STRUCT_INITIALIZE
    ( wpn_ch_acc, acc )
    {
        .parent  = channel,
        .begin   = channel->data+begin,
        .end     = channel->data+begin+size,
        .pos     = channel->data+begin+pos
    };

    return acc;
}

// ----------------------------------------------------------------------
// ** CHANNEL-DRAIN: sets the whole slice to zero
// ** CHANNEL-VFILL: fills the whole slice with value 'v'
// ----------------------------------------------------------------------

WPN_OK void
wpn_channel_drain(wpn_ch_acc* acc )
{
    memset(acc->begin, 0, acc->size);
}

WPN_OK void
wpn_channel_vfill(wpn_ch_acc* acc, sgn_t v)
{
    FOREACH_CHANNEL_SAMPLE(acc, s)
            acc->begin[s] = v;
}
// ----------------------------------------------------------------------
// ** CHANNEL-COPY-MERGE
// ----------------------------------------------------------------------
// source's contents are merged into dest's ( a >> b )
// /!\ they are not replaced !!
// ----------------------------------------------------------------------
WPN_EXAMINE void
wpn_channel_cpmg(wpn_ch_acc* source, wpn_ch_acc* dest)
{
    usize index = source->size < dest->size ?
                  source->size : dest->size;

    while ( index-- )
            dest->begin[index] +=
            source->begin[index];
}
// ----------------------------------------------------------------------
// ** CHANNEL-POUR
// ----------------------------------------------------------------------
// source's contents are merged into dest's ( a >>= b )
// and then drained
// ----------------------------------------------------------------------
WPN_OK void
wpn_channel_pour(wpn_ch_acc* source, wpn_ch_acc* dest)
{
    wpn_channel_cpmg(source, dest);
    wpn_channel_drain(dest);
}
//-------------------------------------------------------------------------------------------------
// CHANNEL OPERATIONS & ANALYSIS
//-------------------------------------------------------------------------------------------------
#define FOREACH_SAMPLE_OPERATOR(_c, _s, _v)                                                       \
        FOREACH_CHANNEL_SAMPLE(_c, _n) _c->begin[_n] _s _v;
//-------------------------------------------------------------------------------------------------
void // +=
wpn_channel_dcadd(wpn_ch_acc* acc, sgn_t offset)
{
    FOREACH_SAMPLE_OPERATOR
    ( acc, +=, offset );
}

void // -=
wpn_channel_dcrem(wpn_ch_acc* acc, sgn_t offset)
{
    FOREACH_SAMPLE_OPERATOR
    ( acc, -=, offset );
}

void // *=
wpn_channel_mul(wpn_ch_acc* acc, sgn_t factor)
{
    FOREACH_SAMPLE_OPERATOR
    ( acc, *=, factor );
}

void // /=
wpn_channel_div(wpn_ch_acc* acc, sgn_t factor)
{
    FOREACH_SAMPLE_OPERATOR
    ( acc, /=, factor );
}

//-------------------------------------------------------------------------------------------------

WPN_OK sgn_t
wpn_channel_min(wpn_ch_acc* acc)
{
    sgn_t  m = 0;
    FOREACH_CHANNEL_SAMPLE(acc, n)
           m = wpnmin(m, acc->begin[n]);
    return m;
}

WPN_OK sgn_t
wpn_channel_max(wpn_ch_acc* acc)
{
    sgn_t  m = 0;
    FOREACH_CHANNEL_SAMPLE(acc, n)
           m = wpnmax(m, acc->begin[n]);
    return m;
}

WPN_OK sgn_t
wpn_channel_rms(wpn_ch_acc* acc, enum level_t t)
{
    if ( t == linear )
         return rms(acc->begin, acc->size);
    else return ATODB(rms(acc->begin, acc->size));
}

// ================================================================================================
//
// STREAMS
//
// ================================================================================================

WPN_EXAMINE wpn_stream* // direct construction & allocation
wpn_stream_allocate(u8 nchannels, usize size)
{
    // yes.. i know..
    // but otherwise we would have to interleave it..
    // or having to do multiple frees
    wpn_stream* stream =  (wpn_stream*)
            malloc( sizeof(wpn_stream) + nchannels*
                  ( sizeof(wpn_channel)+ sizeof(sgn_t)*size));

    stream->size = size;
    stream->nchannels = nchannels;
    return stream;
}

wpn_stream_accessor
wpn_stream_access(wpn_stream* stream, usize begin, usize size, usize pos)
{

    WPN_STRUCT_INITIALIZE
    ( wpn_st_acc, acc ) {
        .parent   = stream,
        .begin    = begin,
        .end      = begin+size,
        .size     = size,
        .pos      = pos
    };

    return acc;
}

// ------------------------------------------------------------------------------------------
static __always_inline wpn_channel_accessor
wpn_stream_at(wpn_stream_accessor* acc, usize index)
{
    return wpn_channel_access(&acc->parent->channels[index],
                               acc->begin, acc->size, acc->pos);
}

// ------------------------------------------------------------------------------------------
#define FOREACH_STREAM_CHANNEL(_acc, _c)                                                    \
        for (usize _c = 0; _c < _acc->parent->nchannels; ++_c)
// ------------------------------------------------------------------------------------------

void
wpn_stream_drain(wpn_st_acc* s_acc)
{
    FOREACH_STREAM_CHANNEL (s_acc, c) {
        wpn_ch_acc c_acc = wpn_stream_at(s_acc, c);
        wpn_channel_drain(&c_acc);
    }
}

void
wpn_stream_vfill(wpn_st_acc* s_acc, sgn_t v)
{
    FOREACH_STREAM_CHANNEL (s_acc, c) {
        wpn_ch_acc c_acc = wpn_stream_at(s_acc, c);
        wpn_channel_vfill(&c_acc, v);
    }
}

WPN_UNSAFE void
wpn_stream_cpmg(wpn_st_acc* source, wpn_st_acc* dest)
{
    FOREACH_STREAM_CHANNEL (source, c) {
        wpn_ch_acc s_acc = wpn_stream_at(source, c);
        wpn_ch_acc d_acc = wpn_stream_at(dest, c);
        wpn_channel_cpmg(&s_acc, &d_acc);
    }
}

WPN_OK void
wpn_stream_pour(wpn_st_acc* source, wpn_st_acc* dest)
{
    wpn_stream_cpmg(source, dest);
    wpn_stream_drain(dest);
}

// ------------------------------------------------------------------------------------------
// STREAM OPERATIONS
// ------------------------------------------------------------------------------------------
void
wpn_stream_dcadd(wpn_st_acc* s_acc, sgn_t offset)
{
    FOREACH_STREAM_CHANNEL (s_acc, c) {
        wpn_ch_acc c_acc = wpn_stream_at(s_acc, c);
        wpn_channel_dcadd(&c_acc, offset);
    }
}

void
wpn_stream_dcrem(wpn_st_acc* s_acc, sgn_t offset)
{
    FOREACH_STREAM_CHANNEL (s_acc, c) {
        wpn_ch_acc c_acc = wpn_stream_at(s_acc, c);
        wpn_channel_dcrem(&c_acc, offset);
    }
}

void
wpn_stream_mul(wpn_st_acc* s_acc, sgn_t ratio)
{
    FOREACH_STREAM_CHANNEL (s_acc, c) {
        wpn_ch_acc c_acc = wpn_stream_at(s_acc, c);
        wpn_channel_mul(&c_acc, ratio);
    }
}

void
wpn_stream_div(wpn_st_acc* s_acc, sgn_t factor)
{
    FOREACH_STREAM_CHANNEL (s_acc, c) {
        wpn_ch_acc c_acc = wpn_stream_at(s_acc, c);
        wpn_channel_dcadd(&c_acc, factor);
    }
}

// ------------------------------------------------------------------------------------------
// ANALYSIS
// ------------------------------------------------------------------------------------------

sgn_t
wpn_stream_min(wpn_st_acc* s_acc)
{
    sgn_t m = 0;

    FOREACH_STREAM_CHANNEL (s_acc, c) {
        wpn_ch_acc c_acc = wpn_stream_at(s_acc, c);
        m = wpnmin(wpn_channel_min(&c_acc), m);
    }

    return m;
}

sgn_t
wpn_stream_max(wpn_st_acc* s_acc)
{
    sgn_t m = 0;

    FOREACH_STREAM_CHANNEL (s_acc, c) {
        wpn_ch_acc c_acc = wpn_stream_at(s_acc, c);
        m = wpnmax(wpn_channel_max(&c_acc), m);
    }

    return m;
}

WPN_TODO sgn_t
wpn_stream_rms(wpn_st_acc* acc, enum level_t t)
{

}

//-------------------------------------------------------------------------------------------------
// SYNCS
//-------------------------------------------------------------------------------------------------
// a synchronized ring-buffer-like implementation
//
// up-stream <- connection-stream -> down-stream
//   [512]           [512]              [512]

// connection [draws] from upstream, lets say 32 samples
// upsync is now at position 32
// it returns a stream accessor to CONNECTION's stream
// connection manipulates this slice
// and pours it downstream at position [0..31]
// dnstream is not at position 32
// ..
// wrap it around stream size (512 here)
//-------------------------------------------------------------------------------------------------

static __always_inline void
wpn_sync_wrap(usize* dest, usize limit)
{
    if ( *dest == limit )
         *dest = 0;
}

WPN_OK inline static void
wpn_sync_operate(wpn_stream* source, wpn_stream* dest,
                 const usize pos, const usize sz)
{
    wpn_st_acc upstream = wpn_stream_access(source, pos, sz, 0);
    wpn_st_acc dnstream = wpn_stream_access(dest, pos, sz, 0);
    wpn_stream_cpmg(&upstream, &dnstream);
}

WPN_OK static wpn_stream_accessor
wpn_sync_draw(wpn_stream* stream, usize sz)
{
    usize* pos = &stream->upsync.pos;

    wpn_sync_operate(stream->upsync.stream, stream, *pos, sz);
    wpn_sync_wrap(pos, stream->size);
}

WPN_OK static void
wpn_sync_pour(wpn_stream* stream, usize sz)
{
    usize* pos = &stream->dnsync.pos;

    wpn_sync_operate(stream, stream->dnsync.stream, *pos, sz);
    wpn_sync_wrap(pos, stream->size);
}

// in case no data is actually fetched
// we still have to move the cursor ahead
WPN_OK inline static void
wpn_sync_skip(wpn_stream* stream, usize sz)
{
    if ( stream->upsync.pos+sz == stream->size)
         stream->upsync.pos = stream->dnsync.pos = 0;
}

//==================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAPH
////////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================
// first, we create & initialize a graph
//--------------------------------------------------------------------------------------------------
WPN_OK wpn_graph
wpn_graph_create(sgn_t rate, u16 vector_size, u16 feedback_vector_size)
{
    wpn_graph graph;

    graph.properties.rate       = rate;
    graph.properties.vecsz      = vector_size;
    graph.properties.vecsz_fb   = feedback_vector_size;

    graph.nnodes        = 0;
    graph.nconnections  = 0;

    graph.connections = (wpn_connection*)
            calloc(sizeof(wpn_connection), WPN_GRAPH_MAX_CONNECTIONS);

    graph.nodes = (wpn_node*) calloc(sizeof(wpn_node), WPN_GRAPH_MAX_NODES);
}
//--------------------------------------------------------------------------------------------------
// we then create our processor structures, and register them to the graph
//--------------------------------------------------------------------------------------------------
WPN_OK void
wpn_graph_register(wpn_graph* graph, void* udata,
                   udecl_fn dcl_fn, uconfig_fn cf_fn, uprocess_fn rw_fn)
{
    assert(graph->nnodes < WPN_GRAPH_MAX_NODES);

    // pop a node from graph's 'node stack'
    wpn_node* node = &graph->nodes[graph->nnodes];

    // construct node
    node->udata = udata;
    node->udecl_fn = dcl_fn;
    node->uconfig_fn = cf_fn;
    node->uprocess_fn = rw_fn;

    // if pin declaration function is valid, call it
    if ( dcl_fn ) node->udecl_fn(node);

    // configure/allocate node
    wpn_node_configure(node, graph->properties);
}

WPN_OK static __always_inline void
wpn_pin_allocate(wpn_pin* pin, u16 sz)
{
    pin->stream = wpn_stream_allocate(pin->nchannels, sz);
}

WPN_OK static void
wpn_node_configure(wpn_node* node, gproperties properties)
{
    node->properties = properties;

    // allocate node's pin streams
    for ( u16 n = 0; n < node->uppins.sz; ++n )
          wpn_pin_allocate(&node->uppins.data[n],
                           properties.vecsz);

    // call the configure callback
    if ( node->uconfig_fn )
         node->uconfig_fn(properties.rate,
                          properties.vecsz,
                          node->udata);
}

//------------------------------------------------------------------------
// while registering, graph will call our 'declare' callback
// so we can create and register input/output pins
//------------------------------------------------------------------------
WPN_OK void
wpn_pindecl(wpn_node* node, enum polarity_t polarity, const char* label,
            u8 nchannels, bool default_)
{
    wpn_pin_vector* pvec = wpn_node_get_pin_vector(node, polarity);
    wpn_pin* pin = &pvec->data[pvec->sz];

    pin->index = pvec->sz;
    pvec->sz++;

    pin->parent     = node;
    pin->label      = label;
    pin->default_   = default_;
    pin->nchannels  = nchannels;
    pin->polarity   = polarity;
}

//------------------------------------------------------------------------
// WPN_GET_PIN_VECTOR
// --- returns input/output pin vector of a node
//------------------------------------------------------------------------

WPN_OK static inline wpn_pin_vector*
wpn_node_get_pin_vector(wpn_node* node, enum polarity_t polarity)
{
    switch(polarity)
    {
        case input:  return &node->uppins;
        case output: return &node->dnpins;
    }
}

//------------------------------------------------------------------------
// WPN_DEFAULT_PIN
// --- returns input/output default pin of a node
//------------------------------------------------------------------------

WPN_OK wpn_pin*
wpn_default_pin(wpn_node* node, enum polarity_t polarity)
{
    wpn_pin_vector* pvector =
            wpn_node_get_pin_vector(node, polarity);

    for ( usize n = 0; n < pvector->sz; ++n )
        if ( pvector->data[n].default_ )
             return &pvector->data[n];

    return nullptr_t;
}

//==========================================================================================
//
// CONNECTIONS
//
//==========================================================================================

WPN_OK static inline wpn_node*
wpn_graph_lookup(wpn_graph* graph, void* unit)
{
    for ( usize n = 0; n < graph->nnodes; ++n )
        if ( graph->nodes[n].udata == unit )
             return &graph->nodes[n];

    return nullptr_t;
}


WPN_OK static inline bool
wpn_pin_pconnected(wpn_pin* source, wpn_pin* dest)
{
    for ( usize n = 0; n < source->nconnections; ++n )
          if( source->connections[n]->dest == dest )
              return true;

    return false;
}

WPN_OK static inline bool
wpn_pin_nconnected(wpn_pin* source, wpn_node* dest)
{
    for ( usize n = 0; n < dest->uppins.sz; ++n )
        if ( wpn_pin_pconnected(source, &dest->uppins.data[n]))
             return true;

    return false;
}

WPN_OK static bool
wpn_node_connected(wpn_node* source, wpn_node* dest)
{
    wpn_pin_vector* pvec = wpn_node_get_pin_vector(source, output);

    for ( usize n = 0; n < pvec->sz; ++n )
        if ( wpn_pin_nconnected(&pvec->data[n], dest))
             return true;

    return false;
}

//-------------------------------------------------------------------------------------------------

WPN_OK wpn_connection*
wpn_graph_pconnect(wpn_graph* graph, wpn_pin* source, wpn_pin* dest)
{
    assert((source->polarity == output) && (dest->polarity == input));

    // pop a connection from the 'stack' and build it
    wpn_connection* con = &graph->connections[graph->nconnections];

    con->source  = source;
    con->dest    = dest;
    con->level   = 1;
    con->muted   = false;
    con->active  = true;

    // allocate connection's stream
    u8 nchannels = wpnmax(source->nchannels, dest->nchannels);
    con->stream = wpn_stream_allocate(nchannels, graph->properties.vecsz);

    // check for a feedback connection
    con->feedback = wpn_node_connected(source->parent, dest->parent);;

    graph->nconnections++;
    // add connection references in source & dest
    source->connections[source->nconnections] = con;
    source->nconnections++;

    dest->connections[dest->nconnections] = con;
    dest->nconnections++;

    return con;
}

WPN_OK wpn_connection*
wpn_graph_npconnect(wpn_graph* graph, wpn_node* source, wpn_pin* dest)
{
    wpn_pin* spin = wpn_default_pin(source, output);
    return wpn_graph_pconnect(graph, spin, dest);

}

WPN_OK wpn_connection*
wpn_graph_pnconnect(wpn_graph* graph, wpn_pin* source, wpn_node* dest)
{
    wpn_pin* dpin = wpn_default_pin(dest, input);
    return wpn_graph_pconnect(graph, source, dpin);

}

WPN_OK wpn_connection*
wpn_graph_nconnect(wpn_graph *graph, wpn_node* source, wpn_node* dest)
{
    // get default pins for each node
    // and build connection
    wpn_pin* spin = wpn_default_pin(source, output);
    wpn_pin* dpin = wpn_default_pin(dest, input);

    return wpn_graph_pconnect(graph, spin, dpin);
}

//=================================================================================================
//
// GRAPH_RUN
//
//=================================================================================================

WPN_OK int
wpn_graph_run_exit(wpn_graph* graph, void* unit, usize ntimes)
{
    wpn_node* target = wpn_graph_lookup(graph, unit);

    if ( !target )
         return -1;

    while ( ntimes )
    {
        wpn_node_process(target);
        --ntimes;
    }

    return ntimes;
}

WPN_OK static inline void
wpn_node_make_pools(wpn_pool* poolref, wpn_pin* target, usize pos, usize sz)
{
    for ( u16 n = 0; n < poolref->nstreams; ++n )
    {
        wpn_pin_stream pstr = {
            wpn_stream_access(target[n].stream, pos, sz, 0),
            target[n].label
        };

        poolref->streams[n] = pstr;
    }
}

WPN_OK void
wpn_node_process(wpn_node* node)
{
    if ( node->uprocess_fn == nullptr_t )
         return;

    // we usually use a smaller vector size
    // in case of feedback connections, as to keep
    // the implicit delay to a minimum, without putting
    // too much stress on the processed graph
    usize sz = node->intertwined ?
               node->properties.vecsz :
               node->properties.vecsz_fb;

    // pull each connection
    for ( usize n = 0; n < node->uppins.sz; ++n )
    {
        wpn_pin* pin = &node->uppins.data[n];
        for ( usize c = 0; c < pin->nconnections; ++c )
              wpn_connection_pull(pin->connections[c], sz);
    }

    // build input/output slice pools
    wpn_pool uppool, dnpool;
    uppool.nstreams = node->uppins.sz;
    dnpool.nstreams = node->dnpins.sz;

    wpn_node_make_pools(&uppool, node->uppins.data, node->pos, sz);
    wpn_node_make_pools(&dnpool, node->dnpins.data, node->pos, sz);

    // call the processing callback function
    node->uprocess_fn(&uppool, &dnpool, node->udata);
    node->pos += sz;
}

WPN_OK void
wpn_connection_pull(wpn_connection* con, usize sz)
{
    if ( !con->active )
         return;

    // if this is a feedback connection
    // we directly draw from source's stream
    // otherwise we call process until pin buffer is sufficiently filled
    if ( con->feedback )
         while ( con->source->parent->pos < sz )
                 wpn_node_process(con->source->parent);

    if ( !con->muted )
    {
        // draw from upsync,
        // get a stream accessor to the fetched data,
        // and pour downstream
        wpn_st_acc acc = wpn_sync_draw(con->stream, sz);
        wpn_stream_mul ( &acc, con->level );
        wpn_sync_pour  ( con->stream, sz );
    }

    // if connection is muted, do not draw/pour
    // just increment position
    else wpn_sync_skip(con->stream, sz);
}

// ------------------------------------------------------------------------------------------------
//
// STREAM EXTRACTORS
//
// ------------------------------------------------------------------------------------------------

WPN_OK wpn_st_acc* // from string
wpn_streamext_l(wpn_pool* pool, const char* pin)
{
    for ( usize n = 0; n < pool->nstreams; ++n )
    {
        wpn_pin_stream* pstream = &pool->streams[n];
        if ( !strcmp(pstream->label, pin))
             return &pstream->accessor;
    }
    return nullptr_t;
}

WPN_REVISE wpn_st_acc* // from index
wpn_streamext_i(wpn_pool* pool, u16 index)
{
    for ( usize n = 0; n < pool->nstreams; ++n )
        if ( n == index )
             return &pool->streams[n].accessor;

    return nullptr_t;
}

// ==========================================================================================
// ------------------------------------------------------------------------------------------
// CRASH_TEST_FOR_DUMMIES
// ------------------------------------------------------------------------------------------/
// ==========================================================================================

WPN_OK void
sinetest_dcl(wpn_node* node)
{
    wpn_pindecl(node, input, "FREQUENCY", 1, true);
    wpn_pindecl(node, output, "MAIN", 1, true);
}

WPN_OK void
sinetest_cf(sgn_t rate, u16 size, void* udata)
{
    // build wtable
    sinetest* sine = (sinetest*)udata;
    sine->frequency = 440;
    sine->srate = rate;
    sine->phase = 0;

    sgn_t* wt = sine->wtable;

    for ( u16 i = 0; i < 16384; ++i )
          wt[i] = sin( i/16384*M_PI*2 );
}

void
sinetest_rw(wpn_pool* uppool, wpn_pool* dnpool, void* udata)
{
    sinetest* sine = (sinetest*) udata;

    // we get output pin's stream
    wpn_st_acc* freq = wpn_streamext_l(uppool, "FREQUENCY");
    wpn_st_acc* out  = wpn_streamext_l(dnpool, "OUTPUT");

}
