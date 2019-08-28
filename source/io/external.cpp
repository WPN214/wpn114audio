#include "external.hpp"

#ifdef __linux__
    #ifndef __ANDROID__
        #include "io_alsa.hpp"
    #endif
#endif

#ifdef WPN114AUDIO_JACK
    #include "io_jack.hpp"
#endif

#ifdef __ANDROID__
    #include "io_qt.hpp"
#endif

void
External::componentComplete()
{
#ifdef WPN114AUDIO_JACK
    if (m_backend_id == Backend::Jack) {
        m_backend = new JackExternal(this);
    }
#endif
#ifdef __ANDROID__
    if (m_backend_id == Backend::Qt ||
        m_backend_id == Backend::Default)
        m_backend = new QtAudioBackend(*this);
#endif
#ifdef __APPLE__
    if (m_backend_id == Backend::CoreAudio ||
        m_backend_id == Backend::Default)
        m_backend = new CoreAudioBackend(*this);
#endif
    if (m_running)
        m_backend->run();
}


template<typename T> void
IOBase::link_proxy(IOProxy* proxy)
{
    auto proxy_channels = proxy->channel_vector();
    auto proxy_buffer = new T[proxy_channels.count()];
    auto io_buffer = default_port(m_polarity)->buffer<T*>();

    nchannels_t n = 0;

    for (const auto& channel : proxy_channels) {
        proxy_buffer[n] = io_buffer[channel];
        n++;
    }

    auto proxy_port = proxy->default_port((Port::Type) proxy->type(), m_polarity);
    proxy_port->set_buffer<T*>(proxy_buffer);
}


void
IOBase::on_graph_complete(Graph::properties const& properties)
{

    for (auto& proxy : m_proxies) {
        // gather all external connections
         for (auto& connection : proxy->connections())
             m_connections.push_back(connection);
         // set max number of channels to allocate later
         for (const auto& channel : proxy->channel_vector())
             m_nchannels = std::max(m_nchannels, channel);
    }

    if (m_nchannels > 0)
        m_nchannels++;

    default_port(m_polarity)->set_nchannels(m_nchannels);
    qDebug() << "[IO]" << m_name <<"-"<< m_nchannels << "channels";
    Node::on_graph_complete(properties);
}

void
IOBase::initialize(Graph::properties const& properties)
{
    // link proxies buffers to this one
    for (auto& proxy : m_proxies) {
        auto type = proxy->type();
        switch(type) {
        case IOProxy::Audio: link_proxy<sample_t*>(proxy); break;
        case IOProxy::Midi: link_proxy<midibuffer*>(proxy);
        }
    }
}
