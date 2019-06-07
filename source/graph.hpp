#ifndef QTWRAPPER2_HPP
#define QTWRAPPER2_HPP

#include <QObject>
#include <QQmlParserStatus>
#include <QQmlProperty>
#include <QQmlPropertyValueSource>
#include <QQmlListProperty>
#include <QVector>
#include <QVariant>
#include <cmath>

#include <QtDebug>

// --------------------------------------------------------------------------------------------------
// CONVENIENCE MACRO DEFINITIONS
// --------------------------------------------------------------------------------------------------

#define WPN_OBJECT                                                                                  \
Q_OBJECT

#define WPN_SOCKET(_tp, _pol, _nm, _idx, _nchn)                                                     \
private: Q_PROPERTY(Variant _nm READ get_##_nm WRITE set_##_nm NOTIFY _nm##Changed)                 \
protected: Socket m_##_nm { this, #_nm, _tp, _pol, _idx, _nchn };                                   \
public:                                                                                             \
Variant get_##_nm() { return &m_##_nm; }                                                            \
void set_##_nm(Variant v) { m_##_nm.assign(v); }                                                    \
Q_SIGNAL void _nm##Changed();

#define WPN_ENUM_INPUTS(...)                                                                        \
enum Inputs {__VA_ARGS__, N_IN};

#define WPN_ENUM_OUTPUTS(...)                                                                       \
enum Outputs {__VA_ARGS__, N_OUT};

#define WPN_INPUT_DECLARE(_nm, _tp, _nchn)                                                          \
WPN_SOCKET(_tp, INPUT, _nm, _nm, _nchn)

#define WPN_OUTPUT_DECLARE(_nm, _tp, _nchn)                                                         \
WPN_SOCKET(_tp, OUTPUT, _nm, _nm, _nchn)

#define WPN_TODO
#define WPN_EXAMINE
#define WPN_OK
#define WPN_INCORRECT
#define WPN_CLEANUP
#define WPN_INCOMPLETE
#define WPN_UNIMPLEMENTED

//-------------------------------------------------------------------------------------------------

enum  polarity_t    { OUTPUT = 0, INPUT = 1 };
using sample_t      = qreal;
using vector_t      = uint16_t;
using pool          = sample_t***;
using nchannels_t   = uint8_t;

class Connection;
class Node;

//-------------------------------------------------------------------------------------------------
namespace wpn114
{

//-------------------------------------------------------------------------------------------------
sample_t**
allocate_buffer(nchannels_t nchannels, vector_t nframes);
// we make no assumption as to how many channels each input/output may have
// by the time the graph is ready/updated, so we can't really template it upfront
// we also want to allocate the whole thing in a single block

//-------------------------------------------------------------------------------------------------
void
reset_buffer(sample_t** buffer, nchannels_t nchannels, vector_t nframes);
// sets buffer to zero

//-------------------------------------------------------------------------------------------------
void
fill_buffer(sample_t** buffer, nchannels_t nchannels, vector_t nframes, sample_t value);
// fill buffer with a single sample_t value

//-------------------------------------------------------------------------------------------------
WPN_UNIMPLEMENTED sample_t
midicps(uint8_t mnote);
// midi pitch value to frequency (hertz)

//-------------------------------------------------------------------------------------------------
WPN_UNIMPLEMENTED uint8_t
cpsmidi(sample_t f);
// frequency (hertz) to midi pitch value


} // end namespace wpn114

//=================================================================================================
class Routing : public QObject
// connection matrix between 2 sockets
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
    // --------------------------------------------------------------------------------------------
    {
        if (list.empty())
            return;
        for (int n = 0; n < list.count(); n += 2) {
            m_routing[n][0] = list[n].toInt();
            m_routing[n][1] = list[n+1].toInt();
        }
    }

    // --------------------------------------------------------------------------------------------
    Routing&
    operator=(Routing const& cp) { m_routing = cp.m_routing; return *this; }

    // --------------------------------------------------------------------------------------------
    cable&
    operator[](uint8_t index) { return m_routing[index]; }
    // returns routing matrix 'cable' at index

    // --------------------------------------------------------------------------------------------
    nchannels_t
    ncables() const { return m_routing.size(); }

    // --------------------------------------------------------------------------------------------
    bool
    null() const { return m_routing.empty(); }
    // returns true if Routing is not explicitely defined

    // --------------------------------------------------------------------------------------------
    QVariantList
    to_qvariant() const
    // returns Routing as QVariantList, for use within QML
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

using byte_t = uint8_t;

//=================================================================================================
WPN_INCOMPLETE
struct midi_t
// TODO: properly
//=================================================================================================
{
    // we pad the last 3/4 bytes with zeroes
    // except if we're processing floats
    byte_t status = 0;
    byte_t b1 = 0;
    byte_t b2 = 0;

    midi_t(sample_t sample)
    {
        memcpy(this, &sample, 3);
    }

    operator
    sample_t() const
    {
        sample_t sample = 0;
        memcpy(&sample, this, 3);
        return sample;
    }
};

class Socket;

//=================================================================================================
class Connection : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//=================================================================================================
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Socket* source MEMBER m_source)
    // Connection source Socket (output polarity) from which the signal will transit

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Socket* dest MEMBER m_dest)
    // Connection dest Socket (input polarity) to which the signal will transit

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (QVariantList routing READ routing WRITE set_routing)
    // Connection's channel-to-channel routing matrix
    // materialized as a QVariantList within QML
    // e.g. [0, 1, 1, 0] will connect output 0 to input 1 and output 1 to input 0
    // TODO: [[0,1],[1,0]] format as well

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal mul MEMBER m_mul)
    // Connection's amplitude level (linear)
    // scales the amplitude of the input signal to transit to the output
    // this property might be overriden or scaled by Socket's or Node's level property
    // TODO: prefader/postfader property

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal add MEMBER m_add)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (bool muted MEMBER m_muted)
    // Connection will still process when muted
    // but will output zeros instead of the normal signal

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (bool active MEMBER m_active)
    // if inactive, Connection won't process the upstream part of the Graph
    // to which it is connected

    // --------------------------------------------------------------------------------------------
    Q_INTERFACES (QQmlParserStatus QQmlPropertyValueSource)

    // --------------------------------------------------------------------------------------------
    friend class Socket;
    friend class Node;

