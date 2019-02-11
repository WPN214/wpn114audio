#pragma once
//-------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
//-------------------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//-------------------------------------------------------------------------------------------------
// UNSIGNED TYPES
//-------------------------------------------------------------------------------------------------
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
//-------------------------------------------------------------------------------------------------
// SIGNED TYPES
//-------------------------------------------------------------------------------------------------
typedef char i8;
typedef short i16;
typedef int i32;
typedef long i64;
//-------------------------------------------------------------------------------------------------
// FLOATING-POINT
//-------------------------------------------------------------------------------------------------
typedef float f32;
typedef double f64;
//-------------------------------------------------------------------------------------------------
#define nullptr_t NULL
//-------------------------------------------------------------------------------------------------
#ifndef WPN_USIZE
#define WPN_USIZE uint32_t
#endif
//-------------------------------------------------------------------------------------------------
#ifndef WPN_COMPILE_FLOATING_POINT_PRECISION
#define WPN_COMPILE_FLOATING_POINT_PRECISION double
#endif
//-------------------------------------------------------------------------------------------------
#ifndef WPN_GRAPH_MAX_NODES
#define WPN_GRAPH_MAX_NODES 100u
#endif
#ifndef WPN_GRAPH_MAX_CONNECTIONS
#define WPN_GRAPH_MAX_CONNECTIONS 255u
#endif
//-------------------------------------------------------------------------------------------------
#ifndef WPN_COMPILE_CHANNEL_STACK_SIZE
#define WPN_COMPILE_CHANNEL_STACK_SIZE 512u
#endif
//-------------------------------------------------------------------------------------------------
#ifndef WPN_COMPILE_STREAM_STACK_SIZE
#define WPN_COMPILE_STREAM_STACK_SIZE 64u
#endif
//-------------------------------------------------------------------------------------------------
#ifndef WPN_COMPILE_NODE_STACK_SIZE
#define WPN_COMPILE_NODE_STACK_SIZE 20u
#endif
//-------------------------------------------------------------------------------------------------
#ifndef WPN_COMPILE_PIN_STACK_SIZE
#define WPN_COMPILE_PIN_STACK_SIZE 64u
#endif
//-------------------------------------------------------------------------------------------------
typedef WPN_COMPILE_FLOATING_POINT_PRECISION sgn_t;
typedef WPN_USIZE usize;
//-------------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
//-------------------------------------------------------------------------------------------------
#define wpnmin(a, b) ((a) < (b) ? (a) : (b))
#define wpnmax(a, b) ((a) > (b) ? (a) : (b))
//-------------------------------------------------------------------------------------------------
#define atodb(_s) log10(_s)*20
#define dbtoa(_s) pow(10, _s*.05)
#define lininterp(_x, _a, _b) a+x*(b-a)
static sgn_t rms(sgn_t *dt, usize sz);
//-------------------------------------------------------------------------------------------------
// ENUM_TYPES
//-------------------------------------------------------------------------------------------------
enum level_t     { linear = 0, dbfs = 1  };
enum polarity_t  { input = 0, output = 1 };
//-------------------------------------------------------------------------------------------------
// TYPEDEFS
//-------------------------------------------------------------------------------------------------
// -- WPN_CHANNEL, a single-vector signal container
// -- WPN_CHANNEL_ACCESSOR: represents a slice/chunk of the channel

// -- WPN_STREAM: a multi-vector signal container
// -- WPN_STREAM_ACCESSOR: a slice/chunk of the stream

typedef enum polarity_t polarity_t;
typedef enum level_t level_t;

typedef struct wpn_channel wpn_channel;
typedef struct wpn_stack_channel wpn_stack_channel;
typedef struct wpn_channel_accessor wpn_channel_accessor;

typedef struct wpn_stream wpn_stream;
typedef struct wpn_stack_stream wpn_stack_stream;
typedef struct wpn_stream_accessor wpn_stream_accessor;
typedef struct wpn_sync wpn_sync;

//-------------------------------------------------------------------------------------------------
struct wpn_channel // allocated on the heap
//-------------------------------------------------------------------------------------------------
{
    usize size;
    sgn_t rate;
    sgn_t data[];
};
//-------------------------------------------------------------------------------------------------
struct wpn_stack_channel // allocated on the stack
//-------------------------------------------------------------------------------------------------
{
    usize size;
    sgn_t rate;
    sgn_t data[WPN_COMPILE_CHANNEL_STACK_SIZE];
};

