#pragma once

#include <QObject>
#include <QQmlParserStatus>
#include <QQmlProperty>
#include <QQmlPropertyValueSource>
#include <QQmlListProperty>
#include <QVector>
#include <QVariant>
#include <QJSValue>
#include <QtDebug>
#include <cmath>

#include <wpn114audio/basic_types.hpp>
#include <wpn114audio/execution.hpp>

// --------------------------------------------------------------------------------------------------
// CONVENIENCE MACRO DEFINITIONS
// --------------------------------------------------------------------------------------------------

#define WPN_PORT(_type, _polarity, _name, _def, _nchannels) \
    private: Q_PROPERTY(Port* _name READ get_##_name WRITE set_##_name NOTIFY _name##_changed) \
    public: Q_SIGNAL void _name##_changed(); \
    public: Port m_##_name { this, #_name, _type, _polarity, _def, _nchannels }; \
    public: Port* get_##_name() { return &m_##_name; } \
    void set_##_name(Port* p) { m_##_name.assign(p); }

// ------------------------------------------------------------------------------------------------
#define WPN_DECLARE_DEFAULT_AUDIO_PORT(_name, _polarity, _nchannels) \
    WPN_PORT(Port::Audio, _polarity, _name, true, _nchannels)

#define WPN_DECLARE_AUDIO_PORT(_name, _polarity, _nchannels) \
    WPN_PORT(Port::Audio, _polarity, _name, false, _nchannels)

// ------------------------------------------------------------------------------------------------
#define WPN_DECLARE_DEFAULT_MIDI_PORT(_name, _polarity, _nchannels) \
    WPN_PORT(Port::Midi_1_0, _polarity, _name, true, _nchannels)

#define WPN_DECLARE_MIDI_PORT(_name, _polarity, _nchannels) \
    WPN_PORT(Port::Midi_1_0, _polarity, _name, false, _nchannels)

// ------------------------------------------------------------------------------------------------
#define WPN_DECLARE_DEFAULT_AUDIO_INPUT(_name, _nchannels) \
    WPN_DECLARE_DEFAULT_AUDIO_PORT(_name, Polarity::Input, _nchannels)

#define WPN_DECLARE_DEFAULT_AUDIO_OUTPUT(_name, _nchannels) \
    WPN_DECLARE_DEFAULT_AUDIO_PORT(_name, Polarity::Output, _nchannels)

// ------------------------------------------------------------------------------------------------
#define WPN_DECLARE_DEFAULT_MIDI_INPUT(_name, _nchannels) \
    WPN_DECLARE_DEFAULT_MIDI_PORT(_name, Polarity::Input, _nchannels)

#define WPN_DECLARE_DEFAULT_MIDI_OUTPUT(_name, _nchannels) \
    WPN_DECLARE_DEFAULT_MIDI_PORT(_name, Polarity::Output, _nchannels)

// ------------------------------------------------------------------------------------------------
#define WPN_DECLARE_AUDIO_INPUT(_name, _nchannels) \
    WPN_DECLARE_AUDIO_PORT(_name, Polarity::Input, _nchannels)

#define WPN_DECLARE_AUDIO_OUTPUT(_name, _nchannels) \
    WPN_DECLARE_AUDIO_PORT(_name, Polarity::Output, _nchannels)

#define WPN_DECLARE_MIDI_INPUT(_name, _nchannels) \
    WPN_DECLARE_MIDI_PORT(_name, Polarity::Input, _nchannels)

#define WPN_DECLARE_MIDI_OUTPUT(_name, _nchannels) \
    WPN_DECLARE_MIDI_PORT(_name, Polarity::Output, _nchannels)

#define WPN_SET_EXEC(_type) \
    virtual void* xbuild() const override { return new _type; }

#define WPN_SET_INIT(_init) \
    virtual int_fn_t init_fn() const override { return _init; }

#define WPN_SET_PROC(_proc) \
    virtual prc_fn_t proc_fn() const override { return _proc; }


//-------------------------------------------------------------------------------------------------

namespace wpn114
{
namespace audio
{
namespace qt
{

enum class Polarity { Output = 0, Input = 1 };

class Connection;
class Node;
class Port;

//=================================================================================================
class Dispatch : public QObject
//=================================================================================================
{
    Q_OBJECT

public:

    enum Values {
        UpwardsMerge,
        UpwardsExpand,
        DownwardsChain,
        DownwardsParallel
    };

    Q_ENUM (Values)
};

//=================================================================================================
class Routing : public QObject
// connection matrix between 2 Ports
// materialized as a QVariantList within QML
//=================================================================================================
{
    Q_OBJECT

public:

    // --------------------------------------------------------------------------------------------
    using cable = std::array<uint8_t, 2>;

    // --------------------------------------------------------------------------------------------
    Routing() {}

    // --------------------------------------------------------------------------------------------
    Routing(Routing const& cp) : m_routing(cp.m_routing) {}

    // --------------------------------------------------------------------------------------------
    Routing(QVariantList list)
    // creates Routing object from QVariantList within QML
    {
        if (list.empty())
            return;

        for (int n = 0; n < list.count(); n += 2)  {
            m_routing.push_back({
            static_cast<uint8_t>(list[n].toInt()),
            static_cast<uint8_t>(list[n+1].toInt())});
        }
    }

    // --------------------------------------------------------------------------------------------
    Routing&
    operator=(Routing const& cp) { m_routing = cp.m_routing; return *this; }

    // --------------------------------------------------------------------------------------------
    cable&
    operator[](nchannels_t index) { return m_routing[index]; }
    // returns routing matrix 'cable' at index

    // --------------------------------------------------------------------------------------------
    void
    append(cable const& c) { m_routing.push_back(c); }

    void
    append(uint8_t source, uint8_t dest)
    {
        cable c = { source, dest };
        m_routing.emplace_back(c);
    }

    // --------------------------------------------------------------------------------------------
    nchannels_t
    ncables() const { return static_cast<nchannels_t>(m_routing.size()); }

    // --------------------------------------------------------------------------------------------
    bool
    null() const { return m_routing.empty(); }
    // returns true if Routing is not explicitely defined

    // --------------------------------------------------------------------------------------------
    operator
    QVariantList() const
    // --------------------------------------------------------------------------------------------
    {
        QVariantList lst;
        for (auto& cable : m_routing) {
            QVariantList cbl;
            cbl.append(cable[0]);
            cbl.append(cable[1]);
            lst.append(cbl);
        }
        return lst;
    }

private:
    // --------------------------------------------------------------------------------------------
    std::vector<cable>
    m_routing;
};

//=================================================================================================
class Connection : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//=================================================================================================
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Port* source MEMBER m_source)
    // Connection source Port (output polarity) from which the signal will transit

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Port* dest MEMBER m_dest)
    // Connection dest Port (input polarity) to which the signal will transit

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (QVariantList routing READ routing WRITE set_routing)
    // Connection's channel-to-channel routing matrix
    // materialized as a QVariantList within QML
    // e.g. [0, 1, 1, 0] will connect output 0 to input 1 and output 1 to input 0
    // TODO: [[0,1],[1,0]] format as well

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal mul READ mul WRITE set_mul)
    // Connection's amplitude level (linear)
    // scales the amplitude of the input signal to transit to the output
    // this property might be overriden or scaled by Port's or Node's level property
    // TODO: prefader/postfader property

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal add READ add WRITE set_add)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (bool muted READ muted WRITE set_muted)
    // Connection will still process when muted
    // but will output zeros instead of the normal signal

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (bool active READ active WRITE set_active)
    // if inactive, Connection won't process the upstream part of the Graph
    // to which it is connected

    // --------------------------------------------------------------------------------------------
    Q_INTERFACES (QQmlParserStatus QQmlPropertyValueSource)

    // --------------------------------------------------------------------------------------------
    friend class Port;
    friend class Node;

