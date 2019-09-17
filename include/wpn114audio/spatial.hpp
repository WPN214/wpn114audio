#pragma once

#include <wpn114audio/execution.hpp>
#include <wpn114audio/qtinterface.hpp>

class SpatialProcessor;

//=================================================================================================
class Spatial : public Node
// a satellite node
//=================================================================================================
{
    Q_OBJECT

    WPN_DECLARE_AUDIO_INPUT   (h_orientation, 1)
    WPN_DECLARE_AUDIO_INPUT   (v_orientation, 1)
    WPN_DECLARE_AUDIO_INPUT   (directivity, 1)
    WPN_DECLARE_AUDIO_INPUT   (x, 1)
    WPN_DECLARE_AUDIO_INPUT   (y, 1)
    WPN_DECLARE_AUDIO_INPUT   (z, 1)

public:

    // --------------------------------------------------------------------------------------------
    enum Align
    // this might be useful later on for fixed sources
    // --------------------------------------------------------------------------------------------
    {
        Left        = 0,
        Center      = 1,
        Right       = 3,
        Top         = 4,
        Bottom      = 5
    };

    Q_ENUM(Align)

    // --------------------------------------------------------------------------------------------
    enum Directivity
    // not implemented yet
    // --------------------------------------------------------------------------------------------
    {
        HyperCardioid    = 0,
        Cardioid         = 1,
        Omnidirectional  = 2,
    };

    Q_ENUM(Directivity)

    // --------------------------------------------------------------------------------------------
    Spatial(nchannels_t nchannels) : m_nchannels(nchannels)
    // --------------------------------------------------------------------------------------------
    {
        m_name = "Spatial";        
        m_x.set_nchannels(nchannels);
        m_y.set_nchannels(nchannels);
        m_z.set_nchannels(nchannels);
        m_h_orientation.set_nchannels(nchannels);
        m_v_orientation.set_nchannels(nchannels);
        m_directivity.set_nchannels(nchannels);
    }

    // --------------------------------------------------------------------------------------------
    nchannels_t
    nchannels() const { return m_nchannels; }

    // --------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    // --------------------------------------------------------------------------------------------
    {
        qDebug() << "Spatial component complete";
        Node::componentComplete();
    }

private:

    nchannels_t
    m_nchannels = 0;

};

//=================================================================================================
class SpatialProcessor : public Node
//=================================================================================================
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT     (audio_in, 0)
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT    (audio_out, 0)

public:

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {
        Node::componentComplete();
        nchannels_t nchannels = 0;

        for (auto& node : m_subnodes) {
            m_spatial_inputs.push_back(node->spatial());
            nchannels += node->spatial()->nchannels();
        }

        m_nchannels = nchannels;
        m_audio_in.set_nchannels(nchannels);
        m_audio_out.set_nchannels(nchannels);
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool &inputs, pool &outputs, vector_t nframes) override
    //---------------------------------------------------------------------------------------------
    {
        // otherwise, spatial node won't be processed in time
        for (auto& input: m_spatial_inputs)
             input->process(nframes);
    }

protected:    
    std::vector<Spatial*>
    m_spatial_inputs;

    nchannels_t
    m_nchannels = 0;

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
    //---------------------------------------------------------------------------------------------
    {
        m_name = "StereoPanner";
        m_audio_in.set_nchannels(1);
        m_audio_out.set_nchannels(2);
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //---------------------------------------------------------------------------------------------
    {
        auto in     = inputs.audio[0][0]; // the input buffer (mono)
        auto out    = outputs.audio[0]; // the output buffer (stereo)
        auto x      = m_spatial_inputs[0]->m_x.buffer<audiobuffer_t>()[0];

        for (vector_t f = 0; f < nframes; ++f) {
            out[0][f] = sqrt(in[f]*x[f]);
            out[1][f] = sqrt(in[f]*(1-x[f]));
        }
    }
};