//-------------------------------------------------------------------------------------------------
struct wpn_channel_accessor
//-------------------------------------------------------------------------------------------------
{
    wpn_channel* parent;
    sgn_t* begin;
    sgn_t* end;
    usize size;
    sgn_t* pos;
};

wpn_channel *
wpn_channel_allocate        ( usize n, usize size, sgn_t rate);

// slices & operations:
wpn_channel_accessor
wpn_channel_access          ( wpn_channel* channel, usize begin, usize size, usize pos);

wpn_channel_accessor
wpn_stack_channel_access    ( wpn_stack_channel* channel, usize begin, usize size, usize pos);

void wpn_channel_drain      ( wpn_channel_accessor* accessor);
void wpn_channel_vfill      ( wpn_channel_accessor* accessor, sgn_t v);
void wpn_channel_cpmg       ( wpn_channel_accessor* lhs, wpn_channel_accessor* rhs);
void wpn_channel_pour       ( wpn_channel_accessor* lhs, wpn_channel_accessor* rhs);

void wpn_channel_crem       ( wpn_channel_accessor* target, wpn_channel_accessor* operator);
void wpn_channel_cmul       ( wpn_channel_accessor* target, wpn_channel_accessor* operator);
void wpn_channel_cdiv       ( wpn_channel_accessor* target, wpn_channel_accessor* operator);

void wpn_channel_lookup     ( wpn_channel_accessor* source,
                              wpn_channel_accessor* dest,
                              wpn_channel_accessor* head,
                              bool increment);

void wpn_channel_dcadd      ( wpn_channel_accessor* channel, sgn_t offset);
void wpn_channel_dcrem      ( wpn_channel_accessor* channel, sgn_t offset);
void wpn_channel_mul        ( wpn_channel_accessor* channel, sgn_t factor);
void wpn_channel_div        ( wpn_channel_accessor* channel, sgn_t factor);

sgn_t wpn_channel_min       ( wpn_channel_accessor* channel);
sgn_t wpn_channel_max       ( wpn_channel_accessor* channel);
sgn_t wpn_channel_rms       ( wpn_channel_accessor* channel, enum level_t scale);

//-------------------------------------------------------------------------------------------------
// STREAM
//-------------------------------------------------------------------------------------------------
struct wpn_sync
//-------------------------------------------------------------------------------------------------
{
    wpn_stream* stream;
    usize size;
    usize pos;
};

//-------------------------------------------------------------------------------------------------
struct wpn_stream
//-------------------------------------------------------------------------------------------------
{
    usize size;               // <-- remember that stream is not a channel vector
    u8 nchannels;             // <-- not to be confused with this
    wpn_sync upsync;          // <-- TODO: are syncs really necessary?
    wpn_sync dnsync;
    wpn_channel channels[];   // <-- compiler does not complain it seems..
};
//-------------------------------------------------------------------------------------------------
struct wpn_stack_stream
//-------------------------------------------------------------------------------------------------
{
    const usize size;
    const u8 nchannels;
    // there is probably no need to embed syncs in stack streams
    // they're meant to be fast and temporary
    // and should be used for operations only
    wpn_stack_channel channels[WPN_COMPILE_STREAM_STACK_SIZE];
};

//-------------------------------------------------------------------------------------------------
struct wpn_stream_accessor
//-------------------------------------------------------------------------------------------------
{
    wpn_stream* parent;
    usize begin;
    usize end;
    usize pos;
    usize size;
};

wpn_stream*
wpn_stream_allocate         ( u8 nchannels, usize size);

wpn_stream_accessor
wpn_stream_access           ( wpn_stream* channel, usize begin, usize size, usize pos);

wpn_stream_accessor
wpn_stack_stream_access     ( wpn_stack_stream* stream, usize begin, usize size, usize pos);

void wpn_stream_drain       ( wpn_stream_accessor* accessor);
void wpn_stream_vfill       ( wpn_stream_accessor* accessor, sgn_t v);


