#pragma once
#include <wpn114audio/graph.hpp>


//=================================================================================================
class PeakRMS : public Node
//=================================================================================================
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT     (audio_in, 0)
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT    (audio_out, 0)

    WPN_DECLARE_AUDIO_OUTPUT            (peak, 0)
    WPN_DECLARE_AUDIO_OUTPUT            (rms, 0)

    audiobuffer_t
    m_buffer = nullptr;

    sample_t
    m_refresh = 20;

    vector_t
    m_block_size = 0;

    size_t
    m_pos = 0;

    nchannels_t
    m_nchannels = 0;

    QVector<sample_t>
    m_peak_out;

    QVector<sample_t>
    m_rms_out;

public:

    //---------------------------------------------------------------------------------------------
    PeakRMS()
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    virtual
    ~PeakRMS() override
    //---------------------------------------------------------------------------------------------
    {
        for (nchannels_t c = 0; c < m_nchannels; ++c)
            delete[] m_buffer[c];

        delete[] m_buffer;
    }

    //---------------------------------------------------------------------------------------------
    Q_SIGNAL void
    peak(QVector<sample_t> peak);

    Q_SIGNAL void
    rms(QVector<sample_t> rms);

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {
        Node::componentComplete();

        // this would be the model for multichannel expansion
        // this has to be generalized
        nchannels_t nchannels = 0;
        auto connections = m_audio_in.connections();

        for (auto& connection : connections)
            nchannels = std::max(nchannels, connection->source()->nchannels());

        m_audio_in.set_nchannels(nchannels);
        m_audio_out.set_nchannels(nchannels);
        m_peak.set_nchannels(nchannels);
        m_rms.set_nchannels(nchannels);

        m_nchannels = nchannels;
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    initialize(Graph::properties const& properties) override
    //---------------------------------------------------------------------------------------------
    {
        m_block_size = properties.rate/m_refresh;
        m_buffer = wpn114::allocate_buffer<audiobuffer_t>(m_nchannels, properties.vector);

        m_peak_out.fill(0, m_nchannels);
        m_rms_out.fill(0, m_nchannels);
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override
    //---------------------------------------------------------------------------------------------
    {
        m_block_size = rate/m_refresh;
    }

    //---------------------------------------------------------------------------------------------
    void
    buffer_complete(vector_t nframes, audiobuffer_t peak_out, audiobuffer_t rms_out)
    //---------------------------------------------------------------------------------------------
    {
        auto nchannels = m_nchannels;
        auto pos = m_pos;
        auto block = m_buffer;

        // reset output buffers
        m_peak_out.fill(0, nchannels);
        m_rms_out.fill(0, nchannels);

        for (nchannels_t c = 0; c < nchannels; ++c)
        {
            for (vector_t f = 0; f < nframes; ++f) {
                m_peak_out[c] = std::max<sample_t>(m_peak_out[c], m_buffer[c][f]);
                m_rms_out[c] += std::pow<sample_t>(m_buffer[c][f], 2);
            }

            m_rms_out[c]   = std::sqrt(1/nframes*m_rms_out[c]);
            m_peak_out[c]  = std::log10(m_peak_out[c])* 20;
            m_rms_out[c]   = std::log10(m_rms_out[c])*20;

            // fill peak/rms outputs
            for (vector_t f = 0; f < nframes; ++f) {
                peak_out[c][f] = m_peak_out[c];
                rms_out[c][f] = m_rms_out[c];
            }
        }

        // async signal outputs
        QMetaObject::invokeMethod(this, "peak", Qt::QueuedConnection,
            Q_ARG(QVector<sample_t>, m_peak_out));

        QMetaObject::invokeMethod(this, "rms", Qt::QueuedConnection,
            Q_ARG(QVector<sample_t>, m_rms_out));
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //---------------------------------------------------------------------------------------------
    {
        auto in     = inputs.audio[0];
        auto out    = outputs.audio[0];
        auto peak   = outputs.audio[1];
        auto rms    = outputs.audio[2];

        const auto nchannels    = m_nchannels;
        auto pos                = m_pos;
        auto buffer             = m_buffer;
        auto bsize              = m_block_size;

        int32_t rest = bsize-(pos+nframes);

        if (rest)
        {
            for (nchannels_t c = 0; c < nchannels; ++c)
                for (vector_t f = 0; f < nframes; ++c)
                    m_buffer[c][f] = in[c][f];

            m_pos += nframes;
        }

        else
        {
            for (nchannels_t c = 0; c < nchannels; ++c)
                for (vector_t f = 0; f < nframes+rest; ++f)
                    m_buffer[c][m_pos+f] = in[c][f];

            // dispatch buffer to outputs
            buffer_complete(nframes, peak, rms);

            // reset buffer and start filling again
            // with the rest of the current block
            for (nchannels_t c = 0; c < nchannels; ++c)
                for (vector_t f = 0; f < nframes; ++f)
                    m_buffer[c][f] = 0;

            for (nchannels_t c = 0; c < nchannels; ++c)
                for (vector_t f = 0; f < -rest; ++f)
                    m_buffer[c][f] = in[c][nframes+rest+f];

            m_pos = -rest;
        }

        // pass through input buffer
        // latch output for peak/rms
        for (nchannels_t c = 0; c < nchannels; ++c)
            for (vector_t f = 0; f < nframes; ++f) {
                 out[c][f] = in[c][f];
                 peak[c][f] = m_peak_out[c];
                 rms[c][f] = m_rms_out[c];
            }
    }
};
