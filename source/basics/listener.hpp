#pragma once

#include <source/graph.hpp>

//-------------------------------------------------------------------------------------------------
class Listener : public Node
// listens to the audio/midi thread and report events/data to the qml thread
//-------------------------------------------------------------------------------------------------
{
    WPN_ENUM_INPUTS (audioIn, midiIn)

    //-------------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE  (audioIn, Socket::Audio, 0)

    //-------------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE  (midiIn, Socket::Midi_1_0, 1)

    //-------------------------------------------------------------------------------------------------
    Q_PROPERTY  (qreal rate READ rate WRITE set_rate)

public:

    //-------------------------------------------------------------------------------------------------
    Listener() {}

    //-------------------------------------------------------------------------------------------------
    virtual QString
    name() const override { return "Listener"; }

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    audio(QVector<qreal> const&);

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    noteOn(unsigned int channel, unsigned int index, unsigned int velocity);

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    noteOff(unsigned int channel, unsigned int index, unsigned int velocity);

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    control(unsigned int channel, unsigned int index, unsigned int value);

    //-------------------------------------------------------------------------------------------------
    virtual void
    initialize(const Graph::properties& properties) override { m_rate = properties.rate;}

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    {
//        QMetaObject::invokeMethod(this, "noteOn", Qt::QueuedConnection, nullptr, 0, 0, 0);

    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override
    {
        m_rate = rate;
    }

private:

    size_t
    m_phase = 0;

    std::atomic<size_t>
    m_n_samples { 2048 };

    sample_t
    m_rate = 0;

};