void wpn_stream_cprp        ( wpn_stream_accessor* source, wpn_stream_accessor* dest);
void wpn_stream_cpmg        ( wpn_stream_accessor* source, wpn_stream_accessor* dest);
void wpn_stream_pour        ( wpn_stream_accessor* source, wpn_stream_accessor* dest);

void wpn_stream_srem        ( wpn_stream_accessor* target, wpn_stream_accessor* operator);
void wpn_stream_smul        ( wpn_stream_accessor* target, wpn_stream_accessor* operator);
void wpn_stream_sdiv        ( wpn_stream_accessor* target, wpn_stream_accessor* operator);

void wpn_stream_lookup      ( wpn_stream_accessor* source,
                              wpn_stream_accessor* dest,
                              wpn_stream_accessor* head,
                              bool increment);

void wpn_stream_dcadd       ( wpn_stream_accessor* channel, sgn_t offset);
void wpn_stream_dcrem       ( wpn_stream_accessor* channel, sgn_t offset);
void wpn_stream_mul         ( wpn_stream_accessor* channel, sgn_t factor);
void wpn_stream_div         ( wpn_stream_accessor* channel, sgn_t factor);

sgn_t wpn_stream_min        ( wpn_stream_accessor* channel);
sgn_t wpn_stream_max        ( wpn_stream_accessor* channel);
sgn_t wpn_stream_rms        ( wpn_stream_accessor* channel, enum level_t scale);

static wpn_stream_accessor wpn_sync_draw(wpn_stream* stream, usize sz);
static void wpn_sync_pour(wpn_stream* stream, usize sz);
static void wpn_sync_skip(wpn_stream* stream, usize sz);

//=================================================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
// GRAPH
///////////////////////////////////////////////////////////////////////////////////////////////////
//=================================================================================================

typedef struct wpn_graph wpn_graph;
typedef struct gproperties gproperties;

typedef struct wpn_node wpn_node;
typedef struct wpn_pin wpn_pin;
typedef struct wpn_pin_vector wpn_pin_vector;
typedef struct wpn_pin_stream wpn_pin_stream;
typedef struct wpn_pool wpn_pool;

typedef struct wpn_connection wpn_connection;

// -- UDECL_FN:     the pin-declaration callback
// -- UCONFIG_FN:   called just before being included in the signal-processing chain
// -- UPROCESS_FN:  the rwrite processing function

typedef void (*udecl_fn)    ( wpn_node*);
typedef void (*uprocess_fn) ( wpn_pool*, wpn_pool*, void*);
typedef void (*uconfig_fn)  ( sgn_t, u16, void*); //-- TO_BE_EXAMINED

//=================================================================================================
struct gproperties
{
    sgn_t rate;
    usize vecsz;
    usize vecsz_fb;
};
//-------------------------------------------------------------------------------------------------
struct wpn_graph
//-------------------------------------------------------------------------------------------------
{
    gproperties properties;
    usize nnodes;
    usize nconnections;
    wpn_node* nodes;
    wpn_connection* connections;
};

wpn_graph                   //---------------------------------------------------------------
wpn_graph_create            ( sgn_t rate, u16 vsz, u16 fvsz);

void                        //---------------------------------------------------------------
wpn_graph_register          ( wpn_graph* graph, void* udata,
                              udecl_fn dcl_fn, uconfig_fn cf_fn, uprocess_fn rw_fn);

wpn_connection*             //---------------------------------------------------------------
wpn_graph_pconnect          ( wpn_graph* graph, wpn_pin* source, wpn_pin* dest);

wpn_connection*             //---------------------------------------------------------------
wpn_graph_nconnect          ( wpn_graph* graph, wpn_node* source, wpn_node* dest);

wpn_connection*             //---------------------------------------------------------------
wpn_graph_pnconnect         ( wpn_graph* graph, wpn_pin* source, wpn_node* dest);

wpn_connection*             //---------------------------------------------------------------
wpn_graph_npconnect         ( wpn_graph* graph, wpn_node* source, wpn_pin* pin);

wpn_connection*             //---------------------------------------------------------------
wpn_graph_connect           ( wpn_graph* graph, void* source, void* dest);

//-------------------------------------------------------------------------------------------

int wpn_graph_run_exit(wpn_graph* graph, void* unit, usize ntimes);

