#pragma once

#include <wpn114audio/graph.hpp>

//=================================================================================================
class VCA : public Node
//=================================================================================================
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT     (audio_in, 0)
    WPN_DECLARE_AUDIO_INPUT             (gain, 1)
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT    (audio_out, 0)

    enum inputs   { audio_in = 0, gain = 1 };
    enum outputs  { audio_out = 0 };

public:

    //-------------------------------------------------------------------------------------------------
    VCA() { m_name = "VCA"; }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // this Node will benefits from multichannel expansion if needed
    //-------------------------------------------------------------------------------------------------
    {
        auto in     = inputs.audio[VCA::audio_in][0];
        auto gain   = inputs.audio[VCA::gain][0];
        auto out    = outputs.audio[VCA::audio_out][0];

        for (vector_t f = 0; f < nframes; ++f)
             out[f] = in[f] * gain[f];
    }
};
