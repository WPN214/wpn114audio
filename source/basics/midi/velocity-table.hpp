#pragma once

#include <wpn114audio/graph.hpp>

//-------------------------------------------------------------------------------------------------
class VelocityMap : public Node
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_MIDI_INPUT(midi_in, 1)
    WPN_DECLARE_DEFAULT_MIDI_OUTPUT(midi_out, 1)

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

        m_vtable[0] = 0;

        // 180° switched cosine
        for (byte_t n = 0; n < 127; ++n) {
            float phase = n/127.f;
            phase *= static_cast<float>(M_PI)*2.f;
            m_vtable[n] = static_cast<byte_t>(1.f-cosf(phase));

            fprintf(stderr, "velocity map at n=%d: %d\n", n, m_vtable[n]);
        }

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
            case 0x90:
                mt.data[1] = m_vtable[mt.data[1]];
                break;
            }

            midi_out->push(mt);
        }

    }

private:

    byte_t
    m_vtable[128];


};
