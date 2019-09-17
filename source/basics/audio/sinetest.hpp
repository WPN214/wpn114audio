#pragma once

#include <wpn114audio/qtinterface.hpp>
#include <wpn114audio/execution.hpp>
#include <wpn114audio/utilities.hpp>

#define esz 16384


namespace wpn114 {
namespace audio  {

//=================================================================================================
struct sinetest
// this is the data that will be passed on in the graph execution context
// bear in mind it has to be as small/cache-friendly as possible
// in a qml context, it will be constructed through its qt interface (see below)
//=================================================================================================
{
    enum inputs     { frequency = 0, midi_in = 1 };
    enum outputs    { audio_out = 0 };

    sample_t*
    env = nullptr;

    sample_t
    rate = 0;

    uint32_t
    phs = 0;

    ~sinetest() { delete[] env; }

    // --------------------------------------------------------------------------------------------
    static void
    initialize(sample_t rate, void* udata)
    // initialization method, all additional allocation should be done here
    // store sample rate
    // --------------------------------------------------------------------------------------------
    {
        sinetest& st = *static_cast<sinetest*>(udata);
        st.rate = rate;

        st.env = new sample_t[esz];

        for (size_t n = 0; n < esz; ++n)
             st.env[n] = std::sin(static_cast<sample_t>(M_PI)*2*
                                  static_cast<sample_t>(n)/esz);
    }

    // --------------------------------------------------------------------------------------------
    static void
    rwrite(vector_ref<abuffer_t> audio,
           vector_ref<mbuffer_t> midi,
           vector_t nframes, void* udata)
    // --------------------------------------------------------------------------------------------
    {
        auto freq  = audio[sinetest::frequency];
        auto out   = audio[sinetest::audio_out];
        auto& md   = midi[sinetest::midi_in]; // todo

        auto st = *static_cast<sinetest*>(udata);

        // process each frame
        for (vector_t f = 0; f < nframes; ++f) {
            st.phs += static_cast<uint32_t>(freq[f]/st.rate * esz);
            wpn114::audio::wrap<uint32_t>(st.phs, esz);
            out[f] = st.env[st.phs];
        }
    }
};

namespace qt {
// this now is qt's entry point

//=================================================================================================
class Sinetest : public Node
/*!
* \class Sinetest
* \brief cheap sinusoidal generator (no interpolation)
*/
//=================================================================================================
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT     (frequency, 1)
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT    (audio_out, 1)
    WPN_DECLARE_DEFAULT_MIDI_INPUT      (midi_in, 1)

    WPN_SET_EXEC (sinetest)
    WPN_SET_INIT (&sinetest::initialize)
    WPN_SET_PROC (&sinetest::rwrite)

public:

    //-------------------------------------------------------------------------------------------------
    Sinetest()
    // set default name and dispatch behaviour (if needed)
    //-------------------------------------------------------------------------------------------------
    {
        m_name      = "Sinetest";
        m_dispatch  = Dispatch::DownwardsChain;
        // this would be the default dipsatch behaviour withinin Sinetest QML scope
    }
};

}
}
}
