#include "external.hpp"
#include <jack/midiport.h>

//-------------------------------------------------------------------------------------------------
void
External::register_input(Input* input)
//-------------------------------------------------------------------------------------------------
{
    switch(input->type()) {
    case Input::Type::Audio: m_audio_inputs.push_back(input); break;
    case Input::Type::Midi: m_midi_inputs.push_back(input);
    }
}

//-------------------------------------------------------------------------------------------------
void
External::register_output(Output* output)
//-------------------------------------------------------------------------------------------------
{
    switch(output->type()) {
    case Output::Type::Audio: m_audio_outputs.push_back(output); break;
    case Output::Type::Midi: m_midi_outputs.push_back(output);
    }
}

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

    nchannels_t n = 0;

    auto audio_inputs   = ext.audio_inputs();
    auto midi_inputs    = ext.midi_inputs();

    // AUDIO_INPUTS ------------------------------------------------------------------------------
    for (auto& input : audio_inputs)
    {
        // directly fetch the output buffer
        // no need to process the node
        auto port  = input->default_port(Port::Audio, Polarity::Output);
        auto bufr  = port->buffer<audiobuffer_t>();

        auto channels = input->channels_vec();

        for (auto& channel : input->channels_vec())
        {
            auto j_in = j_ext.m_audio_inputs[channel];
            auto j_buf = static_cast<sample_t*>(jack_port_get_buffer(j_in, nframes));

            for (jack_nframes_t f = 0; f < nframes; ++f)
                bufr[0][f] = j_buf[f];
            n++;
        }
    }

    // MIDI_INPUTS ------------------------------------------------------------------------------

    for (auto& input : midi_inputs)
    {
        auto port = input->default_port(Port::Midi_1_0, Polarity::Output);
        auto bufr = port->buffer<midibuffer_t>();
        auto channels = input->channels_vec();
        jack_midi_event_t ev;
        n = 0;

        for (auto& channel : channels)
        {
            auto j_in   = j_ext.m_midi_inputs[channel];
            auto j_buf  = jack_port_get_buffer(j_in, nframes);
            auto n_evt  = jack_midi_get_event_count(j_buf);

            for (jack_nframes_t f = 0; f < nframes; ++f) {
                for (jack_nframes_t e = 0; e < n_evt; ++e)
                {
                    jack_midi_event_get(&ev, j_buf, e);
                    if (ev.time == f) {
                        midi_t* mt = bufr[n]->reserve(ev.size-1);
                        mt->frame = ev.time;
                        mt->status = ev.buffer[0];
                        memcpy(mt->data, &(ev.buffer[1]), ev.size-1);
                    }}}
            n++;
        }
    }

    //---------------------------------------------------------------------------------------------
    // process the internal graph
    Graph::instance().run();

    // fill outputs
    auto audio_outputs  = ext.audio_outputs();
    auto midi_outputs   = ext.midi_outputs();

    // AUDIO OUTPUTS ------------------------------------------------------------------------------

    for (auto& audio_out : audio_outputs)
    {
        auto port = audio_out->default_port(Port::Audio, Polarity::Input);
        auto bufr = port->buffer<audiobuffer_t>();
        auto channels = audio_out->channels_vec();
        n = 0;

        for (auto& channel : channels)
        {
            auto j_out = j_ext.m_audio_outputs[channel];
            auto j_buf = static_cast<float*>(jack_port_get_buffer(j_out, nframes));

            for (jack_nframes_t f = 0; f < nframes; ++f)
                j_buf[f] = bufr[n][f];
            n++;
        }
    }

    // MIDI OUTPUTS ------------------------------------------------------------------------------

    for (auto& midi_out : midi_outputs)
    {
        auto port = midi_out->default_port(Port::Midi_1_0, Polarity::Input);
        auto bufr = port->buffer<midibuffer_t>();
        auto channels = midi_out->channels_vec();
        n = 0;

        for (auto channel : channels)
        {
            auto j_out = j_ext.m_midi_outputs[channel];
            auto j_buf = jack_port_get_buffer(j_out, nframes);
            jack_midi_clear_buffer(j_buf);

            for (auto& mt : *bufr[n])
            {
                auto ev = jack_midi_event_reserve(j_buf, mt.frame, mt.nbytes+1);
                memcpy(&(ev[1]), mt.data, mt.nbytes);
                ev[0] = mt.status;
            }

            n++;
        }
    }

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
    for (nchannels_t n = 0; n < nchannels; ++n) {
        qDebug() << "[JACK] registering jack port:" << port_mask;
        auto port = jack_port_register(m_client, port_mask, type, polarity, 0);
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
    jack_set_client_registration_callback(m_client, on_jack_client_registration, this);

    auto audio_inputs   = parent->audio_inputs();
    auto midi_inputs    = parent->midi_inputs();
    auto audio_outputs  = parent->audio_outputs();
    auto midi_outputs   = parent->midi_outputs();

    // register input/output ports
    for (auto& input : audio_inputs)
        register_ports(input->nchannels(), CSTR(input->name()),
                       JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,
                       m_audio_inputs);

    for (auto& input : midi_inputs)
        register_ports(input->nchannels(), CSTR(input->name()),
                       JACK_DEFAULT_MIDI_TYPE, JackPortIsInput,
                       m_midi_inputs);

    for (auto& output : audio_outputs)
        register_ports(output->nchannels(), CSTR(output->name()),
                       JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput,
                       m_audio_outputs);

    for (auto& output : midi_outputs)
        register_ports(output->nchannels(), CSTR(output->name()),
                       JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput,
                       m_midi_outputs);
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::connect_ports(
        std::vector<jack_port_t*>& ports,
        unsigned long polarity,
        const char* type,
        QString target,
        Routing routing)
{
    if (target.isEmpty())
        return;

    auto ctarget = jack_get_ports(m_client, CSTR(target), type, polarity);

    if (!ctarget) {
        qDebug() << "[JACK] error, could not connect to target" << target;
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

    for (auto& input : m_parent.audio_inputs())
        for (auto& connection : input->connections())
            connect_ports(m_audio_inputs, JackPortIsOutput,
                          JACK_DEFAULT_AUDIO_TYPE,
                          connection.target, connection.routing);

    for (auto& input : m_parent.midi_inputs())
         for (auto& connection : input->connections())
             connect_ports(m_midi_inputs, JackPortIsOutput,
                           JACK_DEFAULT_MIDI_TYPE,
                           connection.target, connection.routing);

    for (auto& output : m_parent.audio_outputs())
        for (auto& connection : output->connections())
            connect_ports(m_audio_outputs, JackPortIsInput,
                          JACK_DEFAULT_AUDIO_TYPE,
                          connection.target, connection.routing);

    for (auto& output: m_parent.midi_outputs())
        for (auto& connection : output->connections())
            connect_ports(m_midi_outputs, JackPortIsInput,
                          JACK_DEFAULT_MIDI_TYPE,
                          connection.target, connection.routing);

    m_parent.connected();
    qDebug() << "[JACK] running";
}
