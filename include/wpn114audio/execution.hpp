#pragma once

#include <wpn114audio/basic_types.hpp>
#include <thread>

// a data-oriented audio execution graph system
// which, ideally, could be built at compile-time,
// as we'd want this to run on embedded devices

namespace wpn114 {
namespace audio  {

//-------------------------------------------------------------------------------------------------
struct connection
// a ticket-like convenience struct, holding references to graph's connections
//-------------------------------------------------------------------------------------------------
{
    connection(vector_t source_id, vector_t dest_id,
               vector_t source_port, vector_t dest_port,
               std::atomic<sample_t>* mulref,
               std::atomic<sample_t>* addref) noexcept :
        s_id(source_id), s_pid(source_port),
        d_id(dest_id), d_pid(dest_port),
        m_mul(mulref), m_add(addref) {}

    connection&
    operator=(connection const& con)
    {
        s_id    = con.s_id;
        d_id    = con.d_id;
        s_pid   = con.s_pid;
        d_pid   = con.d_pid;
        return  *this;
    }

    void
    set_mul(sample_t mul) { m_mul->store(mul); }

    void
    set_add(sample_t add) { m_add->store(add); }

    vector_t
    s_id, d_id, s_pid, d_pid;

    std::atomic<sample_t>
    *m_mul, *m_add;
};

//-------------------------------------------------------------------------------------------------
struct node
//-------------------------------------------------------------------------------------------------
{
    node(int_fn_t int_fn, prc_fn_t prc_fn, void* udata,
         nchannels_t n_ai, nchannels_t n_ao,
         nchannels_t n_mi, nchannels_t n_mo) noexcept :

        m_int_fn(int_fn), m_prc_fn(prc_fn), m_udata(udata),
        m_n_ai(n_ai), m_n_ao(n_ao),
        m_n_mi(n_mi), m_n_mo(n_mo) {}

    nchannels_t
    m_n_ai, m_n_ao, m_n_mi, m_n_mo;

    uint32_t
    m_id = 0;

    int_fn_t m_int_fn;
    prc_fn_t m_prc_fn;
    void* m_udata;
};

//-------------------------------------------------------------------------------------------------
struct xstage_a {
// audio preprocessing stage part.II (connection pull)
//-------------------------------------------------------------------------------------------------
    wpn114::vector_ref<abuffer_t> source;
    wpn114::vector_ref<abuffer_t> dest;
    wpn114::vector_ref<std::atomic<sample_t>> mul;
    wpn114::vector_ref<std::atomic<sample_t>> add;
};

//-------------------------------------------------------------------------------------------------
struct xstage_m {
// midi preprocessing stage (connection pull)
//-------------------------------------------------------------------------------------------------
    wpn114::vector_ref<mbuffer_t> source;
    wpn114::vector_ref<mbuffer_t> dest;
};

//-------------------------------------------------------------------------------------------------
struct xstage_p {
// audio/midi processing stage
//-------------------------------------------------------------------------------------------------
    wpn114::vector_ref<wpn114::vector_ref<abuffer_t>> audio;
    wpn114::vector_ref<wpn114::vector_ref<mbuffer_t>> midi;
    wpn114::vector_ref<prc_fn_t> processors;
    wpn114::vector_ref<void*> udatas;
};

//-------------------------------------------------------------------------------------------------
struct xstage {
//-------------------------------------------------------------------------------------------------
    xstage_a audio;
    xstage_m midi;
    xstage_p process;
};

//-------------------------------------------------------------------------------------------------
struct xdata
//-------------------------------------------------------------------------------------------------
{
    xdata(mempool& mp) noexcept :
        ai_buffers(mp), ao_buffers(mp),
        mi_buffers(mp), mo_buffers(mp),
        muls(mp), adds(mp), values(mp),
        processors(mp), udatas(mp), xstages(mp) {}

    wpn114::vector<std::atomic<sample_t>>
    values, muls, adds;

    wpn114::vector<prc_fn_t>
    processors;

    wpn114::vector<void*>
    udatas;

    wpn114::vector<abuffer_t>
    ai_buffers, ao_buffers;

    wpn114::vector<mbuffer_t>
    mi_buffers, mo_buffers;

    wpn114::vector<xstage>
    xstages;
};

static const uint32_t
nthreads = std::thread::hardware_concurrency();

//-------------------------------------------------------------------------------------------------
struct graph
//-------------------------------------------------------------------------------------------------
{
    graph(mempool& pool) noexcept :
        m_mempool(pool),
        m_nodes(wpn114::vector<node>(pool)),
        m_connections(wpn114::vector<connection>(pool)),
        m_xdata(pool) {}

    node&
    register_node(int_fn_t int_fn, prc_fn_t prc_fn, void* udata,
                  nchannels_t n_ai, nchannels_t n_ao,
                  nchannels_t n_mi, nchannels_t n_mo) noexcept
    {
        node n(int_fn, prc_fn, udata, n_ai, n_ao, n_mi, n_mo);
        n.m_id = m_nodes.count();

        m_nodes.push(n);
        m_xdata.processors.push(prc_fn);
        m_xdata.udatas.push(udata);
        return m_nodes.last();
    }

    connection&
    pconnect(node const& src, nchannels_t s_port,
             node const& dst, nchannels_t d_port) noexcept
    {
        connection c(src.m_id, dst.m_id, s_port, d_port, nullptr, nullptr);
        m_connections.push(c);
        return m_connections.last();
    }

    void
    initialize(sample_t rate, vector_t maxbs)  noexcept
    {
        m_rate    = rate;
        m_vector  = maxbs;

        vector_t n_ai = 0, n_ao = 0, n_mi = 0, n_mo = 0;

        for (const auto& n : m_nodes) {
             n_ai += n.m_n_ai;
             n_ao += n.m_n_ao;
             n_mi += n.m_n_mi;
             n_mi += n.m_n_mo;
        }

        // reserve allocation space from mempool
        // audio inputs & offset values
        m_xdata.ai_buffers.reserve(n_ai);
        m_xdata.values.reserve(n_ai);

        for (nchannels_t c = 0; c < n_ai; ++c)  {
             abuffer_t b = m_mempool.request<sample_t>(maxbs, nullptr);
             m_xdata.ai_buffers.push(b);
             m_xdata.values.pull().store(0);
        }

        // audio outputs
        m_xdata.ao_buffers.reserve(n_ao);
        for (nchannels_t c = 0; c < n_ao; ++c)  {
             abuffer_t b = m_mempool.request<sample_t>(maxbs, nullptr);
             m_xdata.ai_buffers.push(b);
        }

        // midi inputs
        m_xdata.mi_buffers.reserve(n_mi);
        for (nchannels_t c = 0; c < n_mi; ++c)  {
             mbuffer_t b(m_mempool, sizeof(sample_t)*maxbs);
             m_xdata.mi_buffers.push(b);
        }

        // midi outputs
        m_xdata.mo_buffers.reserve(n_mo);
        for (nchannels_t c = 0; c < n_mo; ++c)  {
             mbuffer_t b(m_mempool, sizeof(sample_t)*maxbs);
             m_xdata.mo_buffers.push(b);
        }

        // connection mul/adds
        m_xdata.adds.reserve(m_connections.count());
        m_xdata.muls.reserve(m_connections.count());

        for (auto& con : m_connections)  {
            m_xdata.adds.pull().store(0);
            m_xdata.muls.pull().store(1);
            con.m_add = &m_xdata.adds.last();
            con.m_mul = &m_xdata.muls.last();
        }
    }

    void
    run(vector_t nframes) noexcept
    {
        assert(m_xdata.ai_buffers.count() ==  m_xdata.values.count());
        // pull value for each audio input buffer
        // doesn't matter the order, so we'll do it linearly

        std::thread threads[nthreads];

        for (vector_t n = 0; n < m_xdata.ai_buffers.count(); ++n)
             threads[n%nthreads] = std::thread([n, nframes, this]() {
                 for(vector_t f = 0; f < nframes; ++f)
                     m_xdata.ai_buffers[n][f] = m_xdata.values[n];
                  }); // ok

        for (uint32_t n = 0; n < nthreads; ++n) {
             threads[n].join();
        }

        // now for each stage,
        // pull audio/midi input connections, muladd signal if audio
        // process
        for (auto& xstage : m_xdata.xstages)
        {
            const auto nt2 = nthreads/2 == 0 ? 1 : nthreads/2;

            // audio/midi are ideally processed on different threads
            for (vector_t a = 0; a < xstage.audio.dest.count(); ++a) {
                threads[a%nt2] = std::thread([a, nframes, &xstage]() {
                    auto& src = xstage.audio.source[a];
                    auto& dst = xstage.audio.dest[a];
                    auto mul = xstage.audio.mul[a].load();
                    auto add = xstage.audio.add[a].load();

                    for (vector_t f = 0; f < nframes; ++f)
                         dst[f] = src[f]*mul+add;
                });
            }

            for (vector_t m = 0; m < xstage.midi.dest.count(); ++m) {
                threads[m%nt2+nt2] = std::thread([m, &xstage]() {
                    auto& src = xstage.midi.source[m];
                    auto& dst = xstage.midi.dest[m];
                    for (const auto& ev : src)
                         dst.push(ev);
                });
            }

            // join & redispatch threads for processing
            for (uint32_t n = 0; n < nthreads; ++n) {
                 threads[n].join();
            }

            for (vector_t p = 0; p < xstage.process.udatas.count(); ++p) {
                threads[p%nthreads] = std::thread([p, nframes, &xstage] {
                    auto& udata = xstage.process.udatas[p];
                    auto& prcfn = xstage.process.processors[p];
                    prcfn(xstage.process.audio[p],
                          xstage.process.midi[p],
                          nframes, udata);
                });
            }

            for (uint32_t n = 0; n < nthreads; ++n) {
                 threads[n].join();
            }
        }
    }

private:

    sample_t
    m_rate = 0;

    vector_t
    m_vector = 0;

    mempool&
    m_mempool;

    wpn114::vector<node>
    m_nodes;

    wpn114::vector<connection>
    m_connections;

    xdata
    m_xdata;
};
}
}
