#pragma once

#include "stream.h"
#include "utilities.h"
#include <assert.h>

// ----------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------------------

#define stmextr(_p, _cc) wpn_pool_extr(_p, _cc)

sframe(stereo_frame, 2u, frame_t);
sframe(quadriphonic_frame, 4u, frame_t) ;
sframe(hexaphonic_frame, 6u, frame_t);
sframe(octophonic_frame, 8u, frame_t);

typedef struct stereo_frame stereo_frame;
typedef struct quadriphonic_frame quadriphonic_frame;
typedef struct hexaphonic_frame hexaphonic_frame;
typedef struct octophonic_frame octophonic_frame;

#ifdef WPN_EXTERN_DEF_DOUBLE_PRECISION
int unittest_1()
{
    assert(sizeof(stereo_frame) == 16);
    assert(sizeof(quadriphonic_frame) == 32);
    assert(sizeof(hexaphonic_frame) == 48);
    assert(sizeof(octophonic_frame) == 64);
}
#endif

schannel(schannel32, 32u, smp_t);
schannel(schannel64, 64u, smp_t);
schannel(schannel128, 128u, smp_t);
schannel(schannel256, 256u, smp_t);
schannel(schannel512, 512u, smp_t);

typedef struct schannel32 schannel32;
typedef struct schannel64 schannel64;
typedef struct schannel128 schannel128;
typedef struct schannel256 schannel256;
typedef struct schannel512 schannel512;

sstream(sstream32, 32, smp_t);
sstream(sstream64, 64, smp_t);
sstream(sstream128, 128, smp_t);
sstream(sstream256, 256, smp_t);
sstream(sstream512, 512, smp_t);

typedef struct sstream32 sstream32;
typedef struct sstream64 sstream64;
typedef struct sstream128 sstream128;
typedef struct sstream256 sstream256;
typedef struct sstream512 sstream512;

//-------------------------------------------------------------------------------------------------
typedef struct wpn_node wpn_node;
typedef struct wpn_socket wpn_socket;
typedef struct wpn_connection wpn_connection;
typedef struct wpn_graph wpn_graph;
typedef struct wpn_graph_properties wpn_graph_properties;
typedef struct wpn_stream wpn_stream;
typedef struct wpn_pool wpn_pool;

typedef wpn_node node114;
typedef wpn_socket socket114;
typedef wpn_connection connection114;
typedef wpn_graph graph114;
typedef wpn_stream stream114;
typedef wpn_pool pool114;

typedef uint16_t vector_t;

// io-declaration function pointer
typedef void (*udcl_fn) (wpn_node*);

// pre-configuration function pointer
typedef void (*ucfg_fn) (wpn_graph_properties, void*);

// signal processing function pointer
typedef void (*uprc_fn) (wpn_pool*, wpn_pool*, void*, vector_t);


// vectors ---------------------------------------
vector_decl(nd_vec, wpn_node, vector_t)
vector_decl(sck_vec, wpn_socket, vector_t)
vector_decl(cn_vec, wpn_connection, vector_t)
vector_decl(cnref_vec, wpn_connection*, vector_t)

typedef struct nd_vec nd_vec;
typedef struct sck_vec sck_vec;
typedef struct cn_vec cn_vec;
typedef struct cnref_vec cnref_vec;

typedef byte_t wpn_cable[2];

//-------------------------------------------------------------------------------------------------
struct wpn_routing
//-------------------------------------------------------------------------------------------------
{
    byte_t count;
    wpn_cable data[];
};

//-------------------------------------------------------------------------------------------------
struct wpn_graph_properties
//-------------------------------------------------------------------------------------------------
{
    sample_t rate;
    vector_t vecsz;
    vector_t vecsz_fb;
};

//-------------------------------------------------------------------------------------------------
struct wpn_graph
//-------------------------------------------------------------------------------------------------
{
    wpn_graph_properties properties;

    // nodes and connections are owned by the graph
    nd_vec nodes;
    cn_vec connections;
};