public:

    // --------------------------------------------------------------------------------------------
    Connection() {}

    // --------------------------------------------------------------------------------------------
    Connection(Connection const& cp) :
        m_nchannels (cp.m_nchannels),
        m_source    (cp.m_source),
        m_dest      (cp.m_dest),
        m_routing   (cp.m_routing),
        m_active    (cp.m_active),
        m_muted     (cp.m_muted),
        m_mul       (cp.m_mul),
        m_add       (cp.m_add) {}
    // copy constructor, don't really know in which case it should/will be used

    // --------------------------------------------------------------------------------------------
    Connection(Port& source, Port& dest, Routing matrix);
    // constructor accessed from C++

    // --------------------------------------------------------------------------------------------
    Connection&
    operator=(Connection const& cp)
    // --------------------------------------------------------------------------------------------
    {
        m_source     = cp.m_source;
        m_dest       = cp.m_dest;
        m_routing    = cp.m_routing;
        m_nchannels  = cp.m_nchannels;
        m_mul        = cp.m_mul;
        m_add        = cp.m_add;
        m_active     = cp.m_active;
        m_muted      = cp.m_muted;

        return *this;
    }

    // --------------------------------------------------------------------------------------------
    bool
    operator==(Connection const& rhs) noexcept
    {
        return m_source == rhs.m_source && m_dest == rhs.m_dest;
    }

    // --------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}
    // unused interface override

    virtual void
    componentComplete() override;

    // --------------------------------------------------------------------------------------------
    virtual void
    setTarget(const QQmlProperty& target) override;
    // qml property binding, e. g.:
    // Connection on 'Port' { level: db(-12); routing: [0, 1] }

    // --------------------------------------------------------------------------------------------
    void
    update();
    // this might be temporary, this is called when source or dest's number of channels changes

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    activate() noexcept { m_active = true; }

    Q_INVOKABLE void
    deactivate() noexcept { m_active = false; }

    void
    set_active(bool active) noexcept { m_active = active; }

    bool
    active() const noexcept { return m_active; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    mute() noexcept {  m_muted = true; }

    Q_INVOKABLE void
    unmute() noexcept { m_muted = false; }

    void
    set_muted(bool muted) noexcept { m_muted = muted; }

    bool
    muted() const noexcept { return m_muted; }

    // --------------------------------------------------------------------------------------------
    Port*
    source() noexcept { return m_source; }
    // returns Connection's source Port (output polarity)

    Port*
    dest() noexcept { return m_dest; }
    // returns Connection's dest Port (input polarity)

    // --------------------------------------------------------------------------------------------
    QVariantList
    routing() const noexcept { return m_routing; }
    // returns Connection's routing matrix as QML formatted list

    void
    set_routing(QVariantList list) noexcept { m_routing = Routing(list); }
    // sets routing matrix from QML

    void
    set_routing(Routing matrix) noexcept { m_routing = matrix; }
    // sets routing matrix from C++

    // --------------------------------------------------------------------------------------------
    sample_t
    mul() const noexcept { return m_mul; }

    void
    set_mul(sample_t mul) noexcept { m_mul = mul; }

    // --------------------------------------------------------------------------------------------
    sample_t
    add() const noexcept { return m_add; }

    void
    set_add(sample_t add) noexcept { m_add = add; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE qreal
    db(qreal v) noexcept { return std::pow(10, v*.05); }
    // this is not an a->db function
    // it is a useful, readable and declarative way to signal that the value
    // used within qml is of the db unit.
    // consequently, it returns the value as a normal linear amplitude value.
    // in general, to ease-up calculations, the db should be calculated once and upfront

private:

    // --------------------------------------------------------------------------------------------
    bool
    m_muted {false};
    // if Connection is muted, it will still process its inputs
    // but will output 0 continuously

    bool
    m_active {true};
    // if Connection is inactive, it won't process its inputs
    // and consequently call the upstream graph

    // --------------------------------------------------------------------------------------------
    nchannels_t
    m_nchannels = 0;

    // --------------------------------------------------------------------------------------------
    Port*
    m_source = nullptr;

    Port*
    m_dest = nullptr;

    // --------------------------------------------------------------------------------------------
    Routing
    m_routing;

    // --------------------------------------------------------------------------------------------
    sample_t
    m_mul {1}, m_add {0};
};

//=================================================================================================
class Port : public QObject
// represents a node single input/output
// holding specific types of values
// and with nchannels_t channels
//=================================================================================================
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (bool muted MEMBER m_muted WRITE set_muted)
    // MUTED property: manages and overrides all Port connections mute property

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal value READ value WRITE set_value)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal mul MEMBER m_mul WRITE set_mul)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal add MEMBER m_add WRITE set_add)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (int nchannels MEMBER m_nchannels WRITE set_nchannels)
    // NCHANNELS property: for multichannel expansion
    // and dynamic channel setting/allocation

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (QVariantList routing READ routing WRITE set_routing)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (Type type READ type)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (QVariant assign READ get_assign WRITE assign)

    // --------------------------------------------------------------------------------------------
    friend class Connection;
    friend class Graph;
    friend class Node;

