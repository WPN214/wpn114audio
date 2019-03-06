#include "stream.h"
#include <string.h>
#include <assert.h>

typedef channel_accessor c_acc;
typedef stream_accessor s_acc;
//-------------------------------------------------------------------------------------------------

sample_t __always_inline rms(sample_t* dt, size_t sz)
{
    sample_t mean = 0;
    for ( size_t n = 0; n < sz; ++n )
          mean += dt[n];

    return sqrtgen(1.0/sz*mean);
}

//-------------------------------------------------------------------------------------------------

hchannel*
hchannel_alloc(size_t nsamples)
{
    hchannel* channel = wpn_malloc_flm(hchannel, smp_t, nsamples);
    channel->nsamples = nsamples;

    return channel;
}

channel_accessor
hchannel_access(hchannel* chn, size_t bg, size_t nsamples)
{
    WPN114_STRUCT_CREATE_INITIALIZE
        (channel_accessor, acc)
    {
        .nsamples = nsamples == 0 ?
                    chn->nsamples :
                    nsamples,

        .data = &chn->data[bg],
    };

    return acc;
}

channel_accessor
schannel_access(void* chn, size_t bg, size_t sz, size_t pos)
{
    byte_t* raw_channel = (byte_t*) chn;

    WPN114_CALL_UNIMPLEMENTED;
}

void
channel_zero(channel_accessor* chn)
{
    memset(chn->data, 0, sizeof(smp_t)*chn->nsamples);
}

void
channel_normalize(channel_accessor* chn)
{
    WPN114_CALL_UNIMPLEMENTED;
}

//-------------------------------------------------------------------------------------------------
#define ch_operate(_a,_b,_op) \
    size_t _sz = wpnmin(_a->nsamples, _b->nsamples); \
    for (size_t _n = 0; _n < _sz; ++_n) \
         _b->data[_n] _op _a->data[_n]
//-------------------------------------------------------------------------------------------------

// now, should we assume that both accessors are of the same size?

void
channel_cpmg(channel_accessor* source, channel_accessor* dest)
{
    ch_operate(source, dest, +=);
}

void
channel_mvmg(channel_accessor* source, channel_accessor* dest)
{
    channel_cpmg(source, dest);
    channel_zero(source);
}

void
channel_cprp(channel_accessor* source, channel_accessor* dest)
{
    ch_operate(source, dest, =);
}

void
channel_mvrp(channel_accessor* source, channel_accessor* dest)
{
    channel_cprp(source, dest);
    channel_zero(dest);
}

void
channel_addeq(channel_accessor* lhs, channel_accessor* rhs)
{
    channel_cpmg(lhs, rhs);
}

void
channel_remeq(channel_accessor* lhs, channel_accessor* rhs)
{
    ch_operate(rhs, lhs, -=);
}

void
channel_muleq(channel_accessor* lhs, channel_accessor* rhs)
{
    ch_operate(rhs, lhs, *=);
}

void
channel_diveq(channel_accessor* lhs, channel_accessor* rhs)
{
    ch_operate(rhs, lhs, /=);
}

//-------------------------------------------------------------------------------------------------
#define ch_operate_v(_chn, _v, _op) \
    for (size_t _n = 0; _n < _chn->nsamples; ++_n) \
         _chn->data[_n] _op _v;
//-------------------------------------------------------------------------------------------------

void
channel_fillv(channel_accessor* chn, sample_t const v)
{
    ch_operate_v(chn, v, =);
}

void
channel_addv(channel_accessor* lhs, sample_t const v)
{
    ch_operate_v(lhs, v, +=);
}

void
channel_remv(channel_accessor* lhs, sample_t const v)
{
    ch_operate_v(lhs, v, -=);
}

void
channel_mulv(channel_accessor* lhs, sample_t const v)
{
    ch_operate_v(lhs, v, *=);
}

void
channel_divv(channel_accessor* lhs, sample_t const v)
{
    ch_operate_v(lhs, v, /=);
}

//-------------------------------------------------------------------------------------------------

sample_t
channel_min(channel_accessor* chn)
{
    sample_t m = 0;
    for ( size_t n = 0; n < chn->nsamples; ++n)
           m = wpnmin(m, chn->data[n]);
    return m;
}

sample_t
channel_max(channel_accessor* chn)
{
    sample_t m = 0;
    for ( size_t n = 0; n < chn->nsamples; ++n)
           m = wpnmax(m, chn->data[n]);
    return m;
}

