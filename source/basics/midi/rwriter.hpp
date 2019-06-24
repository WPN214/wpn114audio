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
    Gateway() { m_name = "MidiGateway"; }

    //-------------------------------------------------------------------------------------------------
    void
    enqueue_basic(unsigned int status, unsigned int b1, unsigned int b2 = 0)
    //-------------------------------------------------------------------------------------------------
    {
        midi_t* mt = m_outbuffer.reserve(2);
        mt->frame = m_frame.load();
        mt->status = status;
        mt->data[0] = b1;
        mt->data[1] = b2;
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_note_on(unsigned int channel, unsigned int index, unsigned int velocity)
    //-------------------------------------------------------------------------------------------------
    {
        enqueue_basic(0x90+channel, index, velocity);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_note_off(unsigned int channel, unsigned int index, unsigned int velocity)
    //-------------------------------------------------------------------------------------------------
    {
        enqueue_basic(0x80+channel, index, velocity);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_control(unsigned int channel, unsigned int index, unsigned int value)
    //-------------------------------------------------------------------------------------------------
    {
        enqueue_basic(0xb0+channel, index, value);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_program(unsigned int channel, unsigned int index)
    //-------------------------------------------------------------------------------------------------
    {
        enqueue_basic(0xc0+channel, index);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_aftertouch(unsigned int channel, unsigned int index, unsigned int value)
    //-------------------------------------------------------------------------------------------------
    {
        enqueue_basic(0xa0+channel, index, value);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_pressure(unsigned int channel, unsigned int value)
    //-------------------------------------------------------------------------------------------------
    {
        enqueue_basic(0xd0+channel, value);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_bend(unsigned int channel, unsigned int value)
    // 14bits TODO
    //-------------------------------------------------------------------------------------------------
    {
        enqueue_basic(0xe0+channel, value & 0x7f, value >> 7);
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    write_sysex(QVariantList list)
    // when do we deallocate it?
    //-------------------------------------------------------------------------------------------------
    {
        QByteArray arr8;

        for (int n = 1; n < list.size(); ++n)
        {
            QVariant v = list[n];
            if (v.type() == QMetaType::QString)
                for (const auto& u8 : v.toString())
                     arr8.append(u8.toLatin1());
            else arr8.append(v.toInt());
        }

        midi_t* mt = m_outbuffer.reserve(arr8.count());
        mt->frame = m_frame;
        mt->status = list[0].value<byte_t>();

        for (int n = 0; n < arr8.count(); ++n)
             mt->data[n] = arr8[n];
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
        m_outbuffer.allocate(sizeof(sample_t)*properties.vector);
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
    invoke_out_signal_2(const char* signal, byte_t channel, unsigned int b1)
    //-------------------------------------------------------------------------------------------------
    {
        QMetaObject::invokeMethod(this, signal, Qt::QueuedConnection,
            Q_ARG(unsigned int, channel), Q_ARG(unsigned int, b1));
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    {
        auto midi_events  = inputs.midi[0][0];
        auto midi_out     = outputs.midi[0][0];

        for (auto& mt : *midi_events)
        {            
            byte_t channel = mt.status & 0x0f;

            switch(mt.status & 0xf0) {
            case 0x80: invoke_out_signal_3("noteOff", channel, mt.data[0], mt.data[1]); break;
            case 0x90: invoke_out_signal_3("noteOn", channel, mt.data[0], mt.data[1]); break;
            case 0xa0: invoke_out_signal_3("aftertouch", channel, mt.data[0], mt.data[1]); break;
            case 0xb0: invoke_out_signal_3("control", channel, mt.data[0], mt.data[1]); break;
            case 0xc0: invoke_out_signal_2("program", channel, mt.data[0]); break;
            case 0xd0: invoke_out_signal_2("pressure", channel, mt.data[0]); break;
            case 0xe0: invoke_out_signal_2("bend", channel, (mt.data[0] & 0x7f) | (mt.data[1] << 7)); break;
            }

            m_frame++;
        }

        // copy-write output data
        for (auto& mt : m_outbuffer)
             midi_out->push(mt);

        m_frame = 0;
        m_outbuffer.clear();
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override { m_rate = rate; }

private:

    size_t
    m_phase = 0;

    sample_t
    m_rate = 0;

    midibuffer
    m_outbuffer;

    std::atomic<vector_t>
    m_frame {0};

};
