#pragma once

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QIODevice>
#include <source/io/external.hpp>

//=================================================================================================
class QtAudioBackend : public ExternalBase, public QIODevice
// this would be the preferred i/o backend for Android/iOS platforms
//=================================================================================================
{
    Q_OBJECT
    Q_INTERFACES (QIODevice)

    External&
    m_parent;

    QAudioFormat
    m_format;

    QAudioInput*
    m_input = nullptr;

    QAudioOutput*
    m_output = nullptr;

public:

    //---------------------------------------------------------------------------------------------
    QtAudioBackend(External& parent) : m_parent(parent)
    //---------------------------------------------------------------------------------------------
    {
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleType(QAudioFormat::Float);
        m_format.setSampleSize(32);
        m_format.setSampleRate(Graph::instance().rate());
        m_format.setChannelCount(2);

        auto in_device_info   = QAudioDeviceInfo::defaultInputDevice();
        auto out_device_info  = QAudioDeviceInfo::defaultOutputDevice();

        assert(in_device_info.isFormatSupported(m_format));
        assert(out_device_info.isFormatSupported(m_format));

        m_input  = new QAudioInput(in_device_info, m_format, this);
        m_output = new QAudioOutput(out_device_info, m_format, this);
    }

    //---------------------------------------------------------------------------------------------
    virtual qint64
    readData(char *data, qint64 maxlen) override
    //---------------------------------------------------------------------------------------------
    {
        Graph::instance().run();

        auto out = reinterpret_cast<sample_t*>(data);
        auto vsz = Graph::instance().vector();

        for (auto output : m_parent.audio_outputs())
        {
            auto port = output->default_port(Port::Audio, Polarity::Input);
            auto bufr = port->buffer<audiobuffer_t>();

            for (nchannels_t c = 0; c < port->nchannels(); ++c)
                for (vector_t f = 0; f < vsz; ++f)
                     *out++ = bufr[c][f];
        }

        return vsz*sizeof(sample_t)*2;
    }

    //---------------------------------------------------------------------------------------------
    virtual qint64
    writeData(const char *data, qint64 len) override
    //---------------------------------------------------------------------------------------------
    {
        auto in = reinterpret_cast<const sample_t*>(data);
        auto vsz = Graph::instance().vector();

        for (auto input : m_parent.audio_inputs())
        {
            auto port = input->default_port(Port::Audio, Polarity::Output);
            auto bufr = port->buffer<audiobuffer_t>();

            for (nchannels_t c = 0; c < port->nchannels(); ++c)
                for (vector_t f = 0; f < vsz; ++f)
                    bufr[c][f] = *in++;
        }

        return vsz*sizeof(sample_t)*2;
    }

    //---------------------------------------------------------------------------------------------
    virtual qint64
    bytesAvailable() const override
    //---------------------------------------------------------------------------------------------
    {
        return 0;
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    run() override
    //---------------------------------------------------------------------------------------------
    {
        m_input->start(this);
        m_output->start(this);
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    stop() override
    //---------------------------------------------------------------------------------------------
    {
        m_input->stop();
        m_output->stop();
    }
};