public:

    // --------------------------------------------------------------------------------------------
    Connection() {}

    // --------------------------------------------------------------------------------------------
    Connection(Connection const& cp) :
        m_source(cp.m_source)
      , m_dest(cp.m_dest)
      , m_routing(cp.m_routing)
      , m_active(cp.m_active)
      , m_muted(cp.m_muted)
      , m_mul(cp.m_mul)
      , m_add(cp.m_add) {}
    // copy constructor, don't really know in which case it should/will be used

    // --------------------------------------------------------------------------------------------
    Connection(Socket& source, Socket& dest, Routing matrix) :
        m_source(&source)
      , m_dest(&dest)
      , m_routing(matrix) {}
    // constructor accessed from C++

    // --------------------------------------------------------------------------------------------
    Connection&
    operator=(Connection const& cp)
    // --------------------------------------------------------------------------------------------
    {
        m_source    = cp.m_source;
        m_dest      = cp.m_dest;
        m_routing   = cp.m_routing;
        m_active    = cp.m_active;
        m_muted     = cp.m_muted;
        m_mul       = cp.m_mul;
        m_add       = cp.m_add;

        return *this;
    }

    // --------------------------------------------------------------------------------------------
    bool
    operator==(Connection const& other)
    // --------------------------------------------------------------------------------------------
    {
        return m_source == other.m_source && m_dest == other.m_dest;
    }

    // --------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}
    // unused interface override

    // --------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override;
    // allocates/reallocates Connection buffer
    // allocates/reallocates source/dest buffers as well
    // depending on both sockets numchannels properties

    // --------------------------------------------------------------------------------------------
    virtual void
    setTarget(const QQmlProperty& target) override;
    // qml property binding, e. g.:
    // Connection on 'socket' { level: db(-12); routing: [0, 1] }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    activate() noexcept { m_active = true; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    deactivate() noexcept { m_active = false; }

    // --------------------------------------------------------------------------------------------
    void
    set_active(bool active) noexcept { m_active = active; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    mute() noexcept { m_muted = true; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    unmute() noexcept { m_muted = false; }

    // --------------------------------------------------------------------------------------------
    void
    set_muted(bool muted) noexcept { m_muted = muted; }

    // --------------------------------------------------------------------------------------------
    bool
    active() const noexcept { return m_active; }

    // --------------------------------------------------------------------------------------------
    bool
    muted() const noexcept { return m_muted; }

    // --------------------------------------------------------------------------------------------
    Socket*
    source() { return m_source; }
    // returns Connection's source Socket (output polarity)

    // --------------------------------------------------------------------------------------------
    Socket*
    dest() { return m_dest; }
    // returns Connection's dest Socket (input polarity)

    // --------------------------------------------------------------------------------------------
    QVariantList
    routing() const noexcept { return m_routing.to_qvariant(); }
    // returns Connection's routing matrix as QML formatted list

    // --------------------------------------------------------------------------------------------
    void
    set_routing(QVariantList list) { m_routing = Routing(list); }
    // sets routing matrix from QML

    // --------------------------------------------------------------------------------------------
    void
    set_routing(Routing matrix) { m_routing = matrix; }
    // sets routing matrix from C++

    // --------------------------------------------------------------------------------------------
    void
    set_feedback(bool feedback) { m_feedback = feedback; }

    // --------------------------------------------------------------------------------------------
    bool
    feedback() const noexcept { return m_feedback; }

    // --------------------------------------------------------------------------------------------
    sample_t
    mul() const { return m_mul; }

    // --------------------------------------------------------------------------------------------
    sample_t
    add() const { return m_add; }

    // --------------------------------------------------------------------------------------------
    void
    set_mul(sample_t mul) { m_mul = mul; }

    // --------------------------------------------------------------------------------------------
    void
    set_add(sample_t add) { m_add = add; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE qreal
    db(qreal v) { return std::pow(10, v*.05); }
    // this is not an a->db function
    // it is a useful, readable and declarative way to signal that the value
    // used within qml is of the db unit.
    // consequently, it returns the value as a normal linear amplitude value.
    // in general, to ease-up calculations, the db should be calculated once and upfront

private:

    // --------------------------------------------------------------------------------------------
    void
    pull(vector_t nframes) noexcept;
    // the main processing function

    // --------------------------------------------------------------------------------------------
    nchannels_t
    m_nchannels = 0;

    // --------------------------------------------------------------------------------------------
    Socket*
    m_source = nullptr;

    Socket*
    m_dest = nullptr;

    // --------------------------------------------------------------------------------------------
    Routing
    m_routing = {};

    // --------------------------------------------------------------------------------------------
    bool
    m_muted = false;
    // if Connection is muted, it will still process its inputs
    // but will output 0 continuously

    // --------------------------------------------------------------------------------------------
    bool
    m_active = true;
    // if Connection is inactive, it won't process its inputs
    // and consequently call the upstream graph

    // --------------------------------------------------------------------------------------------
    bool
    m_feedback = false;
    // if this is a feedback Connection, it will recursively
    // fetch the previous input buffer without calling input Node's process function again

    // --------------------------------------------------------------------------------------------
    qreal
    m_mul = 1,
    m_add = 0;
};

Q_DECLARE_METATYPE(Connection)

// --------------------------------------------------------------------------------------------
class Variant : public QObject
// this one is for allowing to assign various types of values to a specific Socket object
// (QVariant doesn't allow us to read Socket properties)
// --------------------------------------------------------------------------------------------
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (bool muted READ muted WRITE set_muted)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal mul READ mul WRITE set_mul)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal add READ add WRITE set_add)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (int nchannels READ nchannels)
    // NCHANNELS property: for multichannel expansion
    // and dynamic channel setting/allocation

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (Routing routing READ routing WRITE set_routing)

    // --------------------------------------------------------------------------------------------
    union value
    // --------------------------------------------------------------------------------------------
    {
        value();
        ~value();

        Socket* socket;
        Connection connection;
        qreal real;
        QVariant variant;
        QVariantList list;
    }   m_value;

    // --------------------------------------------------------------------------------------------
    enum class type { Socket, Connection, Real, QVariant, List}
    m_type;

