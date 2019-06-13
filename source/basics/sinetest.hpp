#pragma once
#include <source/graph.hpp>

//=================================================================================================
class Sinetest : public Node
/*!
* \class Sinetest
* \brief cheap sinusoidal generator
*/
//=================================================================================================
{
    Q_OBJECT

    WPN_PORT (Port::Audio, Polarity::Input, frequency, true, 1)
    WPN_PORT (Port::Midi_1_0, Polarity::Input, midi_in, false, 1)
    WPN_PORT (Port::Audio, Polarity::Output, audio_out, true, 1)

    static constexpr int
    frequency = 0,
    midi_in = 1,
    audio_out = 0;


    //-------------------------------------------------------------------------------------------------
    static constexpr size_t
    esz = 16384;

public:

    //-------------------------------------------------------------------------------------------------
    Sinetest() { m_dispatch = Dispatch::Values::Downwards; }
    // this would be the default dipsatch behaviour withinin Sinetest QML scope

    //-------------------------------------------------------------------------------------------------
    virtual
    ~Sinetest() override { delete[] m_env; }

    //-------------------------------------------------------------------------------------------------
    virtual QString
    name() const override { return "Sinetest"; }

    //-------------------------------------------------------------------------------------------------
    virtual void
    initialize(const Graph::properties& properties) override
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
        auto frequency = inputs[Sinetest::frequency][0];
        auto midi = inputs[Sinetest::midi_in][0];
        auto out = outputs[Sinetest::audio_out][0];

        size_t phs = m_phs;
        sample_t const rate = m_rate;

        for (vector_t f = 0; f < nframes; ++f) {
            phs += static_cast<size_t>(frequency[f]/rate * esz);
            wpnwrap(phs, esz);
            out[f] = m_env[phs];
        }

        m_phs = phs;
    }

private:

    sample_t* m_env = nullptr;
    sample_t m_rate = 0;
    size_t m_phs = 0;
};
