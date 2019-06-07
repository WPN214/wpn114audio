#pragma once

#include <source/graph.hpp>
#include <jack/jack.h>

//---------------------------------------------------------------------------------------------
class ExternalBase
//---------------------------------------------------------------------------------------------
{

public:
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
    rwrite(pool& inputs, pool& outputs, vector_t nframes) = 0;

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
    register_ports(nchannels_t nchannels, const char* port_mask, const char* type,
                   int polarity, std::vector<jack_port_t*>& target);

    //---------------------------------------------------------------------------------------------
    void
    connect_ports(std::vector<jack_port_t*>& ports, int target_polarity, const char *type,
                  QStringList const& targets, Routing routing);

public:

    //---------------------------------------------------------------------------------------------
    JackExternal(External* parent);

    //---------------------------------------------------------------------------------------------
    virtual ~JackExternal() override
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
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override;

    //---------------------------------------------------------------------------------------------
    virtual void
    on_name_changed(QString &name) override {}

    // TODO: override the other ones...

};

//=================================================================================================
class External : public Node
// this is a generic Node wrapper for the external backends
// it embeds an AbstrackBackend object
// and will send it commands and notifications whenever a property changes
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    WPN_ENUM_INPUTS     (audioInputs, midiInputs)

    //---------------------------------------------------------------------------------------------
    WPN_ENUM_OUTPUTS    (audioOutputs, midiOutputs)

    //---------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (audioInputs, Socket::Audio, 0)

    //---------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (midiInputs, Socket::Midi_1_0, 0)

    //---------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE  (audioOutputs, Socket::Audio, 0)

    //---------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE  (midiOutputs, Socket::Midi_1_0, 0)

    //---------------------------------------------------------------------------------------------
    enum Backend
    //---------------------------------------------------------------------------------------------
    {
        None         = 0,
        Alsa         = 1,
        Jack         = 2,
        PulseAudio   = 3,
        Core         = 4,
        VSTHost      = 5
    };

    Q_ENUM (Backend)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int numAudioInputs READ n_audio_inputs WRITE setn_audio_inputs)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int numAudioOutputs READ n_audio_outputs WRITE setn_audio_outputs)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int numMidiInputs READ n_midi_inputs WRITE setn_midi_inputs)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (int numMidiOutputs READ n_midi_outputs WRITE setn_midi_outputs)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (Backend backend READ backend WRITE set_backend)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant inAudioTargets READ in_audio_targets WRITE set_in_audio_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant outAudioTargets READ out_audio_targets WRITE set_out_audio_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant inMidiTargets READ in_midi_targets WRITE set_in_midi_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariant outMidiTargets READ out_midi_targets WRITE set_out_midi_targets)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariantList inAudioRouting READ in_audio_routing WRITE set_in_audio_routing)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariantList outAudioRouting READ out_audio_routing WRITE set_out_audio_routing)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariantList inMidiRouting READ in_midi_routing WRITE set_in_midi_routing)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QVariantList outMidiRouting READ out_midi_routing WRITE set_out_midi_routing)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (QString name READ name WRITE set_name)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY  (bool running READ running WRITE set_running)

    //---------------------------------------------------------------------------------------------
    QStringList
    m_in_audio_targets,
    m_out_audio_targets,
    m_in_midi_targets,
    m_out_midi_targets;

    //---------------------------------------------------------------------------------------------
    Routing
    m_in_audio_routing,
    m_out_audio_routing,
    m_in_midi_routing,
    m_out_midi_routing;

    //---------------------------------------------------------------------------------------------
    ExternalBase*
    m_backend = nullptr;

    //---------------------------------------------------------------------------------------------
    bool
    m_complete  = false,
    m_running   = false;

    //---------------------------------------------------------------------------------------------
    QString
    m_name = "wpn114audio-device";

    //---------------------------------------------------------------------------------------------
    uint8_t
    m_n_audio_inputs    = 0,
    m_n_audio_outputs   = 2,
    m_n_midi_inputs     = 0,
    m_n_midi_outputs    = 0;

    //---------------------------------------------------------------------------------------------
    Backend
    m_backend_id = None;

public:


    //-------------------------------------------------------------------------------------------------
    External() {}

    //-------------------------------------------------------------------------------------------------
    virtual ~External() override { delete m_backend; }

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

        if (m_running)
            m_backend->run();
    }

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
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //-------------------------------------------------------------------------------------------------
    {
        m_backend->rwrite(inputs, outputs, nframes);
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override
    //-------------------------------------------------------------------------------------------------
    {
        m_backend->on_sample_rate_changed(rate);
    }

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
    n_audio_inputs() const { return m_n_audio_inputs; }

    //-------------------------------------------------------------------------------------------------
    void
    setn_audio_inputs(uint8_t n_inputs)
    //-------------------------------------------------------------------------------------------------
    {
        if (m_complete && n_inputs != m_n_audio_inputs) {
            m_backend->on_audio_inputs_changed(n_inputs);
        }

        m_n_audio_inputs = n_inputs;
    }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_audio_outputs() const { return m_n_audio_outputs; }

    //-------------------------------------------------------------------------------------------------
    void
    setn_audio_outputs(uint8_t n_outputs)
    //-------------------------------------------------------------------------------------------------
    {
        if (m_complete && n_outputs != m_n_audio_outputs) {
            m_backend->on_audio_outputs_changed(n_outputs);
        }

        m_n_audio_outputs = n_outputs;
    }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_midi_inputs() const { return m_n_midi_inputs; }

    //-------------------------------------------------------------------------------------------------
    void
    setn_midi_inputs(uint8_t n_inputs)
    //-------------------------------------------------------------------------------------------------
    {
        if (m_complete && n_inputs != m_n_midi_inputs) {
            m_backend->on_midi_inputs_changed(n_inputs);
        }

        m_n_midi_inputs = n_inputs;
    }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_midi_outputs() const { return m_n_midi_outputs; }

    //-------------------------------------------------------------------------------------------------
    void
    setn_midi_outputs(uint8_t n_outputs)
    //-------------------------------------------------------------------------------------------------
    {
        if (m_complete && n_outputs != m_n_midi_outputs) {
            m_backend->on_midi_outputs_changed(n_outputs);
        }

        m_n_midi_outputs = n_outputs;
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
    in_audio_routing() const    { return m_in_audio_routing.to_qvariant(); }

    //---------------------------------------------------------------------------------------------
    QVariantList
    out_audio_routing() const   { return m_out_audio_routing.to_qvariant(); }

    //---------------------------------------------------------------------------------------------
    QVariantList
    in_midi_routing() const     { return m_in_midi_routing.to_qvariant(); }

    //---------------------------------------------------------------------------------------------
    QVariantList
    out_midi_routing() const    { return m_out_midi_routing.to_qvariant(); }

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
    QString
    name() const override { return m_name; }

    //---------------------------------------------------------------------------------------------
    void
    set_name(QString name)
    //---------------------------------------------------------------------------------------------
    {
        m_name = name;
        m_backend->on_name_changed(m_name);
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
};


