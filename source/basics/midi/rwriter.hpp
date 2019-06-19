#pragma once

#include <source/graph.hpp>
#include <source/rbuffer.hpp>

//-------------------------------------------------------------------------------------------------
class Gateway : public Node
// listens to the audio/midi thread and report events/data back to the qt gui/qml thread
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_MIDI_INPUT      (midi_in, 1)
    WPN_DECLARE_DEFAULT_MIDI_OUTPUT     (midi_out, 1)

public:

    //-------------------------------------------------------------------------------------------------
    Gateway() { m_name = "MidiRw"; }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_note_on(unsigned int channel, unsigned int index, unsigned int velocity)
    //-------------------------------------------------------------------------------------------------
    {
        // append event to the buffer
        midi_t mt;
        mt.status = 0x90+channel;
        mt.nbytes = 2;
        mt.frame = m_frame.load();
        mt.data[0] = index;
        mt.data[1] = velocity;

        wpn_midibuffer_push(m_outbuffer, &mt);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_note_off(unsigned int channel, unsigned int index, unsigned int velocity)
    //-------------------------------------------------------------------------------------------------
    {

    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_control(unsigned int channel, unsigned int index, unsigned int value)
    //-------------------------------------------------------------------------------------------------
    {

    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_program(unsigned int channel, unsigned int index)
    //-------------------------------------------------------------------------------------------------
    {

    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_aftertouch(unsigned int channel, unsigned int index, unsigned int value)
    //-------------------------------------------------------------------------------------------------
    {

    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_pressure(unsigned int channel, unsigned int value)
    //-------------------------------------------------------------------------------------------------
    {

    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_bend(unsigned int channel, unsigned int value)
    //-------------------------------------------------------------------------------------------------
    {

    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_sysex(QVariantList list)
    //-------------------------------------------------------------------------------------------------
    {

    }

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
    Q_SIGNAL void
    program(unsigned int channel, unsigned int index);

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    aftertouch(unsigned int channel, unsigned int index, unsigned int value);

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    pressure(unsigned int channel, unsigned int value);

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    bend(unsigned int channel, unsigned int value);

    //-------------------------------------------------------------------------------------------------
    virtual void
    initialize(const Graph::properties& properties) override
    {
        m_outbuffer = wpn_midibuffer_alloc(properties.vector/2);
        m_rate = properties.rate;
    }

    //-------------------------------------------------------------------------------------------------
    void
    invoke_out_signal_3(const char* signal, byte_t channel, byte_t b1, byte_t b2)
    //-------------------------------------------------------------------------------------------------
    {
        QMetaObject::invokeMethod(this, signal, Qt::QueuedConnection,
            Q_ARG(unsigned int, channel), Q_ARG(unsigned int, b1),
            Q_ARG(unsigned int, b2));
    }

    //-------------------------------------------------------------------------------------------------
    void
    invoke_out_signal_2(const char* signal, byte_t channel, byte_t b1)
    //-------------------------------------------------------------------------------------------------
    {
        QMetaObject::invokeMethod(this, signal, Qt::QueuedConnection,
            Q_ARG(unsigned int, channel), Q_ARG(unsigned int, b1));
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    {
        auto midi_events = inputs.midi[0];
        auto midi_out = outputs.midi[0];

        byte_t* buffer = nullptr;

        for (vector_t n = 0; n < midi_events->nelem; ++n)
        {
            // parse midi event
            midi_t* mt = wpn_midibuffer_at(midi_events, n);

            if ((mt->status & 0xf0) == 0x80)
                invoke_out_signal_3("noteOff", 0, mt->data[0], mt->data[1]);

            else if ((mt->status & 0xf0) == 0x90)
                invoke_out_signal_3("noteOn", 0, mt->data[0], mt->data[1]);

            else if ((mt->status & 0xf0) == 0xa0)
                invoke_out_signal_3("aftertouch", 0, mt->data[0], mt->data[1]);

            else if ((mt->status & 0xf0) == 0xb0)
                invoke_out_signal_3("control", 0, mt->data[0], mt->data[1]);

            else if ((mt->status & 0xf0) == 0xc0)
                invoke_out_signal_2("program", 0, mt->data[0]);

            else if ((mt->status & 0xf0) == 0xd0)
                invoke_out_signal_2("pressure", 0, mt->data[0]);

            else if ((mt->status & 0xf0) == 0xe0)
                invoke_out_signal_2("bend", 0, mt->data[0]);

            m_frame++;
        }

        m_frame.store(0);

        // write output data
        for (vector_t n = 0; n < m_outbuffer->nelem; ++n)
        {
            midi_t* mt = wpn_midibuffer_at(m_outbuffer, n);
            wpn_midibuffer_push(midi_out, mt);
        }

        wpn_midibuffer_clear(m_outbuffer);
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override { m_rate = rate; }

private:

    size_t
    m_phase = 0;

    sample_t
    m_rate = 0;

    midibuffer_t*
    m_outbuffer = nullptr;

    std::atomic<vector_t>
    m_frame;

};
