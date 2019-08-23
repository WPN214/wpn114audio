#pragma once

#include <jack/jack.h>
#include <source/io/external.hpp>

//-------------------------------------------------------------------------------------------------
struct JackExternalIO
//-------------------------------------------------------------------------------------------------
{
    ExternalIO* io;
    std::vector<jack_port_t*> ports;
};

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

    //---------------------------------------------------------------------------------------------
    std::vector<JackExternalIO>
    m_audio_input_ios, m_audio_output_ios,
    m_midi_input_ios, m_midi_output_ios;

    std::vector<jack_port_t*>
    m_audio_input_ports, m_audio_output_ports,
    m_midi_input_ports, m_midi_output_ports;

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

    //---------------------------------------------------------------------------------------------
    void
    register_io(ExternalIO* io);

    //---------------------------------------------------------------------------------------------
    jack_port_t*
    find_port(JackPortFlags polarity, int type, int index);

    //---------------------------------------------------------------------------------------------
    void
    connect(ExternalIO*, QString target, Routing r);

    void
    connect(std::vector<JackExternalIO>& target);

    //---------------------------------------------------------------------------------------------
    std::vector<jack_port_t*>&
    io_ports(ExternalIO* io);

    //---------------------------------------------------------------------------------------------
    const char*
    io_type(ExternalIO* io);

    //---------------------------------------------------------------------------------------------
    JackPortFlags
    io_polarity(ExternalIO* io);

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
