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

    //-------------------------------------------------------------------------------------------------
    WPN_ENUM_INPUTS     (frequency, midiInput)
    WPN_ENUM_OUTPUTS    (audioOutput)
    // index 0 of enum is always default input/output

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (frequency, Socket::FloatingPoint, 1)
    /*!
     * \property Sinetest::frequency
     * \brief (audio/control, input) in Hertz
     */

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (midiInput, Socket::Midi_1_0, 1)
    /*!
     * \property Sinetest::midiInput
     * \brief (audio, input) midi, controls frequency of the sine
     */

    //-------------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE (audioOutput, Socket::Audio, 1)
    /*!
     * \property Sinetest::audioOutput
     * \brief (audio, output) main out
     */

    static constexpr size_t
    esz = 16384;

public:
    //-------------------------------------------------------------------------------------------------
    Sinetest() {}
    // ctor, do what you want here

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
        auto frequency  = inputs[Inputs::frequency][0];
        auto midi_in    = inputs[Inputs::midiInput][0];
        auto out        = outputs[Outputs::audioOutput][0];

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
