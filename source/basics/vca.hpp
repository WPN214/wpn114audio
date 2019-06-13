#pragma once
#include <source/graph.hpp>

//=================================================================================================
class VCA : public Node
//=================================================================================================
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_PORT(audio_in, Polarity::Input, 1)
    WPN_DECLARE_DEFAULT_AUDIO_PORT(audio_out, Polarity::Output, 1)
    WPN_DECLARE_AUDIO_PORT(gain, Polarity::Input, 1)

    constexpr static int
    audio_in    = 0,
    audio_out   = 0,
    gain        = 1;

public:

    //-------------------------------------------------------------------------------------------------
    VCA() {}

    //-------------------------------------------------------------------------------------------------
    virtual QString
    name() const override { return "VCA"; }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // this Node will benefits from multichannel expansion if needed
    //-------------------------------------------------------------------------------------------------
    {
        auto in     = inputs[VCA::audio_in][0];
        auto gain   = inputs[VCA::gain][0];
        auto out    = outputs[VCA::audio_out][0];

        for (vector_t f = 0; f < nframes; ++f)
            out[f] = in[f] * gain[f];
    }

private:

};
