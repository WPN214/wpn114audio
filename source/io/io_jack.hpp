#pragma once

#include <jack/jack.h>
#include <source/io/external.hpp>

using jack_port_flags_t = JackPortFlags;

//-------------------------------------------------------------------------------------------------
class JackExternal : public ExternalBase
//-------------------------------------------------------------------------------------------------
{
    //---------------------------------------------------------------------------------------------
    External&
    m_parent;

    //---------------------------------------------------------------------------------------------
    jack_client_t*
    m_client = nullptr;

    std::vector<jack_port_t*>
    m_audio_input_ports,
    m_audio_output_ports,
    m_midi_input_ports,
    m_midi_output_ports;

    //---------------------------------------------------------------------------------------------
    static int
    on_jack_sample_rate_changed(jack_nframes_t nframes, void* udata);

    //---------------------------------------------------------------------------------------------
    static int
    on_jack_buffer_size_changed(jack_nframes_t nframes, void* udata);

    //---------------------------------------------------------------------------------------------
    static int
    jack_process_callback(jack_nframes_t nframes, void* udata);

    //---------------------------------------------------------------------------------------------
    static void
    on_jack_client_registration(const char* name, int reg, void* udata);

public:

    //---------------------------------------------------------------------------------------------
    JackExternal(External* parent);

    //---------------------------------------------------------------------------------------------
    virtual
    ~JackExternal() override
    //---------------------------------------------------------------------------------------------
    {
        jack_deactivate(m_client);
        jack_client_close(m_client);
    }

    //---------------------------------------------------------------------------------------------
    External&
    parent() { return m_parent; }

    //---------------------------------------------------------------------------------------------------
    void
    register_ports(std::vector<jack_port_t*>& dest,
                   nchannels_t nports,
                   const char* mask,
                   const char* type,
                   jack_port_flags_t flags);

    //---------------------------------------------------------------------------------------------------
    void
    connect(QVector<ExternalConnection>& connections,
            std::vector<jack_port_t*>& ports,
            jack_port_flags_t polarity,
            const char* type);

    //---------------------------------------------------------------------------------------------
    virtual void
    run() override;

    //---------------------------------------------------------------------------------------------
    virtual void
    stop() override { jack_deactivate(m_client); }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_name_changed(QString& name) override { Q_UNUSED(name) }
    // TODO: override the other ones...
};
