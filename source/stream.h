#pragma once
//-------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "utilities.h"
//-------------------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//-------------------------------------------------------------------------------------------------

// now, as we cannot go too generic in C
// we basically assume a single type (sample_t) to rule them all
// which can be configured with a simple external define at compilation time

#ifdef WPN_EXTERN_DEF_SINGLE_PRECISION
    typedef float sample_t;
#elif defined(WPN_EXTERN_DEF_DOUBLE_PRECISION)
    typedef double sample_t;
#else
    typedef float sample_t;
#endif

typedef sample_t smp_t;
typedef sample_t frame_t;
typedef frame_t frm_t;
// as the representation of the difference between sample and frame may vary
// from api to api,
// this is based on PCM terminology:
// a sample represents the amplitude of one channel of sound at a certain point in time
// hence a frame consists of exactly one sample per channel

enum wpn_error
{
    NO_ERROR = 0
}   WPN_DECL_ATTRIBUTE_PACKED;

typedef enum wpn_error wpn_error;

//-------------------------------------------------------------------------------------------------
#define log10gen(_x)    _Generic((_x), double: log10, float: log10f, long double: log10l)(_x)
#define powgen(_x, _y)  _Generic((_x,_y), double: pow, float: powf, long double: powl)(_x, _y)
#define sqrtgen(_x)     _Generic((_x), double: sqrt, float: sqrtf, long double: sqrtl)(_x)
#define floorgen(_x)    _Generic((_x), double: floor, float: floorf, long double: floorl)(_x)
//-------------------------------------------------------------------------------------------------
#define atodb(_a) log10gen(_a)*20
#define dbtoa(_a) powgen(10, _a*.05)
//-------------------------------------------------------------------------------------------------
#define lininterp(_x,_a,_b) _a+_x*(_b-_a)
//-------------------------------------------------------------------------------------------------

 sample_t __always_inline rms(sample_t* data, size_t sz);

//-------------------------------------------------------------------------------------------------

enum level_t     { LINEAR = 0, DBFS = 1 } WPN_DECL_ATTRIBUTE_PACKED;
enum polarity_t  { OUTPUT = 0, INPUT= 1 } WPN_DECL_ATTRIBUTE_PACKED;

typedef enum polarity_t polarity_t;
typedef enum level_t level_t;

//-------------------------------------------------------------------------------------------------
#define sframe(_nm, _sz, _tp) struct _nm { _tp data[_sz]; }
// stack-allocated frame definition macro

typedef struct rframe rframe;
// a reference to a frame in a stream
struct rframe
{
    sample_t* begin;
    sample_t* end;
    byte_t nchannels;
};

//-------------------------------------------------------------------------------------------------
typedef struct channel_accessor channel_accessor;

#define schannel(_nm, _sz, _tp) sframe(_nm, _sz, _tp)
// stack-allocated channel definition macro
// e.g. schannel(schannel512, 512u, smp_t);
// will declare a struct named "schannel512" with a stack-array of 512 samples
// note: this is the same macro used for sframe, hence just a matter of identification

//-------------------------------------------------------------------------------------------------
typedef struct hchannel hchannel;
struct hchannel
// a heap-allocated contiguous block of data representing a single audio channel
//-------------------------------------------------------------------------------------------------
{
    size_t nsamples;
    sample_t data[];
};

hchannel*
hchannel_alloc(size_t nsamples);

void
hchannel_free(hchannel*);

//-------------------------------------------------------------------------------------------------
struct channel_accessor
// accessors are used to delimit, iterate over and modify
// a range of data from a collection
//-------------------------------------------------------------------------------------------------
{
    sample_t* data;
    size_t nsamples;
};

channel_accessor
schannel_access(void* channel, size_t begin, size_t size, size_t pos);

channel_accessor
hchannel_access(hchannel* channel, size_t begin, size_t size) __nonnull();

