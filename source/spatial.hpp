#pragma once

#include <QObject>
#include "graph.hpp"

//=================================================================================================
struct spatial_t
// TODO
//=================================================================================================
{
    qreal x = 0, y = 0, z = 0;
    qreal w = 0, d = 0, h = 0;
};

class SpatialProcessor;

//=================================================================================================
class Spatial : public Node
// a satellite node
//=================================================================================================
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT  (audio_in, 0)

    WPN_DECLARE_AUDIO_INPUT   (width, 1)
    WPN_DECLARE_AUDIO_INPUT   (height, 1)
    WPN_DECLARE_AUDIO_INPUT   (depth, 1)
    WPN_DECLARE_AUDIO_INPUT   (angle, 1)
    WPN_DECLARE_AUDIO_INPUT   (curve, 1)
    WPN_DECLARE_AUDIO_INPUT   (x, 1)
    WPN_DECLARE_AUDIO_INPUT   (y, 1)

    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT (audio_out, 0)

    Q_PROPERTY (Node* processor READ processor WRITE set_processor)

public:

    enum Alignment
    {
        Left        = 0,
        Centered    = 1,
        Right       = 2,
        Top         = 3,
        Bottom      = 4
    };

    Q_ENUM(Alignment)

    // --------------------------------------------------------------------------------------------
    Spatial()
    {
        m_name = "Spatial";
        m_dispatch = Dispatch::Downwards;
    }

    // --------------------------------------------------------------------------------------------
    Node* processor();

    // --------------------------------------------------------------------------------------------
    void
    set_processor(Node* processor);
    // --------------------------------------------------------------------------------------------

    virtual void
    componentComplete() override
    {
        qDebug() << "Spatial component complete";
        Node::componentComplete();
    }

    // --------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // update each channel position
    // copy inputs to outputs
    // --------------------------------------------------------------------------------------------
    {

    }

    // --------------------------------------------------------------------------------------------
    std::vector<spatial_t>&
    positions() { return m_positions; }

private:
    SpatialProcessor* m_processor = nullptr;
    std::vector<spatial_t>
    m_positions;
};

//=================================================================================================
class SpatialProcessor : public Node
//=================================================================================================
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT     (audio_in, 0)
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT    (audio_out, 0)

public:
    virtual void
    add_input(Spatial* input) { m_inputs.push_back(input); }

protected:
    std::vector<Spatial*> m_inputs;

};

//=================================================================================================
class StereoPanner : public SpatialProcessor
// simple sqrt-based stereo panner
//=================================================================================================
{
    Q_OBJECT

public:

    //---------------------------------------------------------------------------------------------
    StereoPanner()
    {
        m_name = "StereoPanner";
        default_port(Polarity::Input)->set_nchannels(1);
        default_port(Polarity::Output)->set_nchannels(2);
    }

    //---------------------------------------------------------------------------------------------
    virtual void WPN_CLEANUP
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    {
        auto in = inputs.audio[0][0]; // the input buffer (mono)
        auto out = outputs.audio[0]; // the output buffer (stereo)
        auto x = m_inputs[0]->m_x.buffer<audiobuffer_t>()[0];

        for (vector_t f = 0; f < nframes; ++f) {
            out[0][f] = sqrt(in[f]*x[f]);
            out[1][f] = sqrt(in[f]*(1-x[f]));
        }
    }
};