public:

    Variant() {}

    Variant(Variant const& cp)
    {

    }

    Variant(Socket* s) {
        m_value.socket = s;
        m_type = type::Socket;
    }

    Variant(Connection c) {
        m_value.connection = c;
        m_type = type::Connection;
    }

    Variant(qreal v) {
        m_value.real = v;
        m_type = type::Real;
    }

    Variant(QVariant v) {
        m_value.variant = v;
        m_type = type::QVariant;
    }

    Variant(QVariantList l) {
        m_value.list = l;
        m_type = type::List;
    }

    ~Variant() {}

    Variant& operator=(Variant const& cp)
    {
        return *this;
    }

    Variant& operator=(Socket* s)
    {
        return *this;
    }

    operator Socket*()
    {
        return m_value.socket;
    }

    // --------------------------------------------------------------------------------------------
    template<typename T> bool
    canConvert();

    // --------------------------------------------------------------------------------------------
    template<typename T> T
    value();

    // --------------------------------------------------------------------------------------------
    qreal
    mul() const;

    void
    set_mul(qreal mul);

    // --------------------------------------------------------------------------------------------
    qreal
    add() const;

    void
    set_add(qreal add);

    // --------------------------------------------------------------------------------------------
    bool
    muted() const;

    void
    set_muted(bool muted);

    // --------------------------------------------------------------------------------------------
    nchannels_t
    nchannels() const;

    // --------------------------------------------------------------------------------------------
    Routing
    routing() const;

    void
    set_routing(Routing r);
};

