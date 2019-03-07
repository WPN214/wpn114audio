#pragma once

#include "stream.h"
#include "utilities.h"
#include <assert.h>

// ----------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------------------

// examples of stack-initialized custom data structures
sframe(stereo_frame, 2u, frame_t);
schannel(schannel32, 32u, sample_t);
sstream(sstream32, 32, sample_t);

typedef struct stereo_frame stereo_frame;
typedef struct schannel32 schannel32;
typedef struct sstream32 sstream32;

//-------------------------------------------------------------------------------------------------

typedef struct wpn_node wpn_node;
typedef struct wpn_socket wpn_socket;
typedef struct wpn_connection wpn_connection;
typedef struct wpn_graph wpn_graph;
typedef struct wpn_graph_properties wpn_graph_properties;
typedef struct wpn_stream wpn_stream;
typedef struct wpn_pool wpn_pool;
typedef struct wpn_routing wpn_routing;

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

// POOL
vector_decl(wpn_pool, wpn_stream, vector_t)

#define strmextr(_p, _cc) wpn_pool_extr(_p, _cc)
stream_accessor* wpn_pool_extr(wpn_pool*, const char* stream);

//-------------------------------------------------------------------------------------------------
struct wpn_node
// created, initialized and owned by the graph struct
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

bool wpn_node_connected(wpn_node*);
bool wpn_node_nconnected(wpn_node*, wpn_node*);
bool wpn_node_sconnected(wpn_node*, wpn_socket*);

wpn_socket* wpn_socket_lookup_default(wpn_node*, polarity_t);
wpn_socket* wpn_socket_lookup(wpn_node*, const char*);
wpn_socket* wpn_socket_lookup_p(wpn_node*, polarity_t, const char*);

//-------------------------------------------------------------------------------------------------
struct wpn_connection
// created, initialized and owned by the graph struct
//-------------------------------------------------------------------------------------------------
{
    wpn_socket* source;
    wpn_socket* dest;
    bool muted;
    bool active;
    bool feedback;
    sample_t level;
    hstream* stream;    
    wpn_routing* routing;
};

void wpn_connection_pull(wpn_connection*, vector_t sz);

#ifdef __cplusplus
}
#endif