sample_t
channel_rms(channel_accessor* chn)
{
    return rms(chn->data, chn->nsamples);
}

wpn_error
channel_lookup(channel_accessor* receiver,
               channel_accessor* source,
               channel_accessor* head,
               bool increment)
{
    assert(receiver->nsamples == source->nsamples &&
           receiver->nsamples == head->nsamples);

    if (increment)
    {
        for (size_t n = 0; n < source->nsamples; ++n)
        {
            sample_t index = head->data[n];
            size_t y = floorgen(index);
            sample_t x = index-(sample_t)y;
            sample_t e = lininterp(x, source->data[y],
                                      source->data[y+1]);
            receiver->data[n] = e;
        }
    }
    else
    {
        WPN114_CALL_UNIMPLEMENTED;
    }
}

//-------------------------------------------------------------------------------------------------
wpn_error
sstream_configure(void* channel, byte_t nchannels, frame_order order)
{

}

//-------------------------------------------------------------------------------------------------

hstream*
hstream_alloc(byte_t nchannels, frame_order order, size_t nframes)
{
    hstream* stream = wpn_malloc_flm(
                hstream, smp_t, nchannels*nframes);

    stream->order      = order;
    stream->nchannels  = nchannels;
    stream->nsamples   = nframes;

    return stream;
}

void
hstream_free(hstream* stm)
{
    free(stm);
}

//-------------------------------------------------------------------------------------------------

// returns a pointer to the beginning of an accessor (not stream)
#define s_dataptr(_sacc) &_sacc->data[_sacc->startframe]
#define s_nframes(_sacc) _sacc->nsamples/_sacc->nchannels

stream_accessor
hstream_access(hstream* stream, size_t begin, size_t nframes)
{
    WPN114_STRUCT_CREATE_INITIALIZE
        (stream_accessor, acc)
    {
        .data        = stream->data,
        .order       = stream->order,
        .nchannels   = stream->nchannels,
        .startframe  = begin,
        .fpc         = s_nframes(stream),
        .nsamples    = nframes == 0 ?
                       stream->nsamples :
                       (nframes * acc.nchannels)
    };

    return acc;
}

stream_accessor
sstream_access(void* stm, size_t begin, size_t sz)
{
    WPN114_CALL_UNIMPLEMENTED;
}

channel_accessor
channel_cast(stream_accessor* stream)
{
    channel_accessor ch;
    ch.data = s_dataptr(stream);
    ch.nsamples = stream->nsamples;

    return ch;
}

#define s_chn_at(_acc, _index) index*_acc->fpc + _acc->startframe

channel_accessor
stream_channel_at(stream_accessor* stream, size_t index)
{
    // we want build it now, and not upfront when we're instantiating the stream_accessor
    // because for global non channel-to-channel operations like stream_mulv
    // it will take the stream data as a whole and apply function to it,
    // regardless whether frame order is interleaved or not
    // which will make it a bit faster
    assert(stream->order == NON_INTERLEAVED);
    assert(index < stream->nchannels);

    channel_accessor channel;
    channel.nsamples = s_nframes(stream);
    channel.data = &stream->data[s_chn_at(stream, index)];
    return channel;
}

rframe
stream_frame_at(stream_accessor* stream, size_t index)
{
    assert(stream->order == INTERLEAVED);
    // TODO
}

void
stream_interleave(stream_accessor* stream)
{
    if ( stream->order == INTERLEAVED )
         return;

    WPN114_CALL_UNIMPLEMENTED;
}

void
stream_deinterleave(stream_accessor* stream)
{
    if ( stream->order == NON_INTERLEAVED )
         return;

    WPN114_CALL_UNIMPLEMENTED;
}

//-------------------------------------------------------------------------------------------------

// general guideline:
//-------------------
// for interleaved streams:
// --> we can basically operate directly from raw data pointer
// for channeled streams:
// we've got to do request a channel_accessor for each channel

// now, we might want to make separated functions for interleaved/channeled
// that will save us the cost of the switch
// if the user knows upfront if the stream is interleaved or not

