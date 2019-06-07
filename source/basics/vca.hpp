#pragma once
#include <source/graph.hpp>

//=================================================================================================
class VCA : public Node
//=================================================================================================
{
    Q_OBJECT

    WPN_ENUM_INPUTS   (inputs, gain)
    WPN_ENUM_OUTPUTS  (outputs)

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE (inputs, Socket::Audio, 0)
    /*!
     * \property VCA::inputs
     * \brief (audio) in, multichannel expansion available
     */

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE (gain, Socket::FloatingPoint, 1)
    /*!
     * \property VCA::gainmod
     * \brief (audio) gain modulation (0-1)
     */

    //-------------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE (outputs, Socket::Audio, 0)
    /*!
     * \property VCA::outputs
     * \brief (audio) out, multichannel expansion available
     */

public:

    VCA() {}

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // this Node will benefits from multichannel expansion if needed
    //-------------------------------------------------------------------------------------------------
    {
        auto in  = inputs[Inputs::inputs];
        auto gain = inputs[Inputs::gain][0];
        auto out = outputs[Outputs::outputs];

        for (nchannels_t c = 0; c < m_outputs.nchannels(); ++c)
            for (vector_t f = 0; f < nframes; ++f)
                 out[c][f] = in[c][f] *= gain[f];
    }

private:

};