// the following is valid for both schannel and hchannel:
// modifiers
void channel_zero(channel_accessor* chn) __nonnull();
void channel_normalize(channel_accessor* chn) __nonnull();
void channel_fillv(channel_accessor* chn, sample_t const v) __nonnull();

// copy-merge and move-merge
void channel_cpmg(channel_accessor* source, channel_accessor* dest) __nonnull();
void channel_mvmg(channel_accessor* source, channel_accessor* dest) __nonnull();

// copy-replace and move-replace
void channel_cprp(channel_accessor* source, channel_accessor* dest);
void channel_mvrp(channel_accessor* source, channel_accessor* dest);

// channel-to-channel operations
void channel_addeq(channel_accessor* lhs, channel_accessor* rhs);
void channel_remeq(channel_accessor* lhs, channel_accessor* rhs);
void channel_muleq(channel_accessor* lhs, channel_accessor* rhs);
void channel_diveq(channel_accessor* lhs, channel_accessor* rhs);

void channel_addv(channel_accessor* target, sample_t const v);
void channel_remv(channel_accessor* target, sample_t const v);
void channel_mulv(channel_accessor* target, sample_t const v);
void channel_divv(channel_accessor* target, sample_t const v);

// analysis
sample_t channel_min(channel_accessor* chn);
sample_t channel_max(channel_accessor* chn);
sample_t channel_rms(channel_accessor* chn);

wpn_error
channel_loookup(channel_accessor* receiver,
                channel_accessor* source,
                channel_accessor* head,
                bool increment);

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// STREAMS
//
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef byte_t nchannels_t;
typedef nchannels_t nchn_t;
//-------------------------------------------------------------------------------------------------
enum frame_order { NON_INTERLEAVED = 0, INTERLEAVED = 1 } WPN_DECL_ATTRIBUTE_PACKED;

typedef enum frame_order frame_order;
typedef frame_order f_order;

//-------------------------------------------------------------------------------------------------
typedef struct stream_accessor stream_accessor;

#define sstream(_nm, _sz, _tp) struct _nm { frame_order order; byte_t nch; _tp data[_sz]; }
// stack-allocated stream, same macro principle as schannel

stream_accessor
sstream_access(void* stream, size_t begin, size_t size);
//-------------------------------------------------------------------------------------------------

// returns an error is configuration fails
wpn_error sstream_configure(void*, byte_t nchannels, frame_order order);

//-------------------------------------------------------------------------------------------------
typedef struct hstream hstream;
struct hstream
// heap-allocated stream
// may be interleaved or non_interleaved
//-------------------------------------------------------------------------------------------------
{
    frame_order order;
    nchn_t nchannels;
    size_t nsamples;
    sample_t data[];
};

hstream*
hstream_alloc(byte_t nchannels, frame_order order, size_t nsamples);
void hstream_free(hstream*);

stream_accessor
hstream_access(hstream* stream, size_t begin, size_t nframes);

//-------------------------------------------------------------------------------------------------
struct stream_accessor
//-------------------------------------------------------------------------------------------------
{
    sample_t* data; // this would be a pointer to the beginning of the stream's data
    size_t fpc; // frames per channel TOTAL
    size_t startframe;
    nchn_t nchannels;
    size_t nsamples; // SAMPLES NOT FRAMES, to get nframes: nsamples/nchannels
    frame_order order;
};

channel_accessor
channel_cast(stream_accessor* stream);

channel_accessor
stream_channel_at(stream_accessor* stream, size_t index);

rframe
stream_frm_at(stream_accessor* stream, size_t index);

void
stream_interleave(stream_accessor* stream);

void
stream_deinterleave(stream_accessor* stream);

//-------------------------------------------------------------------------------------------------

void stream_zero(stream_accessor* stream);
void stream_normalize(stream_accessor* stream);
void stream_fillv(stream_accessor* stream, sample_t const v);