public:

    //---------------------------------------------------------------------------------------------
    enum Type
    //---------------------------------------------------------------------------------------------
    {
        // shows what values a specific Port is expecting to receive
        // or is explicitely outputing
        // a connection between a Midi Port and any other Port
        // that isn't Midi will be refused
        // otherwise, if connection types mismatch, a warning will be emitted

        Audio,
        // -1.0 to 1.0 normalized audio signal value

        Midi_1_0,
        // status byte + 2 other index/value bytes

//        Integer,
//        // a control integer value

//        FloatingPoint,
//        // a control floating point value

//        Cv,
//        // -5.0 to 5.0 control voltage values

//        Gate,
//        // a 0-1 latched value

//        Trigger
//        // a single pulse (1.f)
    };

    Q_ENUM (Type)

    // --------------------------------------------------------------------------------------------
    Port() {}

    // --------------------------------------------------------------------------------------------
    Port(Node* parent, QString name, Type type, Polarity polarity,
         bool is_default, uint8_t nchannels);

    // --------------------------------------------------------------------------------------------
    Port(Port const& cp) :
        // this is only called because of the macro Port definitions
        // it shouldn't be called otherwise in any other context
    // --------------------------------------------------------------------------------------------
        m_polarity  (cp.m_polarity)
      , m_index     (cp.m_index)
      , m_nchannels (cp.m_nchannels)
      , m_type      (cp.m_type)
      , m_name      (cp.m_name)
      , m_parent    (cp.m_parent)
      , m_default   (cp.m_default) {}

    // --------------------------------------------------------------------------------------------
    Port&
    operator=(Port const& cp)
    // same as for the copy constructor
    // --------------------------------------------------------------------------------------------
    {
        m_polarity   = cp.m_polarity;
        m_index      = cp.m_index;
        m_nchannels  = cp.m_nchannels;
        m_parent     = cp.m_parent;
        m_type       = cp.m_type;
        m_name       = cp.m_name;
        m_default    = cp.m_default;

        return *this;
    }

    // --------------------------------------------------------------------------------------------
    void
    assign(Port* p);
    // explicitely assign another Port to this one in ordre to make a connection

    void
    assign(QVariant variant);

    // --------------------------------------------------------------------------------------------
    QVariant
    get_assign() const { return QVariant(); }

    // --------------------------------------------------------------------------------------------
    qreal
    value() const { return m_value; }

    // --------------------------------------------------------------------------------------------
    void
    set_value(qreal value)
    // an asynchronous write (from the Qt/GUI main thread)
    // with an explicit latched value
    // note: it might be good to use qvariant instead, for lists
    {
        m_value = value;
    }


    // --------------------------------------------------------------------------------------------
    bool
    muted() const { return m_muted; }

    void
    set_muted(bool muted);
    // mute/unmute all output connections

    // --------------------------------------------------------------------------------------------
    qreal
    mul() const { return m_mul; }

    void
    set_mul(qreal mul);

    // --------------------------------------------------------------------------------------------
    qreal
    add() const { return m_add; }

    void
    set_add(qreal add);

    // --------------------------------------------------------------------------------------------
    nchannels_t
    nchannels() const noexcept { return m_nchannels; }
    // returns Port number of channels

    void
    set_nchannels(nchannels_t nchannels);
    // sets num_channels for Port and
    // allocate/reallocate its buffer

    // --------------------------------------------------------------------------------------------
    Polarity
    polarity() const noexcept { return m_polarity; }
    // returns Port polarity (INPUT/OUTPUT)

    // --------------------------------------------------------------------------------------------
    Type
    type() const noexcept { return m_type; }

    // --------------------------------------------------------------------------------------------
    QString
    name() const noexcept { return m_name; }

    // --------------------------------------------------------------------------------------------
    Node&
    parent_node() noexcept { return *m_parent; }

    // --------------------------------------------------------------------------------------------
    bool
    is_default() const noexcept { return m_default; }

    // --------------------------------------------------------------------------------------------
    std::vector<Connection*>&
    connections() noexcept { return m_connections; }
    // returns Port's current connections

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE bool
    connected() const noexcept { return m_connections.size(); }
    // returns true if Port is connected to anything

    Q_INVOKABLE bool
    connected(Port const& target) const noexcept;
    // returns true if this Port is connected to target

    Q_INVOKABLE bool
    connected(Node const& target) const noexcept;
    // returns true if this Port is connected to
    // one of the target's Port

    // --------------------------------------------------------------------------------------------
    QVariantList
    routing() const noexcept;

    void
    set_routing(QVariantList routing) noexcept;

