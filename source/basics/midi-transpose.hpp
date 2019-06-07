#pragma once
#include <source/graph.hpp>

//---------------------------------------------------------------------------------------------
class MidiTransposer : public Node
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    WPN_ENUM_INPUTS     (inputs, transpose)
    WPN_ENUM_OUTPUTS    (outputs)

    //---------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (inputs, Socket::Midi_1_0, 0)

    //---------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (transpose, Socket::Integer, 1)

    //---------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE  (outputs, Socket::Midi_1_0, 0)


public:

    //---------------------------------------------------------------------------------------------
    MidiTransposer() {}

    //---------------------------------------------------------------------------------------------
    virtual QString
    name() const override { return "MidiTransposer"; }

    //---------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //---------------------------------------------------------------------------------------------
    {
        auto in = inputs[Inputs::inputs];
        auto transpose = inputs[Inputs::transpose][0];
        auto out = outputs[Outputs::outputs];

        for (vector_t n = 0; n < nframes; ++n)
        {
            midi_t mt = in[0][n];
            uint8_t tp = static_cast<uint8_t>(transpose[n]);

            if (mt.status == 0x90)
                mt.b1 += tp;

            out[0][n] = mt;
        }
    }
};
