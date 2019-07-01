#pragma once 

#include <wpn114audio/graph.hpp>

//-------------------------------------------------------------------------------------------------
class Clock : public Node
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT(tempo, 1)
    WPN_DECLARE_AUDIO_INPUT(signature, 1)
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT(clock_out, 1)

    size_t
    m_phase = 0;

    sample_t
    m_rate = 0;

public:

    //---------------------------------------------------------------------------------------------
    Clock()
    //---------------------------------------------------------------------------------------------
    {
        m_name      = "Clock";
        m_dispatch  = Dispatch::Chain;
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    initialize(const Graph::properties &properties) override
    //---------------------------------------------------------------------------------------------
    {
        m_rate = properties.rate;
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override
    //---------------------------------------------------------------------------------------------
    {
        m_rate = rate;
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //---------------------------------------------------------------------------------------------
    {
        auto tempo  = inputs.audio[0][0];
        auto signature = inputs.audio[1][0];
        auto out    = outputs.audio[0][0];
        auto rate   = m_rate;
        auto phs    = m_phase;

        for (vector_t f = 0; f < nframes; ++f)
        {
            auto sig = 0.25f/signature[f];
            auto secpb = 60/tempo[f];

            size_t smppb = secpb*rate;

            if (phs >= smppb) {
                phs -= smppb;
                out[f] = 1;
            } else {
                phs++;
                out[f] = 0;
            }
        }

        m_phase = phs;
    }
};
