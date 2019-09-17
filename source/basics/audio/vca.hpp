#pragma once

#include <wpn114audio/execution.hpp>
#include <wpn114audio/qtinterface.hpp>

namespace wpn114
{
namespace audio
{
namespace qt
{

// note: we don't declare any independant data structure here for the simple
// reason we have no data to actually store during execution

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
    void
    rwrite(vector_ref<abuffer_t> audio,
           vector_ref<mbuffer_t>,
           vector_t nframes, void*)
    // this Node will benefits from multichannel expansion if needed
    {
        auto in     = audio[VCA::audio_in];
        auto gain   = audio[VCA::gain];
        auto out    = audio[VCA::audio_out];

        for (vector_t f = 0; f < nframes; ++f)
             out[f] = in[f] * gain[f];
    }
};

}
}
}