void stream_cpmg(stream_accessor* source, stream_accessor* dest);
void stream_mvmg(stream_accessor* source, stream_accessor* dest);
void stream_cprp(stream_accessor* source, stream_accessor* dest);
void stream114_mvrp(stream_accessor* source, stream_accessor* dest);

void stream_addeq(stream_accessor* lhs, stream_accessor* rhs);
void stream_remeq(stream_accessor* lhs, stream_accessor* rhs);
void stream_muleq(stream_accessor* lhs, stream_accessor* rhs);
void stream_diveq(stream_accessor* lhs, stream_accessor* rhs);

void stream114_addv(stream_accessor* stm, sample_t const v);
void stream114_remv(stream_accessor* stm, sample_t const v);
void stream114_mulv(stream_accessor* stm, sample_t const v);
void stream114_divv(stream_accessor* stm, sample_t const v);

sample_t stream_min(stream_accessor* stm);
sample_t stream_max(stream_accessor* stm);
sample_t stream_rms(stream_accessor* stm);

wpn_error
stream_lookup(stream_accessor* receiver,
              stream_accessor* source,
              stream_accessor* head,
              bool increment);

//-------------------------------------------------------------------------------------------------
// interleaved/channel distinct operation functions
//-------------------------------------------------------------------------------------------------

void stream_zero_i(stream_accessor* stream);
void stream_fillv_i(stream_accessor* stream, sample_t const v);
void stream_normalize_i(stream_accessor* stream);

void stream_zero_c(stream_accessor* stream);
void stream_normalize_c(stream_accessor* stream);
void stream_fillv_c(stream_accessor* stream, sample_t const v);

//------------------------------------------------------------------

void stream_cpmg_i(stream_accessor* source, stream_accessor* dest);
void stream_mvmg_i(stream_accessor* source, stream_accessor* dest);
void stream_cprp_i(stream_accessor* source, stream_accessor* dest);
void stream_mvrp_i(stream_accessor* source, stream_accessor* dest);

void stream_cpmg_c(stream_accessor* source, stream_accessor* dest);
void stream_mvmg_c(stream_accessor* source, stream_accessor* dest);
void stream_cprp_c(stream_accessor* source, stream_accessor* dest);
void stream_mvrp_c(stream_accessor* source, stream_accessor* dest);

//------------------------------------------------------------------

void stream_addeq_i(stream_accessor* lhs, stream_accessor* rhs);
void stream_remeq_i(stream_accessor* lhs, stream_accessor* rhs);
void stream_muleq_i(stream_accessor* lhs, stream_accessor* rhs);
void stream_diveq_i(stream_accessor* lhs, stream_accessor* rhs);

void stream_addeq_c(stream_accessor* lhs, stream_accessor* rhs);
void stream_remeq_c(stream_accessor* lhs, stream_accessor* rhs);
void stream_muleq_c(stream_accessor* lhs, stream_accessor* rhs);
void stream_diveq_c(stream_accessor* lhs, stream_accessor* rhs);

//------------------------------------------------------------------
void stream_addv_i(stream_accessor* stm, sample_t const v);
void stream_remv_i(stream_accessor* stm, sample_t const v);
void stream_mulv_i(stream_accessor* stm, sample_t const v);
void stream_divv_i(stream_accessor* stm, sample_t const v);

void stream_addv_c(stream_accessor* stm, sample_t const v);
void stream_remv_c(stream_accessor* stm, sample_t const v);
void stream_mulv_c(stream_accessor* stm, sample_t const v);
void stream_divv_c(stream_accessor* stm, sample_t const v);

sample_t stream_min_i(stream_accessor* stm);
sample_t stream_max_i(stream_accessor* stm);
sample_t stream_rms_i(stream_accessor* stm);

sample_t stream_min_c(stream_accessor* stm);
sample_t stream_max_c(stream_accessor* stm);
sample_t stream_rms_c(stream_accessor* stm);

#ifdef __cplusplus
}
#endif
