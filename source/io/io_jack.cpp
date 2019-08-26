#include "io_jack.hpp"
#include <jack/midiport.h>

//-------------------------------------------------------------------------------------------------
int
JackExternal::on_jack_sample_rate_changed(jack_nframes_t rate, void* udata)
// static callback, from jack whenever sample rate has changed
// we update the graph, which will in turn notify registered Nodes of this change
//-------------------------------------------------------------------------------------------------
{
    Graph::instance().set_rate(rate);
    return 0;
}

//-------------------------------------------------------------------------------------------------
int
JackExternal::on_jack_buffer_size_changed(jack_nframes_t nframes, void* udata)
// static callback, from jack whenever io buffer size has changed
// we update the graph, which will in turn notify registered Nodes of this change
//-------------------------------------------------------------------------------------------------
{
    Graph::instance().set_vector(nframes);
    return 0;
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::on_jack_client_registration(const char* name, int reg, void* udata)
// static callback, will be used to monitor client targets disconnections/reconnections
//-------------------------------------------------------------------------------------------------
{
    qDebug() << QString(name);
}

//-------------------------------------------------------------------------------------------------
int
JackExternal::jack_process_callback(jack_nframes_t nframes, void* udata)
// run the graph for a certain amount of frames
// copy the graph's final output into the jack output buffer
//-------------------------------------------------------------------------------------------------
{
    auto& j_ext = *static_cast<JackExternal*>(udata);
    auto& ext = j_ext.parent();

    //---------------------------------------------------------------------------------------------
    // AUDIO_INPUTS
    //---------------------------------------------------------------------------------------------
    {
        auto buffer = ext.audio_inputs().buffer<audiobuffer_t>();
        auto ports  = j_ext.m_audio_input_ports;

        for (nchannels_t c = 0; c < ports.size(); ++c)
        {
            auto j_in = ports[c];
            auto j_buf = static_cast<sample_t*>(jack_port_get_buffer(j_in, nframes));

            for (jack_nframes_t f = 0; f < nframes; ++f)
                 buffer[c][f] = j_buf[f];
        }
    }

    //---------------------------------------------------------------------------------------------
    // MIDI_INPUTS
    //---------------------------------------------------------------------------------------------
    {
        auto buffer = ext.midi_inputs().buffer<midibuffer_t>();
        auto ports = j_ext.m_midi_input_ports;
        jack_midi_event_t mevent;

        for (nchannels_t c = 0; c < ports.size(); ++c)
        {
            auto j_in = ports[c];
            auto j_bf = jack_port_get_buffer(j_in, nframes);
            auto n_ev = jack_midi_get_event_count(j_bf);

            for (jack_nframes_t f = 0; f < nframes; ++f) {
                for (jack_nframes_t e = 0; e < n_ev; ++e)
                {
                    jack_midi_event_get(&mevent, j_bf, e);
                    if (mevent.time == f) {
                        midi_t* mt = buffer[c]->reserve(mevent.size-1);
                        mt->frame  = mevent.time;
                        mt->status = mevent.buffer[0];
                        memcpy(mt->data, &(mevent.buffer[1]), mevent.size-1);
                    }
                }
            }
        }
    }

    //---------------------------------------------------------------------------------------------
    // process the internal graph
    Graph::instance().run();

    //---------------------------------------------------------------------------------------------
    // AUDIO_OUTPUTS
    //---------------------------------------------------------------------------------------------
    {
        auto buffer = ext.audio_outputs().buffer<audiobuffer_t>();
        auto ports = j_ext.m_audio_output_ports;

        for (nchannels_t c = 0; c < ports.size(); ++c)
        {
            auto j_out = ports[c];
            auto j_buf = static_cast<sample_t*>(jack_port_get_buffer(j_out, nframes));

            for (jack_nframes_t f = 0; f < nframes; ++f)
                 j_buf[f] = buffer[c][f];
        }
    }

    //---------------------------------------------------------------------------------------------
    // MIDI_OUTPUTS
    //---------------------------------------------------------------------------------------------
    {
        auto buffer = ext.midi_outputs().buffer<midibuffer_t>();
        auto ports = j_ext.m_midi_output_ports;

        for (nchannels_t c = 0; c < ports.size(); ++c)
        {
            auto j_out = ports[c];
            auto j_buf = jack_port_get_buffer(j_out, nframes);
            jack_midi_clear_buffer(j_buf);

            for (auto& mt : *buffer[c]) {
                auto ev = jack_midi_event_reserve(j_buf, mt.frame, mt.nbytes+1);
                memcpy(&(ev[1]), mt.data, mt.nbytes);
                ev[0] = mt.status;
            }
        }
    }

    return 0;
}

//---------------------------------------------------------------------------------------------------
void
JackExternal::register_ports(
//---------------------------------------------------------------------------------------------------
        std::vector<jack_port_t*>& dest,
        nchannels_t nports,
        const char* mask,
        const char* type,
        JackPortFlags flags)
//---------------------------------------------------------------------------------------------------
{
    for (nchannels_t n = 0; n < nports; ++n)
    {
        char name[16], num[3];
        sprintf(num, "%d", n);
        strcpy(name, mask);
        strcat(name, num);
        dest.push_back(jack_port_register(m_client, name, type, flags, 0));
    }
}

//---------------------------------------------------------------------------------------------------
void
JackExternal::connect(
//---------------------------------------------------------------------------------------------------
        QVector<ExternalConnection>& connections,
        std::vector<jack_port_t*>& ports,
        jack_port_flags_t polarity,
        const char* type)
//---------------------------------------------------------------------------------------------------
{

    for (auto& connection : connections)
    {
        auto routing = connection.routing;
        auto target = connection.target;
        auto target_ports = jack_get_ports(m_client, CSTR(target), type,
                            polarity == JackPortIsInput ? JackPortIsOutput : JackPortIsInput);

        if (routing.null())
        {
            for (nchannels_t n = 0; n < ports.size(); ++n)
                if (polarity == JackPortIsInput)
                     jack_connect(m_client, target_ports[n], jack_port_name(ports[n]));
                else jack_connect(m_client, jack_port_name(ports[n]), target_ports[n]);
        }

        else for (nchannels_t n = 0; n < routing.ncables(); ++n)
        {
            nchannels_t
            inno  = routing[n][0],
            outno = routing[n][1];

            if (polarity == JackPortIsInput) {
                auto pname = jack_port_name(ports[inno]);
                jack_connect(m_client, target_ports[outno], pname);
            } else {
                auto pname = jack_port_name(ports[inno]);
                jack_connect(m_client, pname, target_ports[outno]);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------
JackExternal::JackExternal(External* parent) : m_parent(*parent)
  //-------------------------------------------------------------------------------------------------
{
    jack_status_t status;

    qDebug() << "[JACK] opening jack client with name:"
             << parent->name();

    m_client = jack_client_open(CSTR(parent->name()), JackNullOption, &status);
    // sample rate and buffer sizes are defined globally from jack
    // there's no need to set them from the graph
    assert(m_client);

    jack_nframes_t
    srate = jack_get_sample_rate(m_client),
    bsize = jack_get_buffer_size(m_client);

    fprintf(stdout, "[JACK] samplerate: %d, buffer size: %d", srate, bsize);

    Graph::instance().set_rate(srate);
    Graph::instance().set_vector(bsize);

    // set callbacks
    jack_set_sample_rate_callback(m_client,
        on_jack_sample_rate_changed, this);

    jack_set_buffer_size_callback(m_client,
        on_jack_buffer_size_changed, this);

    jack_set_process_callback(m_client,
        jack_process_callback, this);

    jack_set_client_registration_callback(m_client,
        on_jack_client_registration, this);

    auto n_audio_inputs     = m_parent.audio_inputs().nchannels();
    auto n_midi_inputs      = m_parent.midi_inputs().nchannels();
    auto n_audio_outputs    = m_parent.audio_outputs().nchannels();
    auto n_midi_outputs     = m_parent.midi_outputs().nchannels();

    // register ports
    register_ports(m_audio_input_ports,
                   n_audio_inputs, "audio_in_",
                   JACK_DEFAULT_AUDIO_TYPE,
                   JackPortIsInput);

    register_ports(m_midi_input_ports,
                   n_midi_inputs, "midi_in_",
                   JACK_DEFAULT_MIDI_TYPE,
                   JackPortIsInput);

    register_ports(m_audio_output_ports,
                   n_audio_outputs, "audio_out_",
                   JACK_DEFAULT_AUDIO_TYPE,
                   JackPortIsOutput);

    register_ports(m_midi_output_ports,
                   n_midi_outputs, "midi_out_",
                   JACK_DEFAULT_MIDI_TYPE,
                   JackPortIsOutput);
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::run()
// activates jack client, make the input/output port connections
//-------------------------------------------------------------------------------------------------
{
    qDebug() << "[JACK] activating client";
    jack_activate(m_client);

    // if there are input/output targets
    // make the appropriate connections
    qDebug() << "[JACK] connecting ports";

    connect(m_parent.audio_inputs().connections(),
            m_audio_input_ports, JackPortIsInput,
            JACK_DEFAULT_AUDIO_TYPE);

    connect(m_parent.midi_inputs().connections(),
            m_midi_input_ports, JackPortIsInput,
            JACK_DEFAULT_MIDI_TYPE);

    connect(m_parent.audio_outputs().connections(),
            m_audio_output_ports, JackPortIsOutput,
            JACK_DEFAULT_AUDIO_TYPE);

    connect(m_parent.midi_outputs().connections(),
            m_midi_output_ports, JackPortIsOutput,
            JACK_DEFAULT_MIDI_TYPE);

    m_parent.connected();
    qDebug() << "[JACK] running";
}
