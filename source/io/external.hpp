#pragma once
#include <wpn114audio/graph.hpp>

#include <QVector>

//---------------------------------------------------------------------------------------------
class ExternalBase
//---------------------------------------------------------------------------------------------
{

public:

    ExternalBase() {}
    //---------------------------------------------------------------------------------------------
    virtual
    ~ExternalBase() {}

    //---------------------------------------------------------------------------------------------
    virtual void
    run() = 0;

    //---------------------------------------------------------------------------------------------
    virtual void
    stop() = 0;

    //---------------------------------------------------------------------------------------------
    virtual void
    on_audio_inputs_changed(uint8_t n_inputs) { Q_UNUSED(n_inputs) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_audio_outputs_changed(uint8_t n_outputs) { Q_UNUSED(n_outputs) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_midi_inputs_changed(uint8_t n_inputs) { Q_UNUSED(n_inputs) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_midi_outputs_changed(uint8_t n_outputs) { Q_UNUSED(n_outputs) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_name_changed(QString& name) { Q_UNUSED(name) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_input_midi_targets_changed(QStringList& targets, Routing routing)
    { Q_UNUSED(targets) Q_UNUSED(routing) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_output_midi_targets_changed(QStringList& targets, Routing routing)
    { Q_UNUSED(targets) Q_UNUSED(routing) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_input_audio_targets_changed(QStringList& targets, Routing routing)
    { Q_UNUSED(targets) Q_UNUSED(routing) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_output_audio_targets_changed(QStringList& targets, Routing routing)
    { Q_UNUSED(targets) Q_UNUSED(routing) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_sample_rate_changed(sample_t rate) { Q_UNUSED(rate) }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_io_vector_changed(uint16_t nframes) { Q_UNUSED(nframes) }
};

//-------------------------------------------------------------------------------------------------
class IOProxy;
class InputProxy;
class OutputProxy;

//---------------------------------------------------------------------------------------------
struct ExternalConnection
// jack-only
//---------------------------------------------------------------------------------------------
{
    QString target;
    Routing routing;

    ExternalConnection() {}

    operator
    QVariant() const
    {
        QVariantList output;
        output << target;
        output << routing;

        return output;
    }
};

//=================================================================================================
class IOBase : public Node
//=================================================================================================
{
    Q_OBJECT

protected:

    Polarity
    m_polarity;

    nchannels_t
    m_nchannels;

    QVector<ExternalConnection>
    m_connections;

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {
        auto port = default_port(m_polarity);
        port->set_nchannels(m_nchannels);
        Node::componentComplete();
    }

public:

    //---------------------------------------------------------------------------------------------
    template<typename T> T*
    add_proxy(QVector<nchannels_t> const& channels)
    //---------------------------------------------------------------------------------------------
    {
        auto buffer = default_port(m_polarity)->buffer<T*>();
        T* channel_proxy = new T[channels.size()];
        nchannels_t n = 0;

        for (const auto& channel : channels) {
            m_nchannels = std::max(channel, m_nchannels);
            channel_proxy[n] = buffer[channel];
            n++;
        }

        return channel_proxy;
    }

    //---------------------------------------------------------------------------------------------
    void
    add_connections(QVector<ExternalConnection>& connections)
    //---------------------------------------------------------------------------------------------
    {
        m_connections.append(connections);
    }

    //---------------------------------------------------------------------------------------------
    nchannels_t
    nchannels() const { return m_nchannels; }

    //---------------------------------------------------------------------------------------------
    QVector<ExternalConnection>&
    connections()
    //---------------------------------------------------------------------------------------------
    {
        return m_connections;
    }

    //---------------------------------------------------------------------------------------------
    template<typename T> T
    buffer()  { return default_port(m_polarity)->buffer<T>(); }

};

//=================================================================================================
class AudioInput : public IOBase
//=================================================================================================
{
    Q_OBJECT
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT    (audio_out, 0)

public:
    //---------------------------------------------------------------------------------------------
    AudioInput() { m_polarity = Polarity::Output; }
};

//=================================================================================================
class MidiInput : public IOBase
//=================================================================================================
{
    Q_OBJECT
    WPN_DECLARE_DEFAULT_MIDI_OUTPUT (midi_out, 0)

public:
    //---------------------------------------------------------------------------------------------
    MidiInput() { m_polarity = Polarity::Output; }
};

//=================================================================================================
class AudioOutput : public IOBase
//=================================================================================================
{
    Q_OBJECT
    WPN_DECLARE_DEFAULT_AUDIO_INPUT (audio_in, 0)

public:
    //---------------------------------------------------------------------------------------------
    AudioOutput() { m_polarity = Polarity::Input; }
};

//=================================================================================================
class MidiOutput : public IOBase
//=================================================================================================
{
    Q_OBJECT
    WPN_DECLARE_DEFAULT_AUDIO_INPUT (midi_in, 0)

public:
    //---------------------------------------------------------------------------------------------
    MidiOutput() { m_polarity = Polarity::Input; }
};


//-------------------------------------------------------------------------------------------------
class Backend : public QObject
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

public:
    //---------------------------------------------------------------------------------------------
    enum Values
    //---------------------------------------------------------------------------------------------
    {
            None

        ,   Default

#ifdef __linux__
#ifndef __ANDROID__
        ,   Alsa
#endif
#endif
#ifdef WPN114AUDIO_PULSEAUDIO
//      ,   PulseAudio
#endif
#ifdef __APPLE__
//      ,   CoreAudio
#endif
#ifdef WPN114AUDIO_JACK
        ,   Jack
#endif
#ifdef WPN114AUDIO_VSTHOST
//      ,   VSTHost
#endif
#ifdef ANDROID
        ,   Qt
#endif
    };

    Q_ENUM (Values)
};

//=================================================================================================
class External : public QObject, public QQmlParserStatus
// this is a generic Node wrapper for the external backends
// it embeds an AbstrackBackend object
// and will send it commands and notifications whenever a property changes
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_INTERFACES (QQmlParserStatus)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (Backend::Values backend READ backend WRITE set_backend)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QString name READ name WRITE set_name)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (bool running READ running WRITE set_running)

    //---------------------------------------------------------------------------------------------
    ExternalBase*
    m_backend = nullptr;

    //---------------------------------------------------------------------------------------------
    bool
    m_complete  = false,
    m_running   = false;

    //---------------------------------------------------------------------------------------------
    QString
    m_name = "wpn114audio";

public:

    //-------------------------------------------------------------------------------------------------
    External() {}

    //-------------------------------------------------------------------------------------------------
    virtual
    ~External() override { delete m_backend; }

    //-------------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}

    //-------------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override;

    //-------------------------------------------------------------------------------------------------
    Q_SIGNAL void
    connected();

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    run()
    //-------------------------------------------------------------------------------------------------
    {
        if (m_complete)
            m_backend->run();
    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    stop()
    //-------------------------------------------------------------------------------------------------
    {
        if (m_running & m_complete)
            m_backend->stop();
    }

    //-------------------------------------------------------------------------------------------------
    void
    on_rate_changed(sample_t rate) { m_backend->on_sample_rate_changed(rate); }

    //-------------------------------------------------------------------------------------------------
    Backend::Values
    backend() const { return m_backend_id; }

    //-------------------------------------------------------------------------------------------------
    void
    set_backend(Backend::Values backend)
    //-------------------------------------------------------------------------------------------------
    {       
        m_backend_id = backend;
    }

    //---------------------------------------------------------------------------------------------
    bool
    running() const { return m_running; }

    //---------------------------------------------------------------------------------------------
    void
    set_running(bool running)
    //---------------------------------------------------------------------------------------------
    {
        m_running = running;
    }

    //-------------------------------------------------------------------------------------------------
    QString
    name() const { return m_name; }

    //-------------------------------------------------------------------------------------------------
    void
    set_name(QString name) { m_name = name; }

    //-------------------------------------------------------------------------------------------------
    AudioInput&
    audio_inputs() { return m_audio_inputs; }

    MidiInput&
    midi_inputs() { return m_midi_inputs; }

    AudioOutput&
    audio_outputs() { return m_audio_outputs; }

    MidiOutput&
    midi_outputs() { return m_midi_outputs; }

private:

    AudioInput
    m_audio_inputs;

    MidiInput
    m_midi_inputs;

    AudioOutput
    m_audio_outputs;

    MidiOutput
    m_midi_outputs;

    Backend::Values
    m_backend_id = Backend::None;

};

//=================================================================================================
class IOProxy : public Node
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (Type type READ type WRITE set_type)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant channels READ channels WRITE set_channels)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant connections READ targets WRITE set_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int nchannels READ nchannels WRITE set_nchannels)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int offset READ offset WRITE set_offset)

    nchannels_t
    m_nchannels = 0;

    nchannels_t
    m_offset = 0;

public:

    //---------------------------------------------------------------------------------------------
    enum Type { Audio, Midi }; Q_ENUM (Type)

    //---------------------------------------------------------------------------------------------
    Type
    type() const { return m_type; }

    //---------------------------------------------------------------------------------------------
    void
    set_type(Type t) { m_type = t; }

    //---------------------------------------------------------------------------------------------
    nchannels_t
    nchannels() const { return m_nchannels; }

    nchannels_t
    offset() const { return m_offset; }

    //---------------------------------------------------------------------------------------------
    void
    set_nchannels(nchannels_t nchannels)
    //---------------------------------------------------------------------------------------------
    {
        m_nchannels = nchannels;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_offset(nchannels_t offset)
    //---------------------------------------------------------------------------------------------
    {
        m_offset = offset;
    }

    //---------------------------------------------------------------------------------------------
    QVariant
    channels() const { return QVariant::fromValue(m_channels); }

    //---------------------------------------------------------------------------------------------
    QVariant
    targets() const { return m_targets; }

    //---------------------------------------------------------------------------------------------
    QVector<ExternalConnection>&
    connections() { return m_connections; }

    //---------------------------------------------------------------------------------------------
    void
    set_targets(QVariant variant) { m_targets = variant; }

    //---------------------------------------------------------------------------------------------
    void
    set_channels(QVariant var)
    //---------------------------------------------------------------------------------------------
    {
        if (var.canConvert<int>())
            m_channels.push_back(var.value<int>());

        else if (var.canConvert<QVariantList>())
            for (auto& channel : var.value<QVariantList>())
                set_channels(channel);
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {
        if (m_channels.empty())
            for (nchannels_t n = m_offset; n < m_offset+m_nchannels; n++)
                m_channels.push_back(n);

        parse_external_connections();
        Node::componentComplete();
    }

    //---------------------------------------------------------------------------------------------
    void
    parse_external_connections()
    //---------------------------------------------------------------------------------------------
    {
        if (m_targets.canConvert<QString>()) {
            ExternalConnection con;
            con.target = m_targets.value<QString>();
            m_connections.push_back(con);
        }

        else if (m_targets.canConvert<QVariantList>())
        {
            auto list = m_targets.value<QVariantList>();
            auto targets = list[0];
            auto routing = list[1];

            if (targets.canConvert<QString>())
            {
                ExternalConnection con;
                con.target = targets.value<QString>();

                if (routing.canConvert<int>()) {
                    // single routing argument
                    uint8_t source = m_channels[0];
                    uint8_t dest = routing.value<uint8_t>();
                    con.routing.append(source, dest);
                }

                else if (routing.canConvert<QVariantList>())
                {
                    auto lrouting = routing.value<QVariantList>();
                    con.routing = Routing(lrouting);

                    for (int n = 0; n < con.routing.ncables(); ++n)
                        // replace source index by channel
                        con.routing[n][0] = m_channels[con.routing[n][0]];
                }

                m_connections.push_back(con);
            }

            else if (targets.canConvert<QVariantList>())
            {
                // TODO...
            }
        }
    }

protected:

    Type
    m_type = Type::Audio;

    QVector<nchannels_t>
    m_channels;

    QVector<ExternalConnection>
    m_connections;

    QVariant
    m_targets;
};

//---------------------------------------------------------------------------------------------
class OutputProxy : public IOProxy
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT     (audio_in, 0)
    WPN_DECLARE_DEFAULT_MIDI_INPUT      (midi_in, 0)

public:

    //---------------------------------------------------------------------------------------------
    OutputProxy() { m_name = "output_proxy"; }

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {        
        IOProxy::componentComplete();

        auto ext = Graph::instance().external();

        if (m_type == Type::Audio) {
            auto& out = ext->audio_outputs();
            auto abuf = out.add_proxy<sample_t*>(m_channels);
            m_audio_in.set_buffer<audiobuffer_t>(abuf);
            out.add_connections(m_connections);

        } else {
            auto& out = ext->midi_outputs();
            auto mbuf = out.add_proxy<midibuffer*>(m_channels);
            m_midi_in.set_buffer<midibuffer_t>(mbuf);
            out.add_connections(m_connections);
        } 
    }
};

//---------------------------------------------------------------------------------------------
class InputProxy : public IOProxy
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT    (audio_out, 0)
    WPN_DECLARE_DEFAULT_MIDI_OUTPUT     (midi_out, 0)

public:

    //---------------------------------------------------------------------------------------------
    InputProxy()
    //---------------------------------------------------------------------------------------------
    {
        m_name = "input_proxy";
        m_dispatch = Dispatch::Chain;
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {
        IOProxy::componentComplete();

        auto ext = Graph::instance().external();

        if (m_type == Type::Audio) {
            auto& out = ext->audio_inputs();
            auto abuf = out.add_proxy<sample_t*>(m_channels);
            m_audio_out.set_buffer<audiobuffer_t>(abuf);
            out.add_connections(m_connections);
        } else {
            auto& out = ext->midi_inputs();
            auto mbuf = out.add_proxy<midibuffer*>(m_channels);
            m_midi_out.set_buffer<midibuffer_t>(mbuf);
            out.add_connections(m_connections);
        }
    }
};
