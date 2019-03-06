#pragma once
#include <stdint.h>

#define for_n(_n, _v) for(vector_t _n = 0; _n < _v.count; ++_n)

#define foreach(_arg, _v, _f) for(vector_t _n = 0; _n < _v.count; ++_n) { _arg = _v.data[_n]; _f;}

#define nullptr NULL
typedef __uint8_t byte_t;

#define wpn_calloc(_tp, _sz) (_tp*) calloc(sizeof(_tp), _sz)
#define wpn_malloc_flm(_tp, _stp, _sz) (_tp*) malloc(sizeof(_tp)+(sizeof(_stp)*_sz))

#define WPN114_STRUCT_CREATE_INITIALIZE(_tp, _nm) _tp _nm =

#ifdef __GNUC__
    #ifndef __clang__
        #define WPN_DECL_ATTRIBUTE_PACKED __attribute__((__packed__))
        #else
        #define WPN_DECL_ATTRIBUTE_PACKED
    #endif
#else
    #define WPN_DECL_ATTRIBUTE_PACKED
#endif

#define WPN114_CALL_UNIMPLEMENTED  \
    printf("WPN114, calling unimplemented function, aborting..."); \
    assert(false);

//-------------------------------------------------------------------------------------------------
#define wpnmin(_a,_b) ((_a) < (_b) ? (_a) : (_b))
#define wpnmax(_a,_b) ((_a) > (_b) ? (_a) : (_b))
//-------------------------------------------------------------------------------------------------
// VECTORS
//-------------------------------------------------------------------------------------------------

#define vector_decl(_nm, _tp, _sztp) struct _nm { _tp* data; _sztp count; _sztp capacity; }; \
    void _nm##_initialize(struct _nm* _vec, _sztp _res); \
    void _nm##_reserve(struct _nm* _vec, _sztp _sz); \
    void _nm##_add(struct _nm* _vec, struct _tp); \
    void _nm##_rem(struct _nm* _vec, struct _tp*);

#define vector_at(_vector, _index) _vector.data[_index]

#define vector_impl(_nm, _tp, _sztp) \
    void _nm##_initialize(struct _nm* vec, _sztp _sz) { \
        vec->data = nullptr; \
        vec->count = 0; \
        vec->capacity = _sz;\
        _nm##_reserve(vec, _sz);\
    } \
    void _nm##_reserve(struct _nm* vec, _sztp _sz) { \
         vec->data = wpn_calloc(_tp, _sz);\
    } \
    void _nm##_add(struct _nm* vec, struct _tp data) { \
        \
        vec->data[vec->count] = data;\
    }