//-------------------------------------------------------------------------------------------------
struct wpn_pin
//-------------------------------------------------------------------------------------------------
{
    wpn_node* parent;
    const char* label;
    u16 index;
    u8 nchannels;
    bool default_;
    wpn_stream* stream;
    polarity_t polarity;

    u8 nconnections;
    wpn_connection *connections[WPN_COMPILE_PIN_STACK_SIZE];
    // note: connections are owned by the graph,
};
//-------------------------------------------------------------------------------------------------
struct wpn_pin_vector
//-------------------------------------------------------------------------------------------------
{
    u16 sz;
    wpn_pin data[WPN_COMPILE_NODE_STACK_SIZE];
};
//-------------------------------------------------------------------------------------------------
struct wpn_node
//-------------------------------------------------------------------------------------------------
{
    uprocess_fn uprocess_fn;
    uconfig_fn uconfig_fn;
    udecl_fn udecl_fn;
    void *udata;

    usize pos;
    bool intertwined;
    gproperties properties;

    wpn_pin_vector uppins;
    wpn_pin_vector dnpins;
};

//-------------------------------------------------------------------------------------------
void                        //---------------------------------------------------------------
wpn_pindecl                 ( wpn_node* n, enum polarity_t p, const char* l, u8 nch, bool df);

static void                 //---------------------------------------------------------------
wpn_pin_allocate            ( wpn_pin* pin, u16 sz);

static wpn_pin_vector*      //---------------------------------------------------------------
wpn_node_get_pin_vector     ( wpn_node* node, enum polarity_t polarity);

wpn_pin*                    //---------------------------------------------------------------
wpn_default_pin             ( wpn_node* node, enum polarity_t polarity);

//-------------------------------------------------------------------------------------------
static void wpn_node_configure  ( wpn_node* node, gproperties properties);
static void wpn_node_process    ( wpn_node* node);

//-------------------------------------------------------------------------------------------
// POOLS / PIN_STREAMS
//-------------------------------------------------------------------------------------------
struct wpn_pin_stream
//-------------------------------------------------------------------------------------------
{
    wpn_stream_accessor accessor;
    const char *label;
};
//-------------------------------------------------------------------------------------------
struct wpn_pool
//-------------------------------------------------------------------------------------------
{
    usize nstreams;
    wpn_pin_stream streams[WPN_COMPILE_NODE_STACK_SIZE];
};

wpn_stream_accessor* wpn_streamext_l(wpn_pool* pool, const char* pin);
wpn_stream_accessor* wpn_streamext_i(wpn_pool* pool, u16 index);

//-------------------------------------------------------------------------------------------
struct wpn_connection
//-------------------------------------------------------------------------------------------
{
    wpn_stream* stream;
    wpn_pin* source;
    wpn_pin* dest;
    bool active;
    bool muted;
    sgn_t level;
    bool feedback;
};

static void wpn_connection_pull(wpn_connection* con, usize size);

//===========================================================================================
// CRASH_TEST_FOR_DUMMIES
//===========================================================================================

typedef struct sinetest sinetest;
struct sinetest
{
    wpn_node* node;
    sgn_t phase;
    sgn_t frequency;
    sgn_t srate;
    wpn_stream* stream;
};

void sinetest_dcl   ( wpn_node* node);
void sinetest_cf    ( sgn_t rate, u16 size, void* udata);
void sinetest_rw    ( wpn_pool* uppool, wpn_pool* dnpool, void* udata);

//-------------------------------------------------------------------------------------------

typedef struct vca vca;
struct vca
{
    wpn_node* node;
    sgn_t gain;
};

void vca_dcl    ( wpn_node* node);
void vca_cf     ( sgn_t rate, u16 sz, void* udata);
void vca_rw     ( wpn_pool* uppool, wpn_pool* dnpool, void* udata);

//-------------------------------------------------------------------------------------------

typedef struct dummy_output dummy_output;
struct dummy_output
{
    wpn_node* node;
};

void dmout_dcl  ( wpn_node* node);
void dmout_cf   ( sgn_t rate, u16 sz, void* udata);
void dmout_rw   ( wpn_pool* uppool, wpn_pool* dnpool, void* udata);

#ifdef __cplusplus
}
#endif