private:

    // --------------------------------------------------------------------------------------------
    void
    add_connection(Connection* con);
    // note: we have to clearly separate the default connection from the other ones
    // the default connection is set from the parent-child structure within QML

    // --------------------------------------------------------------------------------------------
    void
    remove_connection(Connection& con) {
        m_connections.erase(std::remove(m_connections.begin(),
            m_connections.end(), &con), m_connections.end());
    }

    // --------------------------------------------------------------------------------------------
    QString
    m_name;

    // --------------------------------------------------------------------------------------------
    Polarity
    m_polarity;

    // --------------------------------------------------------------------------------------------
    Type
    m_type;

    // --------------------------------------------------------------------------------------------
    uint8_t
    m_index = 0;
    // note: is it really necessary to store the index here?

    // --------------------------------------------------------------------------------------------
    uint8_t
    m_nchannels = 0;

    // --------------------------------------------------------------------------------------------
    std::vector<Connection*>
    m_connections;

    // --------------------------------------------------------------------------------------------
    Node*
    m_parent = nullptr;

    // --------------------------------------------------------------------------------------------
    bool
    m_muted = false,
    m_default = false;

    // --------------------------------------------------------------------------------------------
    sample_t
    m_mul = 1,
    m_add = 0;

    // --------------------------------------------------------------------------------------------
    sample_t
    m_value;

    // --------------------------------------------------------------------------------------------
    Routing
    m_routing;
};