Q_DECLARE_METATYPE(Variant)

//=================================================================================================
class Socket : public QObject
// represents a node single input/output
// holding specific types of values
// and with nchannels_t channels
//=================================================================================================
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (bool muted MEMBER m_muted WRITE set_muted)
    // MUTED property: manages and overrides all Socket connections mute property

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal mul MEMBER m_mul WRITE set_mul)


    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal add MEMBER m_add WRITE set_add)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (int nchannels MEMBER m_nchannels WRITE set_nchannels)
    // NCHANNELS property: for multichannel expansion
    // and dynamic channel setting/allocation

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (Routing routing READ routing WRITE set_routing)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (Type type READ type)

    // --------------------------------------------------------------------------------------------
    friend class Connection;
    friend class Graph;
    friend class Node;

public:

    //---------------------------------------------------------------------------------------------
    enum Type
    //---------------------------------------------------------------------------------------------
    {
        // shows what values a specific socket is expecting to receive
        // or is explicitely outputing
        // a connection between a Midi Socket and any other socket
        // that isn't Midi will be refused
        // otherwise, if connection types mismatch, a warning will be emitted

        Audio,
        // -1.0 to 1.0 normalized audio signal value

        Midi_1_0,
        // status byte + 2 other index/value bytes

        Integer,
        // a control integer value

        FloatingPoint,
        // a control floating point value

        Cv,
        // -5.0 to 5.0 control voltage values

        Gate,
        // a 0-1 latched value

        Trigger
        // a single pulse (1.f)
    };

    Q_ENUM (Type)

    // --------------------------------------------------------------------------------------------
    Socket() {}

    // --------------------------------------------------------------------------------------------
    Socket(Node* parent, QString name, Type type, polarity_t polarity, uint8_t index, uint8_t nchannels);

    // --------------------------------------------------------------------------------------------
    Socket(Socket const& cp) :
        // this is only called because of the macro Socket definitions
        // it shouldn't be called otherwise in any other context
        m_polarity  (cp.m_polarity)
      , m_index     (cp.m_index)
      , m_nchannels (cp.m_nchannels)
      , m_type      (cp.m_type)
      , m_name      (cp.m_name)
      , m_parent    (cp.m_parent) {}

    // --------------------------------------------------------------------------------------------
    Socket&
    operator=(Socket const& cp)
    // same as for the copy constructor
    // --------------------------------------------------------------------------------------------
    {
        m_polarity   = cp.m_polarity;
        m_index      = cp.m_index;
        m_nchannels  = cp.m_nchannels;
        m_parent     = cp.m_parent;
        m_type       = cp.m_type;
        m_name       = cp.m_name;
    }

    // --------------------------------------------------------------------------------------------
    ~Socket() { free(m_buffer); }

    // --------------------------------------------------------------------------------------------       
    Socket& operator=(Variant v)
    {
        return *this;
    }

    void
    assign(Variant variant);
    // this is called from QmlEngine when we assign a specific socket a value:
    // - a qreal value (will fill the current buffer with this)
    // - another Socket (will establish a connection with this Socket)
    // - an explicit Connection object
    // - a QVariantList, for which each element will in turn be parsed recursively

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
    // each connection involving this socket will multiply the signal by this 'mul' factor

    // --------------------------------------------------------------------------------------------
    qreal
    add() const { return m_add; }

    void
    set_add(qreal add);
    // each connection involving this socket will offset the signal by 'add'

    // --------------------------------------------------------------------------------------------
    void
    set_nchannels(nchannels_t nchannels);
    // sets num_channels for socket and
    // allocate/reallocate its buffer

    // --------------------------------------------------------------------------------------------
    nchannels_t
    nchannels() const { return m_nchannels; }
    // returns Socket number of channels

    // --------------------------------------------------------------------------------------------
    polarity_t
    polarity() const { return m_polarity; }
    // returns Socket polarity (INPUT/OUTPUT)

    // --------------------------------------------------------------------------------------------
    Type
    type() const { return m_type; }

    // --------------------------------------------------------------------------------------------
    QString
    name() const { return m_name; }

    // --------------------------------------------------------------------------------------------
    Node&
    parent_node() { return *m_parent; }
    // returns Socket's parent Node

    // --------------------------------------------------------------------------------------------
    sample_t**
    buffer() { return m_buffer; }

    // --------------------------------------------------------------------------------------------
    std::vector<Connection*>&
    connections() { return m_connections; }
    // returns Socket's current connections

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE bool
    connected() const { return m_connections.size(); }
    // returns true if Socket is connected to anything

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE bool
    connected(Socket const& target) const;
    // returns true if this Socket is connected to target

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE bool
    connected(Node const& target) const;
    // returns true if this Socket is connected to
    // one of the target's Socket

    // --------------------------------------------------------------------------------------------
    Routing
    routing() const;

    // --------------------------------------------------------------------------------------------
    void
    set_routing(Routing routing);

