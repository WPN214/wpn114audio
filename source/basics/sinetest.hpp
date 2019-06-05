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
    WPN_OBJECT
    // this marks initialize and rwrite as overriden

    //-------------------------------------------------------------------------------------------------
    WPN_ENUM_INPUTS  (frequency, midi)
    WPN_ENUM_OUTPUTS (output)
    // index 0 of enum is always default input/output

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (frequency, Socket::Audio, 1)
    /*!
     * \property Sinetest::frequency
     * \brief (audio, input) in Hertz
     */

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (midi, Socket::Midi_1_0, 1)

    //-------------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE (output, Socket::Audio, 1)
    /*!
     * \property Sinetest::output
     * \brief (audio, output) main out
     */

    static constexpr size_t
    esz = 16384;

public:

    //-------------------------------------------------------------------------------------------------
    Sinetest()
    // ctor, do what you want here
    //-------------------------------------------------------------------------------------------------
    {
        m_env   = new sample_t[esz];
        m_rate  = Graph::rate();

        for (size_t n = 0; n < esz; ++n)
            m_env[n] = sin(M_PI*2*n/esz);
    }

    virtual
    ~Sinetest() override
    {
        delete[] m_env;
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // the main processing function
    //-------------------------------------------------------------------------------------------------
    {
        auto frequency  = inputs[Inputs::frequency][0];
        auto midi       = inputs[Inputs::midi][0];
        auto out        = outputs[Outputs::output][0];

        for (vector_t f = 0; f < nframes; ++f) {
            m_phs += frequency[f]/m_rate * esz;
            out[f] = m_env[m_phs];
        }
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override { m_rate = rate; }
    // this is called whenever Graph's sample rate changes

private:

    sample_t* m_env = nullptr;
    sample_t m_rate = 0;
    size_t m_phs = 0;
};