class External;

//=================================================================================================
class Graph : public QObject, public QQmlParserStatus
// embeds all connections between instantiated nodes
//=================================================================================================
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (External* external READ external)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (int vector READ vector WRITE set_vector)
    // Graph's signal vector size (lower is better, but will increase CPU usage)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal rate READ rate WRITE set_rate)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (QQmlListProperty<Node> subnodes READ subnodes)
    // this is the default list property

    // --------------------------------------------------------------------------------------------
    Q_CLASSINFO ("DefaultProperty", "subnodes")

    // --------------------------------------------------------------------------------------------
    Q_INTERFACES (QQmlParserStatus)

public:

    // --------------------------------------------------------------------------------------------
    struct properties
    // holds the graph processing properties
    // --------------------------------------------------------------------------------------------
    {
        sample_t
        rate = 44100;

        vector_t
        vector = 512;
    };

    // --------------------------------------------------------------------------------------------
    Graph();

    // --------------------------------------------------------------------------------------------
    static Graph&
    instance() { return *s_instance; }
    // returns the singleton instance of the Graph

    // --------------------------------------------------------------------------------------------
    virtual ~Graph() override {}
    // there might be something to be done later on

    // --------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}
    // unused interface override

    virtual void
    componentComplete() override;

    Q_SIGNAL void
    complete();

    // --------------------------------------------------------------------------------------------
    void
    debug(QString str)  { qDebug() << "[GRAPH]" << str; }

    // --------------------------------------------------------------------------------------------
    Connection&
    connect(Port& source, Port& dest, Routing matrix = {});
    // connects two Ports together
    // returns a reference to the newly created Connection object for further manipulation

    Connection&
    connect(Node& source, Node& dest, Routing matrix = {});
    // connects source Node's default outputs to dest Node's default inputs

    Connection&
    connect(Node& source, Port& dest, Routing matrix = {});
    // connects source Node's default outputs to dest Port

    Connection&
    connect(Port& source, Node& dest, Routing matrix = {});
    // connects Port source to dest Node's default inputs

    // --------------------------------------------------------------------------------------------
    void
    reconnect(Port& source, Port& dest, Routing matrix)
    // reconnects the two Ports with a new routing
    // --------------------------------------------------------------------------------------------
    {
        if (auto con = get_connection(source, dest))
            con->set_routing(matrix);
    }

    // --------------------------------------------------------------------------------------------
    void
    disconnect(Port& source, Port& dest)
    // disconnects the two Ports
    // --------------------------------------------------------------------------------------------
    {
        if (auto con = get_connection(source, dest)) {
            source.remove_connection(*con);
            dest.remove_connection(*con);

            m_connections.erase(std::remove(m_connections.begin(),
                m_connections.end(), *con), m_connections.end());
        }
    }

    // --------------------------------------------------------------------------------------------
    void
    register_node(Node& node) noexcept;
    // if this happens while graph is running, we have to set a Queued request
    // Graph will take a little amount of time to process it in between two runs

    // --------------------------------------------------------------------------------------------
    void
    unregister_node(Node& node) noexcept
    // if this happens while graph is running, we have to set a Queued request
    // Graph will take a little amount of time to process it in between two runs
    {
        m_nodes.erase(std::remove(m_nodes.begin(),
            m_nodes.end(), &node), m_nodes.end());
    }  

    // --------------------------------------------------------------------------------------------
    void
    add_connection(Connection& con) noexcept { m_connections.push_back(con); }
    // this is called when Connection has been explicitely
    // instantiated from a QML context

    // --------------------------------------------------------------------------------------------
    Connection*
    get_connection(Port& source, Port& dest)
    // returns the Connection object linking the two Ports
    // if Connection cannot be found, returns nullptr
    // --------------------------------------------------------------------------------------------
    {
        for (auto& con : m_connections)
            if (con.source() == &source &&
                con.dest() == &dest)
                return &con;

        return nullptr;
    }

    // --------------------------------------------------------------------------------------------    
    vector_t
    run() noexcept;
    // processes the graph from all of its direct subnodes

    vector_t
    run(Node& target) noexcept;
    // processes the graph from a specific node

    // --------------------------------------------------------------------------------------------
    uint16_t
    vector() noexcept { return m_properties.vector; }
    // returns graph vector size

    // --------------------------------------------------------------------------------------------
    void
    set_vector(uint16_t vector) { m_properties.vector = vector; }

    // --------------------------------------------------------------------------------------------
    Q_SIGNAL void
    vectorChanged(vector_t);

    // --------------------------------------------------------------------------------------------
    sample_t
    rate() noexcept { return m_properties.rate; }
    // returns graph sample rate

    // --------------------------------------------------------------------------------------------
    void
    set_rate(sample_t rate)
    // --------------------------------------------------------------------------------------------
    {        
        if (rate != m_properties.rate) {
            m_properties.rate = rate;
            emit s_instance->rateChanged(rate);
        }
    }

    // --------------------------------------------------------------------------------------------
    External*
    external() { return m_external; }

    // --------------------------------------------------------------------------------------------
    Q_SIGNAL void
    rateChanged(sample_t);

    // --------------------------------------------------------------------------------------------
    QQmlListProperty<Node>
    subnodes()
    {
        return QQmlListProperty<Node>(
               this, this,
               &Graph::append_subnode,
               &Graph::nsubnodes,
               &Graph::subnode,
               &Graph::clear_subnodes);
    }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    append_subnode(Node* subnode) { m_subnodes.push_back(subnode); }
    // pushes back subnode to the Graph's direct children
    // making the necessary implicit connections

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE int
    nsubnodes() const { return m_subnodes.count(); }
    // returns the number of direct children Nodes that the Graph holds

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE Node*
    subnode(int index) const { return m_subnodes[index]; }
    // retrieve children Node at index

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    clear_subnodes() { m_subnodes.clear(); }
    // clear Graph's direct Node children

    // --------------------------------------------------------------------------------------------
    static void
    append_subnode(QQmlListProperty<Node>* list, Node* subnode)
    // static version, see above
    {
        reinterpret_cast<Graph*>(list->data)->append_subnode(subnode);
    }

    // --------------------------------------------------------------------------------------------
    static int
    nsubnodes(QQmlListProperty<Node>* list)
    // static version, see above
    {
        return reinterpret_cast<Graph*>(list->data)->nsubnodes();
    }

    // --------------------------------------------------------------------------------------------
    static Node*
    subnode(QQmlListProperty<Node>* list, int index)
    // static version, see above
    {
        return reinterpret_cast<Graph*>(list->data)->subnode(index);
    }

    // --------------------------------------------------------------------------------------------
    static void
    clear_subnodes(QQmlListProperty<Node>* list)
    // static version, see above
    {
        reinterpret_cast<Graph*>(list->data)->clear_subnodes();
    }

