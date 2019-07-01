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

    // AUDIO INPUTS -------------------------------------------------------------------------------

    for (auto& input : j_ext.m_audio_input_ios)
    {
        auto wport = input.io->default_port(Port::Audio, Polarity::Output);
        auto wbufr = wport->buffer<audiobuffer_t>();

        for (nchannels_t n = 0; n < input.ports.size(); ++n)
        {
            auto j_in = input.ports[n];
            auto j_buf = static_cast<sample_t*>(jack_port_get_buffer(j_in, nframes));
            for (jack_nframes_t f = 0; f < nframes; ++f)
                 wbufr[n][f] += j_buf[f];
        }
    }

    // MIDI INPUTS -------------------------------------------------------------------------------

    for (auto& input : j_ext.m_midi_input_ios)
    {
        auto wport = input.io->default_port(Port::Midi_1_0, Polarity::Output);
        auto wbufr = wport->buffer<midibuffer_t>();
        jack_midi_event_t mevent;

        for (nchannels_t n = 0; n < input.ports.size(); ++n)
        {
            auto j_in = input.ports[n];
            auto j_buf = jack_port_get_buffer(j_in, nframes);
            auto n_evt = jack_midi_get_event_count(j_buf);

            for (jack_nframes_t f = 0; f < nframes; ++f) {
                for (jack_nframes_t e = 0; e < n_evt; ++e)
                {
                    jack_midi_event_get(&mevent, j_buf, e);
                    if (mevent.time == f) {
                        midi_t* mt = wbufr[n]->reserve(mevent.size-1);
                        mt->frame  = mevent.time;
                        mt->status = mevent.buffer[0];
                        memcpy(mt->data, &(mevent.buffer[1]), mevent.size-1);
                    }}}
        }
    }

    //---------------------------------------------------------------------------------------------
    // process the internal graph
    Graph::instance().run();

    // AUDIO OUTPUTS ------------------------------------------------------------------------------

    for (auto& output : j_ext.m_audio_output_ios)
    {
        auto wport = output.io->default_port(Port::Audio, Polarity::Input);
        auto wbufr = wport->buffer<audiobuffer_t>();

        for (nchannels_t n = 0; n < output.ports.size(); ++n)
        {
            auto j_out = output.ports[n];
            auto j_buf = static_cast<sample_t*>(jack_port_get_buffer(j_out, nframes));
            for (jack_nframes_t f = 0; f < nframes; ++f)
                 j_buf[f] += wbufr[n][f];
        }
    }

    // MIDI OUTPUTS -------------------------------------------------------------------------------

    for (auto& output : j_ext.m_midi_output_ios)
    {
        auto wport = output.io->default_port(Port::Midi_1_0, Polarity::Input);
        auto wbufr = wport->buffer<midibuffer_t>();

        for (nchannels_t n = 0; n < output.ports.size(); ++n)
        {
            auto j_out = output.ports[n];
            auto j_buf = jack_port_get_buffer(j_out, nframes);
            jack_midi_clear_buffer(j_buf);

            for (auto& mt : *wbufr[n])
            {
                auto ev = jack_midi_event_reserve(j_buf, mt.frame, mt.nbytes+1);
                memcpy(&(ev[1]), mt.data, mt.nbytes);
                ev[0] = mt.status;
            }
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
jack_port_t*
JackExternal::find_port(JackPortFlags polarity, int type, int index)
// ------------------------------------------------------------------------------------------------
{
    std::vector<jack_port_t*>* target = nullptr;

    if (polarity == JackPortIsInput) {
        if (type == ExternalIO::Audio)
             target = &m_audio_input_ports;
        else target = &m_midi_input_ports;
    }   else  {
        if (type == ExternalIO::Audio)
             target = &m_audio_output_ports;
        else target = &m_midi_output_ports;
    }

    if (index < target->size())
        return target->at(index);

    return nullptr;
}

// ------------------------------------------------------------------------------------------------
const char*
JackExternal::io_type(ExternalIO* io)
// ------------------------------------------------------------------------------------------------
{
    return io->type() == ExternalIO::Audio ? JACK_DEFAULT_AUDIO_TYPE : JACK_DEFAULT_MIDI_TYPE;
}

// ------------------------------------------------------------------------------------------------
JackPortFlags
JackExternal::io_polarity(ExternalIO* io)
// ------------------------------------------------------------------------------------------------
{
    return qobject_cast<Input*>(io) ? JackPortIsInput : JackPortIsOutput;
}

// ------------------------------------------------------------------------------------------------
std::vector<jack_port_t*>&
JackExternal::io_ports(ExternalIO* io)
// ------------------------------------------------------------------------------------------------
{
    std::vector<JackExternalIO>* target = nullptr;

    if (io_polarity(io) == JackPortIsInput) {
        if (io->type() == ExternalIO::Audio)
             target = &m_audio_input_ios;
        else target = &m_midi_input_ios;
    }   else  {
        if (io->type() == ExternalIO::Audio)
             target = &m_audio_output_ios;
        else target = &m_midi_output_ios;
    }

    for (auto& jio : *target)
        if (jio.io == io)
            return jio.ports;

    assert(false);
}

// ------------------------------------------------------------------------------------------------
void
JackExternal::register_io(ExternalIO* io)
// ------------------------------------------------------------------------------------------------
{
    auto polarity = qobject_cast<Input*>(io) ?
                    JackPortIsInput : JackPortIsOutput;

    auto type = io->type() == ExternalIO::Audio ?
                JACK_DEFAULT_AUDIO_TYPE :
                JACK_DEFAULT_MIDI_TYPE ;

    auto name = CSTR(io->name());

    JackExternalIO j_ext;
    j_ext.io = io;

    for (auto& index : io->channels_vec()) {

        jack_port_t* port = find_port(polarity, io->type(), index);

        if (port == nullptr)
            port = jack_port_register(m_client, name, type, polarity, 0);

        j_ext.ports.push_back(port);
    }

    if (polarity == JackPortIsInput) {
        if (io->type() == ExternalIO::Audio)
             m_audio_input_ios.push_back(j_ext);
        else m_midi_input_ios.push_back(j_ext);
    }   else  {
        if (io->type() == ExternalIO::Audio)
             m_audio_output_ios.push_back(j_ext);
        else m_midi_output_ios.push_back(j_ext);
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

    for (auto& input : parent->audio_inputs())
        register_io(input);

    for (auto& input : parent->midi_inputs())
        register_io(input);

    for (auto& output : parent->audio_outputs())
        register_io(output);

    for (auto& output : parent->midi_outputs())
        register_io(output);
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::connect(ExternalIO* io, QString target, Routing routing)
//-------------------------------------------------------------------------------------------------
{
    auto& source_ports      = io_ports(io);
    auto type               = io_type(io);
    auto polarity           = io_polarity(io);
    auto target_ports       = jack_get_ports(m_client, CSTR(target), type,
                              polarity == JackPortIsInput ? JackPortIsOutput : JackPortIsInput);

    if (target_ports == nullptr)
        return;

    if (routing.null())
    {
        for (nchannels_t n = 0; n < source_ports.size(); ++n)
            if (polarity == JackPortIsInput)
                 jack_connect(m_client, target_ports[n], jack_port_name(source_ports[n]));
            else jack_connect(m_client, jack_port_name(source_ports[n]), target_ports[n]);
    }
    else for (nchannels_t n = 0; n < routing.ncables(); ++n)
    {
        uint8_t
        inno  = routing[n][0],
        outno = routing[n][1];

        if (polarity == JackPortIsInput) {
            auto pname = jack_port_name(source_ports[inno]);
            jack_connect(m_client, target_ports[outno], pname);
        } else {
            auto pname = jack_port_name(source_ports[inno]);
            jack_connect(m_client, pname, target_ports[outno]);
        }
    }
}

//-------------------------------------------------------------------------------------------------
void
JackExternal::connect(std::vector<JackExternalIO>& target)
//-------------------------------------------------------------------------------------------------
{
    for (auto& j_ext : target)
        for (auto& connection : j_ext.io->connections())
            connect(j_ext.io, connection.target, connection.routing);
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

    connect(m_audio_input_ios);
    connect(m_midi_input_ios);
    connect(m_audio_output_ios);
    connect(m_midi_output_ios);

    m_parent.connected();
    qDebug() << "[JACK] running";
}
