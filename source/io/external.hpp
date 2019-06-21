#pragma once

#include <source/graph.hpp>
#include <jack/jack.h>

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


class External;

//---------------------------------------------------------------------------------------------
class JackExternal : public ExternalBase
//---------------------------------------------------------------------------------------------
{
    //---------------------------------------------------------------------------------------------
    External&
    m_parent;

    //---------------------------------------------------------------------------------------------
    jack_client_t*
    m_client = nullptr;

    //---------------------------------------------------------------------------------------------
    std::vector<jack_port_t*>
    m_audio_inputs,
    m_audio_outputs,
    m_midi_inputs,
    m_midi_outputs;

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
    void
    register_ports(nchannels_t nchannels,
                   const char* port_mask,
                   const char* type,
                   unsigned long polarity,
                   std::vector<jack_port_t*>& target);

    //---------------------------------------------------------------------------------------------
    void
    connect_ports(std::vector<jack_port_t*>& ports,
                  unsigned long target_polarity,
                  const char *type,
                  QStringList const& targets,
                  Routing routing);

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

class Input;
class Output;

//=================================================================================================
class External : public QObject, public QQmlParserStatus
// this is a generic Node wrapper for the external backends
// it embeds an AbstrackBackend object
// and will send it commands and notifications whenever a property changes
//=================================================================================================
{
    Q_OBJECT

public:
    //---------------------------------------------------------------------------------------------
    enum Backend
    //---------------------------------------------------------------------------------------------
    {
        None         = 0,
//        Alsa         = 1,
        Jack         = 2,
//        PulseAudio   = 3,
//        CoreAudio    = 4,
//        VSTHost      = 5
    };

    Q_ENUM (Backend)

    //---------------------------------------------------------------------------------------------
    Q_INTERFACES (QQmlParserStatus)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (Backend backend READ backend WRITE set_backend)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QString name READ name WRITE set_name)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (bool running READ running WRITE set_running)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int numAudioInputs READ n_audio_inputs WRITE setn_audio_inputs)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int numAudioOutputs READ n_audio_outputs WRITE setn_audio_outputs)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int numMidiInputs READ n_midi_inputs WRITE setn_midi_inputs)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int numMidiOutputs READ n_midi_outputs WRITE setn_midi_outputs)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant outAudioTargets READ out_audio_targets WRITE set_out_audio_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant inAudioTargets READ in_audio_targets WRITE set_in_audio_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant outMidiTargets READ out_midi_targets WRITE set_out_midi_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant inMidiTargets READ in_midi_targets WRITE set_in_midi_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariantList outAudioRouting READ out_audio_routing WRITE set_out_audio_routing)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariantList inAudioRouting READ in_audio_routing WRITE set_in_audio_routing)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariantList outMidiRouting READ out_midi_routing WRITE set_out_midi_routing)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariantList inMidiRouting READ in_midi_routing WRITE set_in_midi_routing)

    //---------------------------------------------------------------------------------------------
    QStringList
    m_in_audio_targets,
    m_out_audio_targets,
    m_in_midi_targets,
    m_out_midi_targets;

    //---------------------------------------------------------------------------------------------
    Routing
    m_out_audio_routing,
    m_out_midi_routing,
    m_in_audio_routing,
    m_in_midi_routing;

    //---------------------------------------------------------------------------------------------
    uint8_t
    m_out_audio_nchannels   = 2,
    m_out_midi_nchannels    = 0,
    m_in_audio_nchannels    = 0,
    m_in_midi_nchannels     = 0;

    //---------------------------------------------------------------------------------------------
    ExternalBase*
    m_backend = nullptr;

    //---------------------------------------------------------------------------------------------
    bool
    m_complete  = false,
    m_running   = false;

    //---------------------------------------------------------------------------------------------
    Backend
    m_backend_id = None;

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
    componentComplete() override
    //-------------------------------------------------------------------------------------------------
    {
        m_complete = true;
        m_backend = new JackExternal(this);

        qDebug() << "[GRAPH] external configuration complete";

        if (m_running)
            m_backend->run();
    }

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
    Backend
    backend() const { return m_backend_id; }

    //-------------------------------------------------------------------------------------------------
    void
    set_backend(Backend backend)
    //-------------------------------------------------------------------------------------------------
    {
        if (!m_complete)
            return;

        if (m_backend)
            delete m_backend;

        switch(backend)
        {
        case Jack:
        {
            m_backend = new JackExternal(this);

            if (m_running)
                m_backend->run();
        }
        }
    }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_audio_inputs() const { return m_in_audio_nchannels; }

    void
    setn_audio_inputs(uint8_t nchannels)
    {
        m_in_audio_nchannels = nchannels;
    }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_audio_outputs() const { return m_out_audio_nchannels; }

    void
    setn_audio_outputs(uint8_t nchannels)
    {
        m_out_audio_nchannels = nchannels;
    }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_midi_inputs() const { return m_in_midi_nchannels; }

    void
    setn_midi_inputs(uint8_t nchannels)
    {
        m_in_midi_nchannels = nchannels;
    }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_midi_outputs() const { return m_out_midi_nchannels; }

    void
    setn_midi_outputs(uint8_t nchannels)
    {
        m_out_midi_nchannels = nchannels;
    }

    //---------------------------------------------------------------------------------------------
    QVariant
    in_audio_targets() const { return m_in_audio_targets; }

    QStringList const&
    in_audio_targets_list() const { return m_in_audio_targets; }
    // TODO:
    // not sure q_properties accept template functions as READ/WRITE targets
    // this is really annoying...

    //---------------------------------------------------------------------------------------------
    QVariant
    out_audio_targets() const { return m_out_audio_targets; }

    QStringList const&
    out_audio_targets_list() const { return m_out_audio_targets; }

    //---------------------------------------------------------------------------------------------
    QVariant
    in_midi_targets() const { return m_in_midi_targets; }

    QStringList const&
    in_midi_targets_list() const { return m_in_midi_targets; }

    //---------------------------------------------------------------------------------------------
    QVariant
    out_midi_targets() const { return m_out_midi_targets; }

    QStringList const&
    out_midi_targets_list() const { return m_out_midi_targets; }

    //---------------------------------------------------------------------------------------------
    void
    set_in_audio_targets(QVariant variant)
    //---------------------------------------------------------------------------------------------
    {
        auto list = variant.value<QStringList>();

        if (m_in_audio_targets != list) {
            m_in_audio_targets = list;
        }

        if (m_complete)
            m_backend->on_input_audio_targets_changed(m_in_audio_targets, {});
    }

    //---------------------------------------------------------------------------------------------
    void
    set_out_audio_targets(QVariant variant)
    //---------------------------------------------------------------------------------------------
    {
        m_out_audio_targets = variant.value<QStringList>();
    }

    //---------------------------------------------------------------------------------------------
    void
    set_in_midi_targets(QVariant variant)
    //---------------------------------------------------------------------------------------------
    {
        m_in_midi_targets = variant.value<QStringList>();
    }

    //---------------------------------------------------------------------------------------------
    void
    set_out_midi_targets(QVariant variant)
    //---------------------------------------------------------------------------------------------
    {
        m_out_midi_targets = variant.value<QStringList>();
    }

    //---------------------------------------------------------------------------------------------
    QVariantList
    in_audio_routing() const    { return m_in_audio_routing; }

    //---------------------------------------------------------------------------------------------
    QVariantList
    out_audio_routing() const   { return m_out_audio_routing; }

    //---------------------------------------------------------------------------------------------
    QVariantList
    in_midi_routing() const     { return m_in_midi_routing; }

    //---------------------------------------------------------------------------------------------
    QVariantList
    out_midi_routing() const    { return m_out_midi_routing; }

    //---------------------------------------------------------------------------------------------
    void
    set_in_audio_routing(QVariantList variant)
    //---------------------------------------------------------------------------------------------
    {
        m_in_audio_routing = variant;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_out_audio_routing(QVariantList variant)
    //---------------------------------------------------------------------------------------------
    {
        m_out_audio_routing = variant;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_in_midi_routing(QVariantList variant)
    //---------------------------------------------------------------------------------------------
    {
        m_in_midi_routing = variant;
    }

    //---------------------------------------------------------------------------------------------
    void
    set_out_midi_routing(QVariantList variant)
    //---------------------------------------------------------------------------------------------
    {
        m_out_midi_routing = variant;
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

    //---------------------------------------------------------------------------------------------
    void
    register_input(Input* input);

    //---------------------------------------------------------------------------------------------
    void
    register_output(Output* output);

    //---------------------------------------------------------------------------------------------
    std::vector<Input*>&
    midi_inputs() { return m_midi_inputs; }

    //---------------------------------------------------------------------------------------------
    std::vector<Output*>&
    midi_outputs() { return m_midi_outputs; }

    //---------------------------------------------------------------------------------------------
    std::vector<Input*>&
    audio_inputs() { return m_audio_inputs; }

    //---------------------------------------------------------------------------------------------
    std::vector<Output*>&
    audio_outputs() { return m_audio_outputs; }

private:

    std::vector<Input*>
    m_audio_inputs,
    m_midi_inputs;

    std::vector<Output*>
    m_audio_outputs,
    m_midi_outputs;

};

//---------------------------------------------------------------------------------------------
class Output : public Node
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_INPUT(audio_in, 0)
    WPN_DECLARE_DEFAULT_MIDI_INPUT(midi_in, 0)

    //---------------------------------------------------------------------------------------------
    enum Type { Audio, Midi }; Q_ENUM (Type)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (Type type READ type WRITE set_type)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant channels READ channels WRITE set_channels)

    //---------------------------------------------------------------------------------------------
    Type
    m_type = Type::Audio;

    //---------------------------------------------------------------------------------------------
    QVector<int>
    m_channels;

public:

    //---------------------------------------------------------------------------------------------
    Output() { m_name = "Output"; }

    //---------------------------------------------------------------------------------------------
    Type
    type() const { return m_type; }

    //---------------------------------------------------------------------------------------------
    void
    set_type(Type t) { m_type = t; }

    //---------------------------------------------------------------------------------------------
    QVariant
    channels() const { return QVariant::fromValue(m_channels); }

    QVector<int>&
    channels_vec() { return m_channels; }

    //---------------------------------------------------------------------------------------------
    void
    set_channels(QVariant var)
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
        Graph::instance().external()->register_output(this);

        if (m_channels.isEmpty())
        {
            if (m_type == Type::Audio)
            {
                auto nchannels = Graph::instance().external()->n_audio_outputs();
                for (int n = 0; n < nchannels; ++n)
                     m_channels.push_back(n);

                m_audio_in.set_nchannels(nchannels);
            }
            else
            {
                auto nchannels = Graph::instance().external()->n_midi_outputs();
                for (int n = 0; n < nchannels; ++n)
                     m_channels.push_back(n);

                m_midi_in.set_nchannels(nchannels);
            }
        }
        else
        {
            if (m_type == Type::Audio)
                 m_audio_in.set_nchannels(m_channels.count());
            else m_midi_in.set_nchannels(m_channels.count());

        }

        Node::componentComplete();
    }
};