private:

    // --------------------------------------------------------------------------------------------
    void
    allocate(vector_t nframes)
    // --------------------------------------------------------------------------------------------
    {
        if (m_buffer)
            free(m_buffer);

        m_buffer = wpn114::allocate_buffer(m_nchannels, nframes);
    }

    // --------------------------------------------------------------------------------------------
    void
    reset(vector_t nframes)
    {
        wpn114::reset_buffer(m_buffer, m_nchannels, nframes);
    }

    // --------------------------------------------------------------------------------------------
    void
    add_connection(Connection& con)
    {
        m_connections.push_back(&con);
    }

    // --------------------------------------------------------------------------------------------
    void
    remove_connection(Connection& con)
    {
        m_connections.erase(std::remove(m_connections.begin(),
            m_connections.end(), &con), m_connections.end());
    }

    // --------------------------------------------------------------------------------------------
    QString
    m_name;

    // --------------------------------------------------------------------------------------------
    polarity_t
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
    sample_t**
    m_buffer = nullptr;

    // --------------------------------------------------------------------------------------------
    bool
    m_muted = false;

    // --------------------------------------------------------------------------------------------
    qreal
    m_mul = 1,
    m_add = 0;
};

Q_DECLARE_METATYPE(Socket)

//=================================================================================================
class Graph : public QObject, public QQmlParserStatus
// embeds all connections between instantiated nodes
//=================================================================================================
{
    Q_OBJECT

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
        sample_t rate    = 44100;
        vector_t vector  = 512;
    };

    // --------------------------------------------------------------------------------------------
    Graph() { s_instance = this; }

    // --------------------------------------------------------------------------------------------
    virtual ~Graph() override
    // there might be something to be done later on
    // --------------------------------------------------------------------------------------------
    {

    }

    // --------------------------------------------------------------------------------------------
    static Graph&
    instance() { return *s_instance; }
    // returns the singleton instance of the Graph

    // --------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}
    // unused interface override

    // --------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override;

    // --------------------------------------------------------------------------------------------
    static Connection&
    connect(Socket& source, Socket& dest, Routing matrix = {});
    // connects two Sockets together
    // returns a reference to the newly created Connection object for further manipulation

    // --------------------------------------------------------------------------------------------
    static Connection&
    connect(Node& source, Node& dest, Routing matrix = {});
    // connects source Node's default outputs to dest Node's default inputs

    // --------------------------------------------------------------------------------------------
    static Connection&
    connect(Node& source, Socket& dest, Routing matrix = {});
    // connects source Node's default outputs to dest Socket

    // --------------------------------------------------------------------------------------------
    static Connection&
    connect(Socket& source, Node& dest, Routing matrix = {});
    // connects Socket source to dest Node's default inputs

    // --------------------------------------------------------------------------------------------
    static void
    register_node(Node& node) noexcept;

    // --------------------------------------------------------------------------------------------
    static void
    unregister_node(Node& node) noexcept
    {
        s_nodes.erase(std::remove(s_nodes.begin(),
            s_nodes.end(), &node), s_nodes.end());
    }

    // --------------------------------------------------------------------------------------------
    WPN_EXAMINE static void
    add_connection(Connection& con) noexcept
    {
        s_connections.push_back(con);
    }
    // this is called when Connection has been explicitely
    // instantiated from a QML context

    // --------------------------------------------------------------------------------------------
    static Connection*
    get_connection(Socket& source, Socket& dest)
    // returns the Connection object linking the two Sockets
    // if Connection cannot be found, returns nullptr
    // --------------------------------------------------------------------------------------------
    {
        for (auto& con : s_connections)
            if (con.source() == &source &&
                con.dest() == &dest)
                return &con;

        return nullptr;
    }

    // --------------------------------------------------------------------------------------------
    static void
    reconnect(Socket& source, Socket& dest, Routing matrix)
    // reconnects the two Sockets with a new routing
    // --------------------------------------------------------------------------------------------
    {
        if (auto con = get_connection(source, dest))
            con->set_routing(matrix);
    }

    // --------------------------------------------------------------------------------------------
    WPN_TODO static void
    disconnect(Socket& source, Socket& dest)
    // disconnects the two Sockets
    // --------------------------------------------------------------------------------------------
    {
        if (auto con = get_connection(source, dest)) {
            source.remove_connection(*con);
            dest.remove_connection(*con);

            s_connections.erase(std::remove(s_connections.begin(),
                s_connections.end(), *con), s_connections.end());
        }
    }

    // --------------------------------------------------------------------------------------------
    static pool&
    run(Node& target) noexcept;
    // the main processing function

    // --------------------------------------------------------------------------------------------
    static uint16_t
    vector() noexcept { return s_properties.vector; }
    // returns graph vector size

    // --------------------------------------------------------------------------------------------
    static void
    set_vector(uint16_t vector) { s_properties.vector = vector; }

    // --------------------------------------------------------------------------------------------
    Q_SIGNAL void
    rateChanged(sample_t);

    // --------------------------------------------------------------------------------------------
    Q_SIGNAL void
    vectorChanged(vector_t);

    // --------------------------------------------------------------------------------------------
    static sample_t
    rate() noexcept { return s_properties.rate; }
    // returns graph sample rate

    // --------------------------------------------------------------------------------------------
    static void
    set_rate(sample_t rate)
    // --------------------------------------------------------------------------------------------
    {
        if (rate != s_properties.rate) {
            s_properties.rate = rate;
            emit s_instance->rateChanged(rate);
        }
    }

    // --------------------------------------------------------------------------------------------
    QQmlListProperty<Node>
    subnodes()
    // --------------------------------------------------------------------------------------------
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

    // --------------------------------------------------------------------------------------------
    static std::vector<Connection>
    s_connections;

    // --------------------------------------------------------------------------------------------
    static std::vector<Node*>
    s_nodes;

    // --------------------------------------------------------------------------------------------
    static Graph::properties
    s_properties;

    // --------------------------------------------------------------------------------------------
    static QVector<Node*>
    m_subnodes;
};

