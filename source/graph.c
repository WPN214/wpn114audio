#include "graph.h"
#include <string.h>

vector_impl(sck_vec, wpn_socket, vector_t)
vector_impl(nd_vec, wpn_node, vector_t)
vector_impl(cn_vec, wpn_connection, vector_t)
vector_impl(cnref_vec, wpn_connection*, vector_t)

// the number of items reserved in respective vectors

#define WPN114_GRAPH_NNODES_INIT    10
#define WPN114_GRAPH_NCONN_INIT     30
#define WPN114_NODE_NSOCKETS_INIT   10
#define WPN114_SOCKET_NCONN_INIT    10

wpn_graph
wpn_graph_create(sample_t rate, vector_t vector_size, vector_t fb_vector_size)
{
    WPN114_STRUCT_CREATE_INITIALIZE
         ( wpn_graph, graph )
    {
        .properties.rate     = rate,
        .properties.vecsz    = vector_size,
        .properties.vecsz_fb = fb_vector_size
    };

    // we reserve space upfront for nodes and connections
    nd_vec_initialize(&graph.nodes, WPN114_GRAPH_NNODES_INIT);
    cn_vec_initialize(&graph.connections, WPN114_GRAPH_NCONN_INIT);

    return graph;
}

// TODO: make this a function in utilities
// check size and realloc if needed

#define rvector_pull(_vec) &_vec->data[_vec->count]; _vec->count++;
#define vector_pull(_vec) &_vec.data[_vec.count]; _vec.count++;

wpn_node*
wpn_graph_register(wpn_graph* graph,
                   void* udata,
                   udcl_fn dcl_fn,
                   ucfg_fn cfg_fn,
                   uprc_fn prc_fn)
{
    WPN114_STRUCT_CREATE_INITIALIZE
         ( wpn_node, node )
    {
        .udata = udata,
        .dcl_fn = dcl_fn,
        .cfg_fn = cfg_fn,
        .prc_fn = prc_fn,
        .properties = graph->properties,
        .interconnected = false,
        .pos = 0
    };

    wpn_node* ptr = vector_pull(graph->nodes);
    *ptr = node;

    // initialize socket vectors and reserve space for 10 sockets
    sck_vec_initialize(&ptr->in_sockets, WPN114_NODE_NSOCKETS_INIT);
    sck_vec_initialize(&ptr->out_sockets, WPN114_NODE_NSOCKETS_INIT);

    if ( dcl_fn )
         dcl_fn(ptr);

    if ( cfg_fn )
        cfg_fn(graph->properties, udata);

    return ptr;
}

wpn_node*
wpn_graph_lookup(wpn_graph* graph, void* udata)
{
    for_n(n, graph->nodes)
        if ( graph->nodes.data[n].udata == udata )
             return &graph->nodes.data[n];
}

static wpn_connection*
wpn_graph_connect(wpn_graph* graph, wpn_socket* source, wpn_socket* dest)
{
    if ( !source || ! dest )
         return nullptr;

    wpn_connection* ncon = vector_pull(graph->connections);

    ncon->source    = source;
    ncon->dest      = dest;
    ncon->active    = true;
    ncon->muted     = false;
    ncon->feedback  = false;
    ncon->level     = 1.0;

    nchn_t nchannels = wpnmax(source->stream->nchannels,
                              dest->stream->nchannels);

    ncon->stream = hstream_alloc(
                nchannels,
                INTERLEAVED,
                graph->properties.vecsz);
}

#define lookup_node(_a,_b) wpn_graph_lookup(_a,_b)
#define lookup_socket(_a,_b,_c) wpn_node_lookup(_a,_b,_c)
#define lookup_default(_a,_b) wpn_node_lookup_default(_a,_b)

wpn_connection*
wpn_graph_nconnect(wpn_graph* graph, void* source, void* dest)
{
    wpn_node* nsource = lookup_node(graph, source);
    wpn_node* ndest = lookup_node(graph, dest);

    return wpn_graph_connect(graph,
                lookup_default(nsource, OUTPUT),
                lookup_default(ndest, INPUT));
}

wpn_connection*
wpn_graph_sconnect(wpn_graph* graph,
                   void* source, const char* out_socket,
                   void* dest, const char* in_socket)
{
    wpn_node* nsource  = lookup_node(graph, source);
    wpn_node* ndest    = lookup_node(graph, dest);

    return wpn_graph_connect(graph,
                lookup_socket(nsource, OUTPUT, out_socket),
                lookup_socket(ndest, INPUT, in_socket));
}

wpn_connection*
wpn_graph_nsconnect(wpn_graph* graph,
                    void* source, void* dest, const char* in_socket)
{
    wpn_node* nsource   = lookup_node(graph, source);
    wpn_node* ndest     = lookup_node(graph, dest);

    return wpn_graph_connect(graph,
                lookup_default(source, OUTPUT),
                lookup_socket(ndest, INPUT, in_socket));
}