//---------------------------------------------------------------------------------------------
class Input : public Node
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT(audio_out, 0)
    WPN_DECLARE_DEFAULT_MIDI_OUTPUT(midi_out, 0)

    //---------------------------------------------------------------------------------------------
    enum Type { Audio, Midi }; Q_ENUM (Type)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (Type type READ type WRITE set_type)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant channels READ channels WRITE set_channels)

    //---------------------------------------------------------------------------------------------
    Type
    m_type = Type::Audio;

    //---------------------------------------------------------------------------------------------
    QVector<int>
    m_channels;

public:

    //---------------------------------------------------------------------------------------------
    Input() { m_name = "Input"; }

    //---------------------------------------------------------------------------------------------
    Type
    type() const { return m_type; }

    //---------------------------------------------------------------------------------------------
    void
    set_type(Type t) { m_type = t; }

    //---------------------------------------------------------------------------------------------
    QVariant
    channels() const { return QVariant::fromValue(m_channels); }

    QVector<int>&
    channels_vec() { return m_channels; }

    //---------------------------------------------------------------------------------------------
    void
    set_channels(QVariant var)
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
        Graph::instance().external()->register_input(this);

        if (m_channels.isEmpty())
        {
            if (m_type == Type::Audio)
            {
                auto nchannels = Graph::instance().external()->n_audio_inputs();
                for (int n = 0; n < nchannels; ++n)
                     m_channels.push_back(n);

                m_audio_out.set_nchannels(nchannels);
            }
            else
            {
                auto nchannels = Graph::instance().external()->n_midi_inputs();
                for (int n = 0; n < nchannels; ++n)
                     m_channels.push_back(n);

                m_midi_out.set_nchannels(nchannels);
            }
        }
        else
        {
            if (m_type == Type::Audio)
                 m_audio_out.set_nchannels(m_channels.count());
            else m_midi_out.set_nchannels(m_channels.count());

        }

        Node::componentComplete();
    }
};
