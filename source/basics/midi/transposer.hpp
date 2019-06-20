#pragma once
#include <source/graph.hpp>

//---------------------------------------------------------------------------------------------
class MidiTransposer : public Node
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_MIDI_INPUT(midi_in, 1)
    WPN_DECLARE_AUDIO_INPUT(transpose, 1)
    WPN_DECLARE_DEFAULT_MIDI_OUTPUT(midi_out, 1)

public:

    //---------------------------------------------------------------------------------------------
    MidiTransposer() { m_name = "MidiTransposer"; }

    //---------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //---------------------------------------------------------------------------------------------
    {
        auto in = inputs.midi[0];
        auto transpose = inputs.audio[0][0];
        auto out = outputs.midi[0];

        for (auto& event : *in)
        {
            switch(event.status & 0xf0) {
            case 0x80: case 0x90:
                event.data[0] += static_cast<byte_t>(transpose[event.frame]);
            break;
            }

            out->push(event);
        }
    }
};