private:

    // --------------------------------------------------------------------------------------------
    static Graph*
    s_instance;

    wpn114::audio::graph
    m_xgraph;

    // --------------------------------------------------------------------------------------------
    std::vector<Connection>
    m_connections;

    // --------------------------------------------------------------------------------------------
    std::vector<Node*>
    m_nodes;

    // --------------------------------------------------------------------------------------------
    Graph::properties
    m_properties;

    // --------------------------------------------------------------------------------------------
    QVector<Node*>
    m_subnodes;

    // --------------------------------------------------------------------------------------------
    External*
    m_external = nullptr;
};

// --------------------------------------------------------------------------------------------
class Spatial;

//=================================================================================================
class Node : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
/*! \class Node
 * \brief the main Node class */
//=================================================================================================
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE set_name)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(bool muted MEMBER m_muted WRITE set_muted)
    /*!
     * \property Node::muted
     * \brief mutes/unmutes all Node's outputs
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(bool active MEMBER m_active WRITE set_active)
    /*!
     * \property Node::active
     * \brief activates/deactivates Node processing
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Spatial* spatial READ spatial)
    /*!
     * \property Node::space
     * \brief unimplemented yet
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(QQmlListProperty<Node> subnodes READ subnodes)
    /*!
     * \property Node::subnodes
     * \brief the list of subnodes that are connected to this Node
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Dispatch::Values dispatch MEMBER m_dispatch)
    /*!
     * \property Node::dispatch
     * \brief sets dispatch mode for subnode connections
    */

    // --------------------------------------------------------------------------------------------
    Q_CLASSINFO
    ("DefaultProperty", "subnodes" )

    // --------------------------------------------------------------------------------------------
    Q_INTERFACES
    (QQmlParserStatus QQmlPropertyValueSource)

    // --------------------------------------------------------------------------------------------
    friend class Connection;
    friend class Graph;
    // Connection & Graph have to call the process function
    // but it not should be available for Nodes inheriting from this class

