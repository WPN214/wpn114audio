#pragma once

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QIODevice>
#include <source/io/external.hpp>

//=================================================================================================
class QtAudioBackend : public QIODevice, public ExternalBase
// this would be the preferred i/o backend for Android/iOS platforms
//=================================================================================================
{
    Q_OBJECT

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
        m_format.setCodec           ("audio/pcm");
        m_format.setByteOrder       (QAudioFormat::LittleEndian);
        m_format.setSampleType      (QAudioFormat::Float);
        m_format.setSampleSize      (32);
        m_format.setSampleRate      (Graph::instance().rate());
        m_format.setChannelCount    (m_parent.audio_outputs().nchannels());

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

        auto buffer = m_parent.audio_outputs().buffer<audiobuffer_t>();
        auto nchnls = m_parent.audio_outputs().nchannels();

        for (nchannels_t c = 0; c < nchnls; ++c)
            for (vector_t f = 0; f < vsz; ++f)
                *out++ = buffer[c][f];

        return vsz*sizeof(sample_t)*nchnls;
    }

    //---------------------------------------------------------------------------------------------
    virtual qint64
    writeData(const char *data, qint64 len) override
    //---------------------------------------------------------------------------------------------
    {
        auto in = reinterpret_cast<const sample_t*>(data);
        auto vsz = Graph::instance().vector();

        auto in_buffer = m_parent.audio_inputs().buffer<audiobuffer_t>();
        auto nchannels = m_parent.audio_inputs().nchannels();

        for (nchannels_t c = 0; c < nchannels; ++c)
            for (vector_t f = 0; f < vsz; ++f)
                in_buffer[c][f] = *in++;

        return vsz*sizeof(sample_t)*nchannels;
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