wpn_connection*
wpn_graph_snconnect(wpn_graph* graph,
                    void* source, const char* out_socket,
                    void* dest)
{
    wpn_node* nsource   = lookup_node(graph, source);
    wpn_node* ndest     = lookup_node(graph, dest);

    return wpn_graph_connect(graph,
                lookup_socket(nsource, OUTPUT, out_socket),
                lookup_default(ndest, INPUT));
}

int
wpn_graph_run_exit(wpn_graph* graph, void* unit, size_t ntimes)
{
    return 0;
}

//-------------------------------------------------------------------------------------------------
// SOCKETS
//-------------------------------------------------------------------------------------------------

static __always_inline sck_vec*
wpn_node_get_vector(wpn_node* n, polarity_t p)
{
    switch(p)
    {
    case INPUT:  return &n->in_sockets;
    case OUTPUT: return &n->out_sockets;
    }
}

void
wpn_socket_declare(wpn_node* n, polarity_t p, const char* l, nchannels_t nch, bool df)
{
    sck_vec* target = wpn_node_get_vector(n,p);

    wpn_socket* socket = rvector_pull(target);

    socket->label      = l;
    socket->parent     = n;
    socket->default_   = df;
    socket->polarity   = p;

    // initialize connections reference vector
    cnref_vec_initialize
    (&socket->connections, WPN114_SOCKET_NCONN_INIT);

    // allocate stream
    socket->stream = hstream_alloc
            (nch, INTERLEAVED, n->properties.vecsz);
}

void
wpn_socket_add_connection(wpn_socket* s, wpn_connection* con)
{
    cnref_vec_add(&s->connections, con);
}

void
wpn_socket_rem_connection(wpn_socket* s, wpn_connection* con)
{
    cnref_vec_rem(&s->connections, con);
}

bool
wpn_socket_connected(wpn_socket* socket)
{
    return socket->connections.count > 0;
}

bool
wpn_socket_sconnected(wpn_socket* socket, wpn_socket* target)
{
    return cnref_vec_contains(target);
}

bool
wpn_socket_nconnected(wpn_socket* socket, wpn_node* target)
{

}

//-------------------------------------------------------------------------------------------------
// NODES
//-------------------------------------------------------------------------------------------------

wpn_socket*
wpn_node_lookup_default(wpn_node* n, polarity_t p)
{
    sck_vec target = *wpn_node_get_vector(n,p);

    for_n(n, target)
        if ( target.data[n].default_ )
             return &target.data[n];

    return nullptr;
}

wpn_socket*
wpn_node_lookup(wpn_node* n, polarity_t p, const char* l)
{
    sck_vec target = *wpn_node_get_vector(n,p);

    for_n(n, target)
        if ( !strcmp(target.data[n].label, l))
             return &target.data[n];

    return nullptr;
}

void
wpn_node_process(wpn_node* node)
{
    // we usually use a smaller vector size
    // in case of feedback connections, as to keep
    // the implicit delay to a minimum, without putting
    // too much stress on the processed graph
    vector_t sz = node->interconnected ?
                  node->properties.vecsz :
                  node->properties.vecsz_fb;

    foreach ( wpn_socket s, node->in_sockets,
           foreach ( wpn_connection* c, s.connections,
                     wpn_connection_pull(c, sz)));

    // make pools
    pool114 uppool, dnpool;

    node->prc_fn(&uppool, &dnpool, node->udata, sz);
    node->pos += sz;

}

//-------------------------------------------------------------------------------------------------
// CONNECTIONS
//-------------------------------------------------------------------------------------------------

void
wpn_connection_pull(wpn_connection* con, vector_t sz)
{
    if ( !con->active )
         return;

    // if this is a feedback connection
    // we directly draw from source's stream
    // otherwise we call process until socket buffer is sufficiently filled
    if ( !con->feedback )
         while (con->source->parent->pos < sz)
                wpn_node_process(con->source->parent);

    if ( !con->muted )
     {
         // draw from upsync,
         // get a stream accessor to the fetched data,
         // and pour downstream
         stream_accessor acc = wpn_sync_draw(con->stream, sz);
         stream_mulv_i( &acc, con->level );
         wpn_sync_pour  ( con->stream, sz );
     }
     // if connection is muted, do not draw/pour
     // just increment position
     else wpn_sync_skip(con->stream, sz);

}

stream_accessor*
wpn_pool_extr(wpn_pool* pool, const char* stream)
{
    wpn_pool p = *pool;

    for_n(n, p)
        if ( !strcmp(p.data[n].label, stream))
             return &p.data[n].stream;

    return nullptr;
}