//=================================================================================================
struct spatial_t
// TODO
//=================================================================================================
{
    qreal x = 0, y = 0, z = 0;
    qreal w = 0, d = 0, h = 0;
};

//=================================================================================================
class Spatial : public QObject
// TODO
//=================================================================================================
{
    Q_OBJECT

public:

    Spatial()
    {

    }

    Spatial(Spatial const&)
    {

    }

    Spatial& operator=(Spatial const&)
    {

    }

    bool operator!=(Spatial const&)
    {

    }

private:
    std::vector<spatial_t>
    m_channels;
};

Q_DECLARE_METATYPE(Spatial)

//=================================================================================================
class Dispatch : public QObject
//=================================================================================================
{
    Q_OBJECT

    public:    
    enum Values { Upwards = 0, Downwards = 1 }; Q_ENUM (Values)
};

//=================================================================================================
class Node : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
/*! \class Node
 * \brief the main Node class */
//=================================================================================================
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Routing routing READ routing WRITE set_routing)

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
    Q_PROPERTY(qreal mul MEMBER m_mul WRITE set_mul)
    /*!
     * \property Node::mul
     * \brief sets Node's default output levels (linear amplitude)
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(qreal add MEMBER m_add WRITE set_add)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Spatial space MEMBER m_spatial)
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
    virtual ~Node() override
    {
        Graph::unregister_node(*this);

        delete[] m_input_pool;
        delete[] m_output_pool;
    }

    // --------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}
    // unused interface override

    // --------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    // parses the local graph and builds appropriate connections between this Node
    // and its subnodes, given the selected dispatch mode
    // --------------------------------------------------------------------------------------------
    {
        Graph::register_node(*this);

        if (m_subnodes.empty())
            return;

        for (auto& subnode : m_subnodes)
             subnode->set_parent(this);

        switch(m_dispatch)
        {
        case Dispatch::Values::Upwards:
        {
            // subnode's chain out connects to this Node
            for (auto& subnode : m_subnodes)
            {
                auto& source = subnode->chainout();
                Graph::connect(source, *this, source.routing());
            }
            break;
        }
        case Dispatch::Values::Downwards:
        {
            // connect this Node default outputs to first subnode
            auto& front = *m_subnodes.front();
            Graph::connect(*this, front.chainout());

            // chain the following subnodes, until last is reached
            for (int n = 0; n < m_subnodes.count(); ++n)
            {
                auto& source = m_subnodes[n]->chainout();
                auto& dest = m_subnodes[n+1]->chainout();
                Graph::connect(source, dest);
            }
        }
        }

        allocate_sockets(Graph::vector());
    }

    // --------------------------------------------------------------------------------------------
    virtual void
    setTarget(const QQmlProperty& target) override
    // a convenience method for quickly adding a processor Node in between
    // target Socket and the actual output (QML)
    // --------------------------------------------------------------------------------------------
    {
        auto socket = target.read().value<Socket*>();

        switch (socket->polarity())
        {
        case INPUT:  Graph::connect(*this, *socket); break;
        case OUTPUT: Graph::connect(*socket, *this);
        }
    }

    // --------------------------------------------------------------------------------------------
    virtual void
    initialize(Graph::properties& properties) {}
    // this is called when the graph and allocation are complete

    // --------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) = 0;
    // the main processing function to override

    // --------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) {}
    // this can be overriden and will be called each time the sample rate changes

    // --------------------------------------------------------------------------------------------
    virtual QString
    name() const { return "Unnamed"; }

    // --------------------------------------------------------------------------------------------
    Routing
    routing()
    // returns the default audio/midi output routing (audio comes first)
    // null routing if there is none
    // --------------------------------------------------------------------------------------------
    {
        if (auto out = default_audio_outputs())
            return out->routing();
        else if (auto out = default_midi_outputs())
            return out->routing();

        return Routing();
    }

    // --------------------------------------------------------------------------------------------
    void
    set_routing(Routing r)
    // --------------------------------------------------------------------------------------------
    {
        if (auto out = default_audio_outputs())
            out->set_routing(r);

        else if (auto out = default_midi_outputs())
            out->set_routing(r);
    }

    // --------------------------------------------------------------------------------------------
    void
    set_mul(qreal mul)
    // set default outputs level + postfader auxiliary connections level
    // --------------------------------------------------------------------------------------------
    {
        if (!m_outputs.empty())
             m_outputs[0]->set_mul(mul);
        m_mul = mul;
    }

    // --------------------------------------------------------------------------------------------
    void
    set_add(qreal add)
    // --------------------------------------------------------------------------------------------
    {
        if (!m_outputs.empty())
            m_outputs[0]->set_add(add);
        m_add = add;
    }

    // --------------------------------------------------------------------------------------------
    WPN_OK void
    set_muted(bool muted) noexcept
    // mute/unmute all output sockets
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket: m_outputs)
             socket->set_muted(muted);
    }

    // --------------------------------------------------------------------------------------------
    WPN_OK void
    set_active(bool active) noexcept
    // activate/deactivate all output socket connections
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket: m_outputs)
            for (auto& connection : socket->connections())
                connection->set_active(active);
    }

    // --------------------------------------------------------------------------------------------
    void
    set_parent(Node* parent) { m_parent = parent; }
    // sets Node's parent Node, creating appropriate connections

    // --------------------------------------------------------------------------------------------
    void
    register_socket(Socket& s) { sockets(s.polarity()).push_back(&s); }
    // this is called from Socket's constructor
    // adds a pointer to the newly created Socket

    // --------------------------------------------------------------------------------------------
    std::vector<Socket*>&
    sockets(polarity_t polarity) noexcept
    // returns Node's socket vector matching given polarity
    // --------------------------------------------------------------------------------------------
    {
        switch(polarity) {
            case INPUT: return m_inputs;
            case OUTPUT: return m_outputs;
        }
    }

    // --------------------------------------------------------------------------------------------
    Socket*
    default_audio_inputs() noexcept
    // returns Node's default inputs
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : m_inputs)
            if (socket->type() == Socket::Type::Audio)
                return socket;
        return nullptr;
    }

    // --------------------------------------------------------------------------------------------
    Socket*
    default_audio_outputs() noexcept
    // returns Node's default outputs
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : m_outputs)
            if (socket->type() == Socket::Type::Audio)
                return socket;
        return nullptr;
    }

    // --------------------------------------------------------------------------------------------
    Socket*
    default_midi_inputs() noexcept
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket: m_inputs)
            if (socket->type() == Socket::Type::Midi_1_0)
                return socket;
        return nullptr;
    }

    // --------------------------------------------------------------------------------------------
    Socket*
    default_midi_outputs() noexcept
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : m_outputs)
            if (socket->type() == Socket::Type::Midi_1_0)
                return socket;
        return nullptr;
    }

    // --------------------------------------------------------------------------------------------
    bool
    connected() noexcept { return (connected(INPUT) || connected(OUTPUT)); }
    // returns true if Node is connected to anything

    // --------------------------------------------------------------------------------------------
    bool
    connected(polarity_t polarity) noexcept
    // returns true if Node is connected to anything (given polarity)
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : sockets(polarity))
            if (socket->connected())
                return true;
        return false;
    }

    // --------------------------------------------------------------------------------------------
    template<typename T> bool
    connected(T const& target) noexcept
    // redirect to input/output sockets
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket: sockets(INPUT))
            if (socket->connected(target))
                return true;
        for (auto& socket: sockets(OUTPUT))
            if (socket->connected(target))
                return true;

        return false;
    }

    // --------------------------------------------------------------------------------------------
    Node&
    chainout() noexcept
    // returns the last Node in the parent-children chain
    // this would be the Node producing the chain's final output
    // --------------------------------------------------------------------------------------------
    {
        switch(m_dispatch)
        {
        case Dispatch::Values::Upwards: return *this;
        case Dispatch::Values::Downwards:
        {
            if (m_subnodes.empty())
                return *this;
            else return *m_subnodes.back();
        }
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

    // --------------------------------------------------------------------------------------------
    bool
    processed() const noexcept { return m_processed; }

    // --------------------------------------------------------------------------------------------
    void
    set_processed(bool processed) { m_processed = processed; }

private:

    // --------------------------------------------------------------------------------------------
    void
    reset_process(vector_t nframes) noexcept
    // resets all input sockets
    // marks Node's processed flag as false
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : m_inputs)
             socket->reset(nframes);

        m_processed = false;
    }

    // --------------------------------------------------------------------------------------------
    void
    allocate_sockets(vector_t nframes)
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : m_inputs)
             socket->allocate(nframes);

        for (auto& socket : m_outputs)
             socket->allocate(nframes);
    }

    // --------------------------------------------------------------------------------------------
    void
    allocate_pools(vector_t nframes)
    // --------------------------------------------------------------------------------------------
    {
        m_input_pool   = new sample_t**[m_inputs.size()];
        m_output_pool  = new sample_t**[m_outputs.size()];

        nchannels_t n = 0;
        for (auto& socket : m_inputs) {
            m_input_pool[n] = socket->buffer();
            n++;
        }

        n = 0;
        for (auto& socket : m_outputs) {
            m_output_pool[n] = socket->buffer();
            n++;
        }
    }

    // --------------------------------------------------------------------------------------------
    void
    process(vector_t nframes) noexcept
    // pre-processing main function
    // - pull input connections
    // - call rwrite
    // - mark node as processed
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : m_inputs)
            for (auto& connection : socket->connections())
                if (connection->active())
                    connection->pull(nframes);

        rwrite(m_input_pool, m_output_pool, nframes);
        m_processed = true;
    }

    // --------------------------------------------------------------------------------------------
    pool&
    outputs() noexcept { return m_output_pool; }
    // returns a collection of all socket outputs

    // --------------------------------------------------------------------------------------------
    pool
    m_input_pool  = nullptr,
    m_output_pool = nullptr;

    // --------------------------------------------------------------------------------------------
    Spatial
    m_spatial;

    // --------------------------------------------------------------------------------------------
    std::vector<Socket*>
    m_inputs,
    m_outputs;

    // --------------------------------------------------------------------------------------------
    QVector<Node*>
    m_subnodes;

    // --------------------------------------------------------------------------------------------
    bool
    m_muted = false,
    m_active = true,
    m_processed = false;

    // --------------------------------------------------------------------------------------------
    Dispatch::Values
    m_dispatch = Dispatch::Values::Upwards;

    // --------------------------------------------------------------------------------------------
    Node*
    m_parent = nullptr;

    // --------------------------------------------------------------------------------------------
    qreal
    m_mul = 1,
    m_add = 0;


};

#endif // QTWRAPPER2_HPP
