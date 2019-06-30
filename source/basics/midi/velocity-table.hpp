#pragma once

#include <wpn114audio/graph.hpp>

#define fmpi static_cast<float>(M_PI)

//-------------------------------------------------------------------------------------------------
class VelocityMap : public Node
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_MIDI_INPUT      (midi_in, 1)
    WPN_DECLARE_DEFAULT_MIDI_OUTPUT     (midi_out, 1)

public:

    //-------------------------------------------------------------------------------------------------
    VelocityMap() { m_name = "VelocityMap"; }

    //-------------------------------------------------------------------------------------------------
    virtual void
    initialize(Graph::properties const& properties) override
    //-------------------------------------------------------------------------------------------------
    {
        // x=y init
//        for (byte_t n = 0; n < 128; ++n)
//             m_vtable[n] = n;

        // lanczos2 window
        fprintf(stderr, "velocitymap: lanczos window\n");

        for (byte_t n = 0; n < 128; ++n) {
            float x = n/128.f-1;
            float sinc_x = std::sin(fmpi*x);
            sinc_x /= fmpi*x;
            m_vtable[n] = sinc_x*128.f;
        }

        m_vtable[0] = 1;

    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //-------------------------------------------------------------------------------------------------
    {
        auto midi_in = inputs.midi[0][0];
        auto midi_out = outputs.midi[0][0];

        for (auto& mt : *midi_in)
        {
            switch(mt.status & 0xf0) {
            case 0x90: {
                mt.data[1] = m_vtable[mt.data[1]];
                break;
            }
            }

            midi_out->push(mt);
        }

    }

private:

    byte_t
    m_vtable[128];


};
