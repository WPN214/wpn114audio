#include "external.hpp"

//-------------------------------------------------------------------------------------------------
int
JackExternal::on_jack_sample_rate_changed(jack_nframes_t rate, void* udata)
//-------------------------------------------------------------------------------------------------
{
    Graph::set_rate(rate);
    return 0;
}

//-------------------------------------------------------------------------------------------------
int
JackExternal::on_jack_buffer_size_changed(jack_nframes_t nframes, void* udata)
//-------------------------------------------------------------------------------------------------
{
    Graph::set_vector(nframes);
    return 0;
}

//-------------------------------------------------------------------------------------------------
int
JackExternal::jack_process_callback(jack_nframes_t nframes, void* udata)
//-------------------------------------------------------------------------------------------------
{
    JackExternal& ext = *static_cast<JackExternal*>(udata);
    pool& outputs = Graph::run(ext.parent());

    return 0;
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::rwrite(pool& inputs, pool& outputs, vector_t nframes)
//-------------------------------------------------------------------------------------------------
{

}

//-------------------------------------------------------------------------------------------------
void
JackExternal::register_ports
(
nchannels_t nchannels,
const char* port_mask,
const char* type,
int polarity,
std::vector<jack_port_t*>& target
)
//-------------------------------------------------------------------------------------------------
{
    for (nchannels_t n = 0; n < nchannels; ++n)
    {
        char name[12];
        char index[3];
        strcpy(name, port_mask);
        sprintf(index, "_%d", n);
        strcat(name, index);

        auto port = jack_port_register(m_client, name, type, polarity, 0);
        m_audio_inputs.push_back(port);
    }
}

#define CSTR(_qstring) _qstring.toStdString().c_str()

//-------------------------------------------------------------------------------------------------
JackExternal::JackExternal(External* parent) : m_parent(*parent)
  //-------------------------------------------------------------------------------------------------
{
    auto name = CSTR(parent->name());
    jack_status_t status;

    m_client = jack_client_open(name, JackNullOption, &status);

    // sample rate and buffer sizes are defined by the user from jack
    // there's no need to set them from the graph
    jack_nframes_t srate = jack_get_sample_rate(m_client);
    jack_nframes_t bsize = jack_get_buffer_size(m_client);

    Graph::set_rate     (srate);
    Graph::set_vector   (bsize);

    // set callbacks
    jack_set_sample_rate_callback(m_client, on_jack_sample_rate_changed, this);
    jack_set_buffer_size_callback(m_client, on_jack_buffer_size_changed, this);
    jack_set_process_callback(m_client, jack_process_callback, this);

    // register input/output ports
    nchannels_t
    n_audio_inputs   = parent->n_audio_inputs(),
    n_audio_outputs  = parent->n_audio_outputs(),
    n_midi_inputs    = parent->n_midi_inputs(),
    n_midi_outputs   = parent->n_midi_outputs();

    register_ports(n_audio_inputs, "audio_in",
                   JACK_DEFAULT_AUDIO_TYPE,
                   JackPortIsInput, m_audio_inputs);

    register_ports(n_audio_outputs, "audio_out",
                   JACK_DEFAULT_AUDIO_TYPE,
                   JackPortIsOutput, m_audio_outputs);

    register_ports(n_midi_inputs, "midi_in",
                   JACK_DEFAULT_MIDI_TYPE,
                   JackPortIsInput, m_midi_inputs);

    register_ports(n_midi_outputs, "midi_out",
                   JACK_DEFAULT_MIDI_TYPE,
                   JackPortIsOutput, m_midi_outputs);
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::connect_ports(std::vector<jack_port_t*>& ports,
                            int polarity, const char* type,
                            QStringList const& targets, Routing routing)
//-------------------------------------------------------------------------------------------------
{
    if (targets.empty())
        return;

    for (auto& target : targets)
    {
        auto ctarget = jack_get_ports(m_client, CSTR(target), type, polarity);

        if (routing.null())
        {
            for (nchannels_t n = 0; n < ports.size(); ++n) {
                auto pname = jack_port_name(ports[n]);
                if (polarity == JackPortIsOutput)
                     jack_connect(m_client, ctarget[n], pname);
                else jack_connect(m_client, pname, ctarget[n]);
            }
        }
        else for (nchannels_t n = 0; n < routing.ncables(); ++n)
        {
            uint8_t
            inno  = routing[n][0],
            outno = routing[n][1];

            if (polarity == JackPortIsInput) {
                auto pname = jack_port_name(ports[outno]);
                jack_connect(m_client, pname, ctarget[inno]);
            } else {
                auto pname = jack_port_name(ports[inno]);
                jack_connect(m_client, ctarget[outno], pname);
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::run()
//-------------------------------------------------------------------------------------------------
{
    jack_activate(m_client);
    // if there are input/output targets
    // make the connections
    connect_ports(m_audio_inputs, JackPortIsOutput,
                  JACK_DEFAULT_AUDIO_TYPE,
                  m_parent.in_audio_targets_list(),
                  m_parent.in_audio_routing());

    connect_ports(m_audio_outputs, JackPortIsInput,
                  JACK_DEFAULT_AUDIO_TYPE,
                  m_parent.out_audio_targets_list(),
                  m_parent.out_audio_routing());

    connect_ports(m_midi_inputs, JackPortIsOutput,
                  JACK_DEFAULT_MIDI_TYPE,
                  m_parent.in_midi_targets_list(),
                  m_parent.in_midi_routing());

    connect_ports(m_midi_outputs, JackPortIsInput,
                  JACK_DEFAULT_MIDI_TYPE,
                  m_parent.out_midi_targets_list(),
                  m_parent.out_midi_routing());
}
