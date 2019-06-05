#include "external.hpp"

JackExternal::JackExternal(External* parent) : m_parent(*parent)
{
    auto name = parent->name().toStdString().c_str();
    jack_status_t status;

    m_client = jack_client_open(name, JackNullOption, &status);

    jack_nframes_t srate = jack_get_sample_rate(m_client);
    Graph::set_rate(srate);

    jack_set_sample_rate_callback(m_client, ...);
    jack_set_buffer_size_callback(m_client, ...);



}
