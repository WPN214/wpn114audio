#include "external.hpp"
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
int
JackExternal::jack_process_callback(jack_nframes_t nframes, void* udata)
// run the graph for a certain amount of frames
// copy the graph's final output into the jack output buffer
//-------------------------------------------------------------------------------------------------
{
    JackExternal&
    jext = *static_cast<JackExternal*>(udata);

    External&
    ext = jext.parent();

    nchannels_t
    n = 0;

    Port
    *extout_m = ext.default_port(Port::Midi_1_0, Polarity::Output),
    *extout_a = ext.default_port(Port::Audio, Polarity::Output),
    *extin_m  = ext.default_port(Port::Midi_1_0, Polarity::Input),
    *extin_a  = ext.default_port(Port::Audio, Polarity::Input);
    // note: External's outputs become inputs in Graph's point of view
    // and obviously External's inputs are what we will be feeding to jack

    // AUDIO INPUTS ------------------------------------------------------------------------------
    if (extout_a->nchannels())
    {
        auto extout_a_buffer = extout_a->buffer<audiobuffer_t>();
        for (auto& input : jext.m_audio_inputs) {
            float* buf = static_cast<float*>(jack_port_get_buffer(input, nframes));
            // graph is processed in qreal, which usually is double
            // except is qt is compiled with the qreal=float option
            // but jack processes in float
            for (jack_nframes_t f = 0; f < nframes; ++f)
                 extout_a_buffer[n][f] = static_cast<sample_t>(buf[n]);
            n++;
        }
    }

    // MIDI INPUTS ------------------------------------------------------------------------------

    if (extout_m->nchannels())
    {
        auto& extout_m_buffer = *extout_m->buffer<midibuffer_t*>();
        jack_midi_event_t ev;
        n = 0;

        for (auto& input : jext.m_midi_inputs) {
            auto buf = jack_port_get_buffer(input, nframes);
            auto nev = jack_midi_get_event_count(buf);

            if (nev)
                qDebug() << "in midi events:" << QString::number(nev);

            for (jack_nframes_t f = 0; f < nframes; ++f) {
                for (jack_nframes_t e = 0; e < nev; ++e)
                {
                    jack_midi_event_get(&ev, buf, e);
                    if (ev.time == f) {
                        midi_t mt;
                        mt.frame = ev.time;
                        mt.status = ev.buffer[0];
                        mt.b1 = ev.buffer[1];
                        mt.b2 = ev.buffer[2];
                        extout_m_buffer.push_back(mt);
                    }}}
            n++;
        }
    }

    //---------------------------------------------------------------------------------------------
    // process the internal graph
    Graph::instance().run();

    // AUDIO OUTPUTS ------------------------------------------------------------------------------

    if (extin_a->nchannels()) {
        auto extin_a_buffer = extin_a->buffer<audiobuffer_t>();
        n = 0;

        for (auto& output : jext.m_audio_outputs) {
            float* buf = static_cast<float*>(jack_port_get_buffer(output, nframes));
            for (jack_nframes_t f = 0; f < nframes; ++f)
                buf[f] = static_cast<float>(extin_a_buffer[n][f]);
            n++;
        }
    }

    // MIDI OUTPUTS ------------------------------------------------------------------------------

    if (extin_m->nchannels()) {
        auto& extin_m_buffer = *extin_m->buffer<midibuffer_t*>();
        n = 0;

        for (auto& output : jext.m_midi_outputs) {
            auto buf = jack_port_get_buffer(output, nframes);                
            for (auto& mt : extin_m_buffer)
            {
                jack_midi_data_t* ev = jack_midi_event_reserve(buf,mt.frame, 3);
                ev[0] = mt.status;
                ev[1] = mt.b1;
                ev[2] = mt.b2;
            }
        }}

    return 0;
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::register_ports(
        nchannels_t nchannels,
        const char* port_mask,
        const char* type, unsigned long polarity,
        std::vector<jack_port_t*>& target)
{
    for (nchannels_t n = 0; n < nchannels; ++n)
    {
        char name[12];
        char index[3];
        strcpy(name, port_mask);
        sprintf(index, "_%d", n);
        strcat(name, index);

        qDebug() << "[JACK] registering jack port:" << name;

        auto port = jack_port_register(m_client, name, type, polarity, 0);
        target.push_back(port);
    }
}

//-------------------------------------------------------------------------------------------------
JackExternal::JackExternal(External* parent) : m_parent(*parent)
  //-------------------------------------------------------------------------------------------------
{
    auto name = CSTR(parent->name());
    jack_status_t status;

    qDebug() << "[JACK] opening jack client with name:"
             << parent->name();

    m_client = jack_client_open(name, JackNullOption, &status);
    // sample rate and buffer sizes are defined by the user from jack
    // there's no need to set them from the graph
    jack_nframes_t
    srate = jack_get_sample_rate(m_client),
    bsize = jack_get_buffer_size(m_client);

    qDebug() << "[JACK] samplerate:" << QString::number(srate)
             << "buffer size:" << QString::number(bsize);

    Graph::instance().set_rate    (srate);
    Graph::instance().set_vector  (bsize);

    // set callbacks
    jack_set_sample_rate_callback(m_client, on_jack_sample_rate_changed, this);
    jack_set_buffer_size_callback(m_client, on_jack_buffer_size_changed, this);
    jack_set_process_callback(m_client, jack_process_callback, this);

    // register input/output ports
    register_ports(parent->n_audio_inputs(), "audio_in",
                   JACK_DEFAULT_AUDIO_TYPE,
                   JackPortIsInput, m_audio_inputs);

    register_ports(parent->n_audio_outputs(), "audio_out",
                   JACK_DEFAULT_AUDIO_TYPE,
                   JackPortIsOutput, m_audio_outputs);

    register_ports(parent->n_midi_inputs(), "midi_in",
                   JACK_DEFAULT_MIDI_TYPE,
                   JackPortIsInput, m_midi_inputs);

    register_ports(parent->n_midi_outputs(), "midi_out",
                   JACK_DEFAULT_MIDI_TYPE,
                   JackPortIsOutput, m_midi_outputs);
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::connect_ports(
        std::vector<jack_port_t*>& ports,
        unsigned long polarity,
        const char* type,
        QStringList const& targets,
        Routing routing)
{
    if (targets.empty())
        return;

    for (auto& target : targets) {
        auto ctarget = jack_get_ports(m_client, CSTR(target), type, polarity);

        if (!ctarget) {
            qDebug() << "[JACK] error, could not connect to target" << target;
            continue;
        }

        if (routing.null()) {
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
// activates jack client, make the input/output port connections
//-------------------------------------------------------------------------------------------------
{
    qDebug() << "[JACK] activating client";
    jack_activate(m_client);

    // if there are input/output targets
    // make the connections
    qDebug() << "[JACK] connecting ports";
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

    qDebug() << "[JACK] running";
}