#define wpn_generic_stream_binary_op(_s, _v, _fn) \
    switch(_s->order) { \
    case INTERLEAVED: { _fn##_i(_s,_v); break; } \
    case NON_INTERLEAVED: { _fn##_c(_s,_v); }}

#define wpn_generic_stream_unary_op(_s, _fn) \
    switch(_s->order) { \
    case INTERLEAVED: { _fn##_i(_s); break;} \
    case NON_INTERLEAVED: { _fn##_c(_s); }}

#define for_each_channel(_s, _n) for (nchannels_t _n = 0; _n < _s->nchannels; ++_n)
#define for_each_sample(_s, _n) for (size_t _n = 0; _n < _s->nsamples; ++_n)

//-------------------------------------------------------------------------------------------------
// STREAM_ZERO
//-------------------------------------------------------------------------------------------------

void
stream_zero(stream_accessor* s)
{
    wpn_generic_stream_unary_op
            (s, stream_zero);
}

void
stream_zero_i(stream_accessor* s)
{
    memset(s_dataptr(s), 0,
           sizeof(sample_t)* s->nsamples);
}

void
stream_zero_c(stream_accessor* s)
{
    // we iterate over stream's channels
    // and apply the matching channel operator function
    for_each_channel(s, n) {
        c_acc acc = stream_channel_at(s, n);
        channel_zero(&acc);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_NORMALIZE
//-------------------------------------------------------------------------------------------------

void
stream_normalize(stream_accessor* s)
{
    wpn_generic_stream_unary_op
            (s, stream_normalize);
}

void stream_normalize_i(stream_accessor* s)
{
    c_acc ch = channel_cast(s);
    channel_normalize(&ch);
}

void stream_normalize_c(stream_accessor* s)
{
    WPN114_CALL_UNIMPLEMENTED;
}

//-------------------------------------------------------------------------------------------------
// STREAM_FILLV
//-------------------------------------------------------------------------------------------------

void
stream_fillv(stream_accessor* s, sample_t v)
{
    wpn_generic_stream_binary_op
            (s,v, stream_fillv);
}

void
stream_fillv_i(stream_accessor* s, sample_t v)
{
    c_acc ch = channel_cast(s);
    channel_fillv(&ch, v);
}

void
stream_fillv_c(stream_accessor* s, sample_t v)
{
    for_each_channel(s, n) {
        c_acc ch = stream_channel_at(s, n);
        channel_fillv(&ch, v);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_COPY_MERGE
//-------------------------------------------------------------------------------------------------

void
stream_cpmg(stream_accessor* source, stream_accessor* dest)
{
    wpn_generic_stream_binary_op(source, dest, stream_cpmg);
}

void
stream_cpmg_i(stream_accessor* source, stream_accessor* dest)
{
    c_acc sch = channel_cast(source);
    c_acc dch = channel_cast(dest);
    channel_cpmg(&sch, &dch);
}

void
stream_cpmg_c(stream_accessor* source, stream_accessor* dest)
{
    for_each_channel(source, n) {
        c_acc sch = stream_channel_at(source, n);
        c_acc dch = stream_channel_at(dest, n);
        channel_cpmg(&sch, &dch);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_MOVE_MERGE
//-------------------------------------------------------------------------------------------------

void
stream_mvmg(stream_accessor* source, stream_accessor* dest)
{
    wpn_generic_stream_binary_op
            (source, dest, stream_mvmg);
}

void
stream_mvmg_i(stream_accessor* source, stream_accessor* dest)
{
    c_acc sch = channel_cast(source);
    c_acc dch = channel_cast(dest);
    channel_mvmg(&sch, &dch);
}

void
stream_mvmg_c(stream_accessor* source, stream_accessor* dest)
{
    for_each_channel(source, n) {
        c_acc sch = stream_channel_at(source, n);
        c_acc dch = stream_channel_at(dest, n);
        channel_mvmg(&sch, &dch);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_COPY_REPLACE
//-------------------------------------------------------------------------------------------------

void
stream_cprp(stream_accessor* source, stream_accessor* dest)
{
    wpn_generic_stream_binary_op
            (source, dest, stream_cprp);
}

void
stream_cprp_i(stream_accessor* source, stream_accessor* dest)
{
    c_acc sch = channel_cast(source);
    c_acc dch = channel_cast(dest);
    channel_cprp(&sch, &dch);
}

void
stream_cprp_c(stream_accessor* source, stream_accessor* dest)
{
    for_each_channel(source, n) {
        c_acc sch = stream_channel_at(source, n);
        c_acc dch = stream_channel_at(dest, n);
        channel_cprp(&sch, &dch);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_MOVE_REPLACE
//-------------------------------------------------------------------------------------------------

void
stream_mvrp(stream_accessor* source, stream_accessor* dest)
{
    wpn_generic_stream_binary_op
            (source, dest, stream_mvrp);
}

void
stream_mvrp_i(stream_accessor* source, stream_accessor* dest)
{
    c_acc sch = channel_cast(source);
    c_acc dch = channel_cast(dest);
    channel_mvrp(&sch, &dch);
}

void
stream_mvrp_c(stream_accessor* source, stream_accessor* dest)
{
    for_each_channel(source, n) {
        c_acc sch = stream_channel_at(source, n);
        c_acc dch = stream_channel_at(dest, n);
        channel_mvrp(&sch, &dch);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_ADD_EQUAL
//-------------------------------------------------------------------------------------------------

void
stream_addeq(stream_accessor* lhs, stream_accessor* rhs)
{
    wpn_generic_stream_binary_op
        (lhs, rhs, stream_addeq);
}

void
stream_addeq_i(stream_accessor* lhs, stream_accessor* rhs)
{
    c_acc clhs = channel_cast(lhs);
    c_acc crhs = channel_cast(rhs);
    channel_addeq(&clhs, &crhs);
}

void
stream_addeq_c(stream_accessor* lhs, stream_accessor* rhs)
{
    for_each_channel(lhs, n) {
        c_acc sch = stream_channel_at(lhs, n);
        c_acc dch = stream_channel_at(rhs, n);
        channel_addeq(&sch, &dch);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_MINUS_EQUAL
//-------------------------------------------------------------------------------------------------

void
stream_remeq(stream_accessor* lhs, stream_accessor* rhs)
{
    wpn_generic_stream_binary_op
        (lhs, rhs, stream_remeq);
}

void
stream_remeq_i(stream_accessor* lhs, stream_accessor* rhs)
{
    c_acc clhs = channel_cast(lhs);
    c_acc crhs = channel_cast(rhs);
    channel_remeq(&clhs, &crhs);
}

void
stream_remeq_c(stream_accessor* lhs, stream_accessor* rhs)
{
    for_each_channel(lhs, n) {
        c_acc sch = stream_channel_at(lhs, n);
        c_acc dch = stream_channel_at(rhs, n);
        channel_remeq(&sch, &dch);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_MULT_EQUAL
//-------------------------------------------------------------------------------------------------

void
stream_muleq(stream_accessor* lhs, stream_accessor* rhs)
{
    wpn_generic_stream_binary_op
            (lhs, rhs, stream_muleq);
}

void
stream_muleq_i(stream_accessor* lhs, stream_accessor* rhs)
{
    c_acc clhs = channel_cast(lhs);
    c_acc crhs = channel_cast(rhs);
    channel_muleq(&clhs, &crhs);
}

void
stream_muleq_c(stream_accessor* lhs, stream_accessor* rhs)
{
    for_each_channel(lhs, n) {
        c_acc sch = stream_channel_at(lhs, n);
        c_acc dch = stream_channel_at(rhs, n);
        channel_muleq(&sch, &dch);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_DIV_EQUAL
//-------------------------------------------------------------------------------------------------

void
stream_diveq(stream_accessor* lhs, stream_accessor* rhs)
{
    wpn_generic_stream_binary_op
            (lhs, rhs, stream_diveq);
}

void
stream_diveq_i(stream_accessor* lhs, stream_accessor* rhs)
{
    c_acc clhs = channel_cast(lhs);
    c_acc crhs = channel_cast(rhs);
    channel_diveq(&clhs, &crhs);
}

void
stream_diveq_c(stream_accessor* lhs, stream_accessor* rhs)
{
    for_each_channel(lhs, n) {
        c_acc sch = stream_channel_at(lhs, n);
        c_acc dch = stream_channel_at(rhs, n);
        channel_diveq(&sch, &dch);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_ADD_VALUE
//-------------------------------------------------------------------------------------------------

void
stream_addv(stream_accessor* s, sample_t const v)
{
    wpn_generic_stream_binary_op
            (s, v, stream_addv);
}

void
stream_addv_i(stream_accessor* s, sample_t const v)
{
    c_acc ch = channel_cast(s);
    channel_addv(&ch, v);
}

void
stream_addv_c(stream_accessor* s, sample_t const v)
{
    for_each_channel(s, n) {
        c_acc sch = stream_channel_at(s, n);
        channel_addv(&sch, v);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_MINUS_VALUE
//-------------------------------------------------------------------------------------------------

void
stream_remv(stream_accessor* s, sample_t const v)
{
    wpn_generic_stream_binary_op
            (s, v, stream_remv);
}

void
stream_remv_i(stream_accessor* s, sample_t const v)
{
    c_acc ch = channel_cast(s);
    channel_remv(&ch, v);
}

void
stream_remv_c(stream_accessor* s, sample_t const v)
{
    for_each_channel(s, n) {
        c_acc sch = stream_channel_at(s, n);
        channel_remv(&sch, v);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_MUL_VALUE
//-------------------------------------------------------------------------------------------------

void
stream_mulv(stream_accessor* s, sample_t const v)
{
    wpn_generic_stream_binary_op
            (s, v, stream_mulv);
}

void
stream_mulv_i(stream_accessor* s, sample_t const v)
{
    c_acc ch = channel_cast(s);
    channel_mulv(&ch, v);
}

void
stream_mulv_c(stream_accessor* s, sample_t const v)
{
    for_each_channel(s, n) {
        c_acc sch = stream_channel_at(s, n);
        channel_mulv(&sch, v);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_DIV_VALUE
//-------------------------------------------------------------------------------------------------

void
stream_divv(stream_accessor* s, sample_t const v)
{
    wpn_generic_stream_binary_op
            (s, v, stream_divv);
}

void
stream_divv_i(stream_accessor* s, sample_t const v)
{
    c_acc ch = channel_cast(s);
    channel_divv(&ch, v);
}

void
stream_divv_c(stream_accessor* s, sample_t const v)
{
    for_each_channel(s, n) {
        c_acc sch = stream_channel_at(s, n);
        channel_divv(&sch, v);
    }
}

//-------------------------------------------------------------------------------------------------
// STREAM_MIN
//-------------------------------------------------------------------------------------------------

sample_t
stream_min(stream_accessor* s)
{
    wpn_generic_stream_unary_op
            (s, stream_min);
}

sample_t
stream_min_i(stream_accessor* s)
{
    c_acc ch = channel_cast(s);
    return channel_min(&ch);
}

sample_t
stream_min_c(stream_accessor* s)
{
    sample_t m = 0;
    for_each_channel(s, n) {
        c_acc sch = stream_channel_at(s, n);
        wpnmin(m, channel_min(&sch));
    }

    return m;
}
//-------------------------------------------------------------------------------------------------
// STREAM_MAX
//-------------------------------------------------------------------------------------------------

sample_t
stream_max(stream_accessor* s)
{
    wpn_generic_stream_unary_op
            (s, stream_max);
}

sample_t
stream_max_i(stream_accessor* s)
{
    c_acc ch = channel_cast(s);
    return channel_max(&ch);
}

sample_t
stream_max_c(stream_accessor* s)
{
    sample_t m = 0;
    for_each_channel(s, n) {
        c_acc sch = stream_channel_at(s, n);
        wpnmax(m, channel_max(&sch));
    }

    return m;
}

//-------------------------------------------------------------------------------------------------
// STREAM_RMS
//-------------------------------------------------------------------------------------------------

sample_t
stream_rms(stream_accessor* s)
{
    wpn_generic_stream_unary_op
            (s, stream_rms);
}

sample_t
stream_rms_i(stream_accessor* s)
{
    c_acc ch = channel_cast(s);
    return channel_rms(&ch);
}

sample_t
stream_rms_c(stream_accessor* s)
{
    WPN114_CALL_UNIMPLEMENTED;
}

//-------------------------------------------------------------------------------------------------
// STREAM_LOOKUP
//-------------------------------------------------------------------------------------------------

wpn_error
stream_lookup(stream_accessor* receiver,
              stream_accessor* source,
              stream_accessor* head,
              bool increment)
{
    // this is the non_interleaved version
    for_each_channel(receiver, n) {
        c_acc rc = stream_channel_at(receiver, n);
        c_acc sc = stream_channel_at(source, n);
        c_acc hc = stream_channel_at(head, n);

        channel_lookup(&rc, &sc, &hc, increment);
    }

    return 0;
}


