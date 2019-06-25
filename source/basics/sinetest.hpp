#pragma once

#include <wpn114audio/graph.hpp>

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
    WPN_DECLARE_DEFAULT_MIDI_INPUT      (midi_in, 1)
    WPN_DECLARE_DEFAULT_MIDI_OUTPUT     (audio_out, 1)

    enum inputs     { frequency = 0, midi_in = 1 };
    enum outputs    { audio_out = 0 };

    static constexpr size_t
    esz = 16384;

public:

    //-------------------------------------------------------------------------------------------------
    Sinetest()
    // set default name and dispatch behaviour (if needed)
    //-------------------------------------------------------------------------------------------------
    {
        m_name      = "Sinetest";
        m_dispatch  = Dispatch::Chain;
        // this would be the default dipsatch behaviour withinin Sinetest QML scope
    }

    //-------------------------------------------------------------------------------------------------
    virtual
    ~Sinetest() override { delete[] m_env; }

    //-------------------------------------------------------------------------------------------------
    virtual void
    initialize(const Graph::properties& properties) override
    // initialization method, all additional allocation should be done here
    // store sample rate
    //-------------------------------------------------------------------------------------------------
    {
        m_rate = properties.rate;
        m_env = new sample_t[esz];

        for (size_t n = 0; n < esz; ++n)
            m_env[n] = sin(M_PI*2*static_cast<sample_t>(n)/esz);
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override { m_rate = rate; }
    // this is called whenever Graph's sample rate changes  

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // the main processing function
    //-------------------------------------------------------------------------------------------------
    {        
        // fetch in/out buffers (first channel, as they only have one anyway)
        auto freq  = inputs.audio[0][0];
        auto midi  = inputs.midi[0]; // todo
        auto out   = outputs.audio[0][0];

        // put member attributes on the stack
        size_t phs = m_phs;
        sample_t const rate = m_rate;

        // process each frame
        for (vector_t f = 0; f < nframes; ++f) {
            phs += static_cast<size_t>(freq[f]/rate * esz);
            wpnwrap(phs, esz);
            out[f] = m_env[phs];
        }

        // update member attribute
        m_phs = phs;
    }

private:

    sample_t*
    m_env = nullptr;

    sample_t
    m_rate = 0;

    size_t
    m_phs = 0;
};