wpn_node*
wpn_graph_lookup(wpn_graph* graph, void* udata);

wpn_graph
wpn_graph_create(sample_t rate, vector_t vecsz, vector_t vecsz_fb);

wpn_node*
wpn_graph_register(wpn_graph* g, void* udata, udcl_fn dcl_fn, ucfg_fn cfg_fn, uprc_fn prc_fn);

wpn_connection*
wpn_graph_nconnect(wpn_graph*, wpn_node*, wpn_node*);

wpn_connection*
wpn_graph_sconnect(wpn_graph*, wpn_socket*, wpn_socket*);

wpn_connection*
wpn_graph_nsconnect(wpn_graph*, wpn_node*, wpn_socket*);

wpn_connection*
wpn_graph_snconnect(wpn_graph*, wpn_socket*, wpn_node*);

void
wpn_graph_sdisconnect(wpn_graph*, wpn_socket*);

//--------------------------------------------------------------------------------------------------

wpn_connection*
wpn_nconnect(wpn_graph*, void* src, void* dst);

wpn_connection*
wpn_sconnect(wpn_graph*, void* src, const char* out, void* dst, const char* in);

wpn_connection*
wpn_nsconnect(wpn_graph*, void* src, void* dst, const char* in);

wpn_connection*
wpn_snconnect(wpn_graph*, void* src, const char* out, void* dst);

void
wpn_sdisconnect(wpn_graph*, void* node, const char* socket);

int
wpn_graph_run_exit(wpn_graph* graph, void* unit, size_t ntimes);

//-------------------------------------------------------------------------------------------------
struct wpn_socket
//-------------------------------------------------------------------------------------------------
{
    hstream* stream;
    wpn_node* parent;

    // vector of connection references
    cnref_vec connections;

    bool default_;
    const char* label;
    polarity_t polarity;
};

void wpn_socket_declare(wpn_node*, polarity_t, const char*, nchannels_t, bool df);

void wpn_socket_add_connection(wpn_socket*, wpn_connection*);
void wpn_socket_rem_connection(wpn_socket*, wpn_connection*);

bool wpn_socket_connected(wpn_socket*);
bool wpn_socket_sconnected(wpn_socket*, wpn_socket* target);
bool wpn_socket_nconnected(wpn_socket*, wpn_node* target);

//-------------------------------------------------------------------------------------------------
struct wpn_stream
// a reference to a socket stream
//-------------------------------------------------------------------------------------------------
{
    const char* label;
    stream_accessor stream;
};

stream_accessor* wpn_pool_extr(wpn_pool*, const char* stream);

// POOL
vector_decl(wpn_pool, wpn_stream, vector_t)

//-------------------------------------------------------------------------------------------------
struct wpn_node
//-------------------------------------------------------------------------------------------------
{
    // user-defined data structure
    // and function pointers
    void* udata;
    udcl_fn dcl_fn;
    ucfg_fn cfg_fn;
    uprc_fn prc_fn;

    bool interconnected;
    wpn_graph_properties properties;

    sck_vec in_sockets;
    sck_vec out_sockets;
    vector_t pos;
};

void wpn_node_process(wpn_node*);

wpn_socket* wpn_socket_lookup_default(wpn_node*, polarity_t);
wpn_socket* wpn_socket_lookup(wpn_node*, const char*);
wpn_socket* wpn_socket_lookup_p(wpn_node*, polarity_t, const char*);

//-------------------------------------------------------------------------------------------------
struct wpn_connection
//-------------------------------------------------------------------------------------------------
{
    wpn_socket* source;
    wpn_socket* dest;
    bool muted;
    bool active;
    bool feedback;
    sample_t level;
    hstream* stream;
};

void wpn_connection_pull(wpn_connection*, vector_t sz);

#ifdef __cplusplus
}
#endif