public:

    // --------------------------------------------------------------------------------------------
    Node() {}

    // --------------------------------------------------------------------------------------------
    virtual
    ~Node() override;

    // --------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}
    // unused interface override

    // --------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    // parses the local graph and builds appropriate connections between this Node
    // and its subnodes, given the selected dispatch mode
    {
        Graph::instance().register_node(*this);

        if (m_subnodes.empty())
            return;

        for (auto& subnode : m_subnodes)
             subnode->set_parent(this);       

        switch(m_dispatch)
        {
        case Dispatch::Values::UpwardsMerge:
        {
            // subnode's chain out connects to this Node
            for (auto& subnode : m_subnodes) {
                auto& source = subnode->chainout();                
                if (auto port = source.default_port(Polarity::Output))
                    Graph::instance().connect(source, *this, port->routing());
            }
            break;
        }
        case Dispatch::Values::DownwardsChain:
        {
            // connect this Node default outputs to first subnode
            auto& front = *m_subnodes.front();
            Graph::instance().connect(*this, front);

            if (m_subnodes.count() == 1)
                return;

            // chain the following subnodes, until last is reached
            for (int n = 0; n < m_subnodes.count(); ++n) {
                auto& source = m_subnodes[n]->chainout();
                auto& dest = *m_subnodes[n+1];
                Graph::instance().connect(source, dest);
            }
        }
        }
    }

    // --------------------------------------------------------------------------------------------
    virtual void
    setTarget(const QQmlProperty& target) override
    // a convenience method for quickly adding a processor Node in between
    // target Port and the actual output (QML)
    {
        auto port = target.read().value<Port*>();

        switch (port->polarity()) {
            case Polarity::Input: Graph::instance().connect(*this, *port); break;
            case Polarity::Output: Graph::instance().connect(*port, *this);
        }
    }

    // --------------------------------------------------------------------------------------------
    Q_SLOT virtual void
    on_graph_complete(Graph::properties const& properties)
    {

    }

    // --------------------------------------------------------------------------------------------

    virtual void*
    xbuild() const { return nullptr; }

    virtual int_fn_t
    init_fn() const { return nullptr; }

    virtual prc_fn_t
    proc_fn() const { return nullptr; }

    // --------------------------------------------------------------------------------------------
    QString
    name() const { return m_name; }

    void
    set_name(QString name) { m_name = name; }

    // --------------------------------------------------------------------------------------------
    Spatial* spatial();

    // --------------------------------------------------------------------------------------------
    void
    set_muted(bool muted) noexcept
    // mute/unmute all output Ports
    // --------------------------------------------------------------------------------------------
    {
        for (auto& port: m_output_ports)
             port->set_muted(muted);
    }

    // --------------------------------------------------------------------------------------------
    void
    set_active(bool active) noexcept
    // activate/deactivate all output Port connections
    // --------------------------------------------------------------------------------------------
    {
        for (auto& port: m_output_ports)
            for (auto& connection : port->connections())
                connection->set_active(active);
    }

    // --------------------------------------------------------------------------------------------
    void
    set_parent(Node* parent) { m_parent = parent; }
    // sets Node's parent Node, creating appropriate connections

    // --------------------------------------------------------------------------------------------
    void
    register_port(Port& s)
    // this is called from Port's constructor
    // adds a pointer to the newly created Port
    {
        ports(s.polarity()).push_back(&s);
    }

    // --------------------------------------------------------------------------------------------
    std::vector<Port*>&
    ports(Polarity polarity) noexcept
    // returns Node's Port vector matching given polarity
    // --------------------------------------------------------------------------------------------
    {
        switch(polarity) {
            case Polarity::Input: return m_input_ports;
            case Polarity::Output: return m_output_ports;
        }

        assert(false);
    }

    // --------------------------------------------------------------------------------------------
    Port*
    default_port(Polarity polarity) noexcept
    // --------------------------------------------------------------------------------------------
    {
        if  (Port* default_audio_port = default_port(Port::Audio, polarity))
             return default_audio_port;
        else return default_port(Port::Midi_1_0, polarity);
    }

    // --------------------------------------------------------------------------------------------
    Port*
    default_port(Port::Type type, Polarity polarity) noexcept
    // --------------------------------------------------------------------------------------------
    {
        for (auto& port : ports(polarity))
            if (port->type() == type && port->is_default())
                return port;
        return nullptr;
    }

    // --------------------------------------------------------------------------------------------
    bool
    connected() noexcept { return (connected(Polarity::Input) || connected(Polarity::Output)); }
    // returns true if Node is connected to anything

    // --------------------------------------------------------------------------------------------
    bool
    connected(Polarity polarity) noexcept
    // returns true if Node is connected to anything (given polarity)
    // --------------------------------------------------------------------------------------------
    {
        for (auto& port : ports(polarity))
            if (port->connected())
                return true;
        return false;
    }

    // --------------------------------------------------------------------------------------------
    template<typename T> bool
    connected(T const& target) noexcept
    // redirect to input/output Ports
    // --------------------------------------------------------------------------------------------
    {
        for (auto& port: ports(Polarity::Input))
            if (port->connected(target))
                return true;

        for (auto& port: ports(Polarity::Output))
            if (port->connected(target))
                return true;
        return false;
    }

    // --------------------------------------------------------------------------------------------
    virtual Node&
    chainout() noexcept
    // returns the last Node in the parent-children chain
    // this would be the Node producing the chain's final output
    // --------------------------------------------------------------------------------------------
    {
        switch(m_dispatch) {
        case Dispatch::Values::UpwardsMerge:
            return *this;
        case Dispatch::Values::DownwardsChain:
            if (m_subnodes.empty())
                return *this;
            else return *m_subnodes.back();
        }
        assert(false);
    }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE qreal
    db(qreal v) { return std::pow(10, v*.05); }
    // this is not an a->db function
    // it is a useful, readable and declarative way to signal that the value
    // used within qml is of the db unit.
    // consequently, it returns the value as a normal linear amplitude value.
    // in general, to ease-up calculations, the db should be calculated once and upfront

    // --------------------------------------------------------------------------------------------
    QQmlListProperty<Node>
    subnodes()
    // returns subnodes (QML format)
    // --------------------------------------------------------------------------------------------
    {
        return QQmlListProperty<Node>(
               this, this,
               &Node::append_subnode,
               &Node::nsubnodes,
               &Node::subnode,
               &Node::clear_subnodes);
    }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    append_subnode(Node* node) { m_subnodes.push_back(node); }
    // appends a subnode to this Node children

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE int
    nsubnodes() const { return m_subnodes.count(); }
    // returns this Node' subnodes count

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE Node*
    subnode(int index) const { return m_subnodes[index]; }
    // returns this Node' subnode at index

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    clear_subnodes() { m_subnodes.clear(); }

    // --------------------------------------------------------------------------------------------
    static void
    append_subnode(QQmlListProperty<Node>* list, Node* node)
    // static Qml version, see above
    {
        reinterpret_cast<Node*>(list->data)->append_subnode(node);
    }

    // --------------------------------------------------------------------------------------------
    static int
    nsubnodes(QQmlListProperty<Node>* list)
    // static Qml version, see above
    {
        return reinterpret_cast<Node*>(list)->nsubnodes();
    }

    // --------------------------------------------------------------------------------------------
    static Node*
    subnode(QQmlListProperty<Node>* list, int index)
    // static Qml version, see above
    {
        return reinterpret_cast<Node*>(list)->subnode(index);
    }

    // --------------------------------------------------------------------------------------------
    static void
    clear_subnodes(QQmlListProperty<Node>* list)
    // static Qml version, see above
    {
        reinterpret_cast<Node*>(list)->clear_subnodes();
    }

protected:

    wpn114::audio::node*
    m_xnode = nullptr;

    // --------------------------------------------------------------------------------------------
    Spatial*
    m_spatial;

    // --------------------------------------------------------------------------------------------
    std::vector<Port*>
    m_input_ports,
    m_output_ports;

    // --------------------------------------------------------------------------------------------
    QVector<Node*>
    m_subnodes;

    // --------------------------------------------------------------------------------------------
    bool
    m_muted      = false,
    m_active     = true,
    m_processed  = false;

    // --------------------------------------------------------------------------------------------
    Dispatch::Values
    m_dispatch = Dispatch::Values::UpwardsMerge;

    // --------------------------------------------------------------------------------------------
    Node*
    m_parent = nullptr;

    // --------------------------------------------------------------------------------------------
    QString
    m_name = "UnnamedNode";
};
}
}
}

Q_DECLARE_METATYPE(wpn114::audio::qt::Routing)
Q_DECLARE_METATYPE(wpn114::audio::qt::Port)
Q_DECLARE_METATYPE(wpn114::audio::qt::Connection)
