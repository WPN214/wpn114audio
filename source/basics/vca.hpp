#pragma once
#include <source/graph.hpp>

//=================================================================================================
class VCA : public Node
//=================================================================================================
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    VCA() {}

    //-------------------------------------------------------------------------------------------------
    virtual QString
    name() const override { return "VCA"; }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // this Node will benefits from multichannel expansion if needed
    //-------------------------------------------------------------------------------------------------
    {
        auto in     = inputs[0][0];
        auto gain   = inputs[1][0];
        auto out    = outputs[0][0];

        for (vector_t f = 0; f < nframes; ++f)
            out[f] = in[f] * gain[f];
    }

private:

};
