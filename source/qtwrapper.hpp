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

class Node;

// --------------------------------------------------------------------------------------------------
// CONVENIENCE MACRO DEFINITIONS
// --------------------------------------------------------------------------------------------------

#define WPN_OBJECT                                                                                  \
Q_OBJECT

#define WPN_SOCKET(_pol, _nm, _idx, _nchn)                                                          \
private: Q_PROPERTY(Socket _nm READ get##_nm WRITE set##_nm NOTIFY _nm##Changed)                    \
protected: Socket m_##_nm { this, _pol, _idx, _nchn };                         \
                                                                                                    \
Socket get##_nm() { return m_##_nm; }                                                               \
void set##_nm(Socket v) {  }                                                                        \
Q_SIGNAL void _nm##Changed();

#define WPN_ENUM_INPUTS(...)                                                                        \
enum Inputs {__VA_ARGS__, N_IN};

#define WPN_ENUM_OUTPUTS(...)                                                                       \
enum Outputs {__VA_ARGS__, N_OUT};

#define WPN_INPUT_DECLARE(_nm, _nchn)                                                               \
WPN_SOCKET(INPUT, _nm, _nm, _nchn)

#define WPN_OUTPUT_DECLARE(_nm, _nchn)                                                              \
WPN_SOCKET(OUTPUT, _nm, _nm, _nchn)

#define FOR_NCHANNELS(_iter, _target)                                                               \
for(uint8_t _iter = 0; _iter < m_##_target.nchannels(); ++_iter) {

#define FOR_NFRAMES(_iter, _target)                                                                 \
for(uint16_t _iter = 0; _iter < _target; ++_iter) {

#define END }

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

}

//-------------------------------------------------------------------------------------------------
class Socket : public QObject
// represents a node single input/output
// with nchannels_t channels
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(bool
    muted MEMBER m_muted WRITE set_muted)
    // MUTED property: manages and overrides all Socket connections mute property

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(qreal
    level MEMBER m_level WRITE set_level)
    // LEVEL property: manages and overrides all Socket connections level property

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(int
    nchannels MEMBER m_nchannels WRITE set_nchannels)
    // NCHANNELS property: for multichannel expansion
    // and dynamic channel setting/allocation

    // --------------------------------------------------------------------------------------------
    friend class Connection;
    friend class Graph;
    friend class Node;

public:

    // --------------------------------------------------------------------------------------------
    Socket() {}

    // --------------------------------------------------------------------------------------------
    Socket(Node* parent, polarity_t polarity, uint8_t index, uint8_t nchannels);

    // --------------------------------------------------------------------------------------------
    Socket(Socket const& cp) :
        // this is only called because of the macro Socket definitions
        // it shouldn't be called otherwise in any other context
        m_polarity  (cp.m_polarity)
      , m_index     (cp.m_index)
      , m_nchannels (cp.m_nchannels)
      , m_parent    (cp.m_parent) {}

    // --------------------------------------------------------------------------------------------
    Socket&
    operator=(Socket const& cp)
    // same as for the copy constructor
    // --------------------------------------------------------------------------------------------
    {
        m_polarity = cp.m_polarity;
        m_index = cp.m_index;
        m_nchannels = cp.m_nchannels;
        m_parent = cp.m_parent;
    }

    // --------------------------------------------------------------------------------------------
    ~Socket() { free(m_buffer); }

    // --------------------------------------------------------------------------------------------
    void
    set_muted(bool muted);
    // mute/unmute all output connections

    // --------------------------------------------------------------------------------------------
    void
    set_level(qreal level);

    // --------------------------------------------------------------------------------------------
    void
    set_nchannels(nchannels_t nchannels)
    // sets num_channels for socket and
    // allocate/reallocate its buffer
    {
        m_nchannels = nchannels;
    }

    // --------------------------------------------------------------------------------------------
    nchannels_t
    nchannels() const { return m_nchannels; }
    // returns Socket number of channels

    // --------------------------------------------------------------------------------------------
    polarity_t
    polarity() const { return m_polarity; }
    // returns Socket polarity (INPUT/OUTPUT)

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

private:

    // --------------------------------------------------------------------------------------------
    void
    allocate(vector_t nframes)
    // --------------------------------------------------------------------------------------------
    {
        if (m_buffer != nullptr)
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
        std::remove(m_connections.begin(), m_connections.end(), &con);
    }

    // --------------------------------------------------------------------------------------------
    polarity_t
    m_polarity = OUTPUT;

    // --------------------------------------------------------------------------------------------
    uint8_t
    m_index = 0;

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
    m_level = 1;
};

//-------------------------------------------------------------------------------------------------
class Routing : public QObject
// connection matrix between 2 sockets
// materialized as a QVariantList within QML
//-------------------------------------------------------------------------------------------------
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
    bool null() const { return m_routing.empty(); }
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

//-------------------------------------------------------------------------------------------------
class Connection : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Socket*
    source MEMBER m_source)
    // Connection source Socket (output polarity) from which the signal will transit

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Socket*
    dest MEMBER m_dest)
    // Connection dest Socket (input polarity) to which the signal will transit

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(QVariantList
    routing READ routing WRITE set_routing)
    // Connection's channel-to-channel routing matrix
    // materialized as a QVariantList within QML
    // e.g. [0, 1, 1, 0] will connect output 0 to input 1 and output 1 to input 0
    // TODO: [[0,1],[1,0]] format as well

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY (qreal
    level MEMBER m_level)
    // Connection's amplitude level (linear)
    // scales the amplitude of the input signal to transit to the output
    // this property might be overriden or scaled by Socket's or Node's level property
    // TODO: prefader/postfader property

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(bool
    muted MEMBER m_muted)
    // Connection will still process when muted
    // but will output zeros instead of the normal signal

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(bool
    active MEMBER m_active)
    // if inactive, Connection won't process the upstream part of the Graph
    // to which it is connected

    // --------------------------------------------------------------------------------------------
    Q_INTERFACES
    (QQmlParserStatus QQmlPropertyValueSource)

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
      , m_level(cp.m_level) {}
    // copy constructor, don't really know in which case it should/will be used

    // --------------------------------------------------------------------------------------------
    Connection(Socket& source, Socket& dest, Routing matrix) :
        m_source(&source)
      , m_dest(&dest)
      , m_routing(matrix) {}
    // constructor accessed from C++

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
    activate() { m_active = true; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    deactivate() { m_active = false; }

    // --------------------------------------------------------------------------------------------
    void
    set_active(bool active) { m_active = active; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    mute() { m_muted = true; }

    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    unmute() { m_muted = false; }

    // --------------------------------------------------------------------------------------------
    void
    set_muted(bool muted) { m_muted = muted; }

    // --------------------------------------------------------------------------------------------
    bool
    active() const { return m_active; }

    // --------------------------------------------------------------------------------------------
    bool
    muted() const { return m_muted; }

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
    routing() const { return m_routing.to_qvariant(); }
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
    feedback() const { return m_feedback; }

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
    pull(vector_t nframes);
    // the main processing function

    // --------------------------------------------------------------------------------------------
    nchannels_t
    m_nchannels = 0;

    // --------------------------------------------------------------------------------------------
    Socket*
    m_source = nullptr;
    // the Connection's source (output polarity)

    // --------------------------------------------------------------------------------------------
    Socket*
    m_dest = nullptr;
    // the Connection's dest (input polarity)

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
    m_level = 1;
    // Connection's output will be scaled by this ratio
    // (linear amplitude level, not logarithmic)
};

Q_DECLARE_METATYPE(Connection)

//-------------------------------------------------------------------------------------------------
class Graph : public QObject, public QQmlParserStatus
// embeds all connections between instantiated nodes
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(int
    vector READ vector WRITE set_vector)
    // Graph's signal vector size (lower is better, but will increase CPU usage)

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(QQmlListProperty<Node>
    subnodes READ subnodes)
    // this is the default list property

    // --------------------------------------------------------------------------------------------
    Q_CLASSINFO
    ("DefaultProperty", "subnodes")

    // --------------------------------------------------------------------------------------------
    Q_INTERFACES
    (QQmlParserStatus)

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
    register_node(Node& node);

    // --------------------------------------------------------------------------------------------
    static void
    unregister_node(Node& node)
    {
        std::remove(s_nodes.begin(), s_nodes.end(), &node);
    }

    // --------------------------------------------------------------------------------------------
    static void
    add_connection(Connection& con) { s_connections.push_back(con); }
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
    static void
    disconnect(Socket& source, Socket& dest)
    // disconnects the two Sockets
    // --------------------------------------------------------------------------------------------
    {
        if (auto con = get_connection(source, dest)) {
            // TODO
        }
    }

    // --------------------------------------------------------------------------------------------
    static pool&
    run(Node& target);
    // the main processing function

    // --------------------------------------------------------------------------------------------
    static uint16_t
    vector() { return s_properties.vector; }
    // returns graph vector size

    static void
    set_vector(uint16_t vector) { s_properties.vector = vector; }

    // --------------------------------------------------------------------------------------------
    Q_SIGNAL void
    rateChanged(sample_t);

    // --------------------------------------------------------------------------------------------
    static sample_t
    rate() { return s_properties.rate; }
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

//-------------------------------------------------------------------------------------------------
struct spatial_t
// TODO
//-------------------------------------------------------------------------------------------------
{
    qreal x = 0, y = 0, z = 0;
    qreal w = 0, d = 0, h = 0;
};

//-------------------------------------------------------------------------------------------------
class Spatial : public QObject
// TODO
//-------------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------------
class Dispatch : public QObject
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    public:    
    enum Values { Upwards = 0, Downwards = 1 }; Q_ENUM (Values)
};

//-------------------------------------------------------------------------------------------------
class Node : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
/*! \class Node
 * \brief the main Node class */
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(bool
    muted MEMBER m_muted WRITE set_muted)    
    /*!
     * \property Node::muted
     * \brief mutes/unmutes all Node's outputs
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(bool
    active MEMBER m_active WRITE set_active)
    /*!
     * \property Node::active
     * \brief activates/deactivates Node processing
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(qreal
    level MEMBER m_level WRITE set_level)
    /*!
     * \property Node::level
     * \brief sets Node's default output levels (linear amplitude)
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Spatial
    space MEMBER m_spatial)
    /*!
     * \property Node::space
     * \brief unimplemented yet
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(QQmlListProperty<Node>
    subnodes READ subnodes)
    /*!
     * \property Node::subnodes
     * \brief the list of subnodes that are connected to this Node
    */

    // --------------------------------------------------------------------------------------------
    Q_PROPERTY(Dispatch::Values
    dispatch MEMBER m_dispatch)
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
    void
    set_level(qreal level)
    // set default outputs level + postfader auxiliary connections level
    // --------------------------------------------------------------------------------------------
    {
        if (!m_outputs.empty())
             m_outputs[0]->set_level(level);
    }

    // --------------------------------------------------------------------------------------------
    void
    set_muted(bool muted)
    // mute/unmute all output sockets
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket: m_outputs)
             socket->set_muted(muted);
    }

    // --------------------------------------------------------------------------------------------
    void
    set_active(bool active)
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
    sockets(polarity_t polarity)
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
    default_inputs()
    // returns Node's default inputs
    // --------------------------------------------------------------------------------------------
    {
        if (m_inputs.empty())
             return nullptr;
        else return m_inputs.front();
    }

    // --------------------------------------------------------------------------------------------
    Socket*
    default_outputs()
    // returns Node's default outputs
    // --------------------------------------------------------------------------------------------
    {
        if (m_outputs.empty())
             return nullptr;
        else return m_outputs.front();
    }

    // --------------------------------------------------------------------------------------------
    bool
    connected() { return (connected(INPUT) || connected(OUTPUT)); }
    // returns true if Node is connected to anything

    // --------------------------------------------------------------------------------------------
    bool
    connected(polarity_t polarity)
    // returns true if Node is connected to anything (given polarity)
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : sockets(polarity))
            if (socket->connected())
                return true;
        return false;
    }

    // --------------------------------------------------------------------------------------------
    bool
    connected(Node const& target)
    // returns true if this Node is connected to target Node
    // --------------------------------------------------------------------------------------------
    {
        for (auto& socket : sockets(INPUT))
            if (socket->connected(target))
                return true;

        for (auto& socket : sockets(OUTPUT))
            if (socket->connected(target))
                return true;

        return false;
    }

    // --------------------------------------------------------------------------------------------
    bool
    connected(Socket const& target)
    // returns true if this Node is connected to target Socket
    // --------------------------------------------------------------------------------------------
    {

    }

    // --------------------------------------------------------------------------------------------
    Routing
    routing() const { return m_routing; }
    // returns Node's default output connection routing

    // --------------------------------------------------------------------------------------------
    Node&
    chainout()
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
    processed() const { return m_processed; }

private:

    // --------------------------------------------------------------------------------------------
    void
    reset_process(vector_t nframes)
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
        nchannels_t n_inputs = 0, n_outputs = 0;

        for (auto& socket : m_inputs)
             n_inputs += socket->nchannels();

        for (auto& socket : m_outputs)
             n_outputs += socket->nchannels();

        m_input_pool   = new sample_t**[n_inputs];
        m_output_pool  = new sample_t**[n_outputs];

        nchannels_t n = 0;
        for (auto& socket : m_inputs)
        {
            m_input_pool[n] = socket->buffer();
            ++n;
        }

        n = 0;
        for (auto& socket : m_outputs)
        {
            m_output_pool[n] = socket->buffer();
            ++n;
        }
    }

    // --------------------------------------------------------------------------------------------
    void
    process(vector_t nframes)
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
    outputs() { return m_output_pool; }
    // returns a collection of all socket outputs

    // --------------------------------------------------------------------------------------------
    pool
    m_input_pool = nullptr;

    // --------------------------------------------------------------------------------------------
    pool
    m_output_pool = nullptr;

    // --------------------------------------------------------------------------------------------
    Spatial
    m_spatial;

    // --------------------------------------------------------------------------------------------
    std::vector<Socket*>
    m_inputs;

    // --------------------------------------------------------------------------------------------
    std::vector<Socket*>
    m_outputs;

    // --------------------------------------------------------------------------------------------
    QVector<Node*>
    m_subnodes;

    // --------------------------------------------------------------------------------------------
    bool
    m_muted = false;

    // --------------------------------------------------------------------------------------------
    bool
    m_active = true;

    // --------------------------------------------------------------------------------------------
    bool
    m_processed = false;

    // --------------------------------------------------------------------------------------------
    qreal
    m_level = 1;

    // --------------------------------------------------------------------------------------------
    Dispatch::Values
    m_dispatch = Dispatch::Values::Upwards;

    // --------------------------------------------------------------------------------------------
    Routing
    m_routing;

    // --------------------------------------------------------------------------------------------
    Node*
    m_parent = nullptr;
};

//-------------------------------------------------------------------------------------------------
class Sinetest : public Node
/*!
* \class Sinetest
* \brief cheap sinusoidal generator
*/
//-------------------------------------------------------------------------------------------------
{       
    WPN_OBJECT
    // this marks initialize and rwrite as overriden

    //-------------------------------------------------------------------------------------------------
    WPN_ENUM_INPUTS  (frequency)
    WPN_ENUM_OUTPUTS (output)
    // index 0 of enum is always default input/output

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE  (frequency, 1)
    /*!
     * \property Sinetest::frequency
     * \brief (audio, input) in Hertz
     */

    //-------------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE (output, 1)
    /*!
     * \property Sinetest::output
     * \brief (audio, output) main out
     */

    static constexpr size_t esz = 16384;

public:

    //-------------------------------------------------------------------------------------------------
    Sinetest()
    // ctor, do what you want here
    //-------------------------------------------------------------------------------------------------
    {
        m_env = new sample_t[esz];

        for (size_t n = 0; n < esz; ++n)
            m_env[n] = sin(M_PI*2*n/esz);
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // the main processing function
    //-------------------------------------------------------------------------------------------------
    {
        auto frequency = inputs[Inputs::frequency][0];
        auto out = outputs[Outputs::output][0];

        for (vector_t f = 0; f < nframes; ++f) {
            m_phs += frequency[f]/m_rate * esz;
            out[f] = m_env[m_phs];
        }
    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override { m_rate = rate; }
    // this is called whenever Graph's sample rate changes

private:

    sample_t* m_env = nullptr;
    sample_t m_rate = 0;
    size_t m_phs = 0;
};

//-------------------------------------------------------------------------------------------------
class VCA : public Node
//-------------------------------------------------------------------------------------------------
{
    WPN_OBJECT
    WPN_ENUM_INPUTS     (inputs, gainmod)
    WPN_ENUM_OUTPUTS    (outputs)

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (inputs,  0 )
    /*!
     * \property VCA::inputs
     * \brief (audio) in, multichannel expansion
     */

    //-------------------------------------------------------------------------------------------------
    WPN_INPUT_DECLARE   (gainmod, 1 )
    /*!
     * \property VCA::gainmod
     * \brief (audio) gain modulation (0-1)
     */

    //-------------------------------------------------------------------------------------------------
    WPN_OUTPUT_DECLARE  (outputs, 0 )
    /*!
     * \property VCA::outputs
     * \brief (audio) out, multichannel expansion
     */

    //-------------------------------------------------------------------------------------------------
    Q_PROPERTY (sample_t gain MEMBER m_gain)
    /*!
     * \property VCA::gain
     * \brief (qreal) gain (linear)
     */

public:

    VCA() {}

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    // this Node will benefits from multichannel expansion if needed
    //-------------------------------------------------------------------------------------------------
    {
        auto in  = inputs[Inputs::inputs];
        auto gmd = inputs[Inputs::gainmod][0];
        auto out = outputs[Outputs::outputs];

        for (nchannels_t c = 0; c < m_outputs.nchannels(); ++c)
            for (vector_t f = 0; f < nframes; ++f)
                 out[c][f] = in[c][f] *= m_gain * gmd[f];
    }

private:
    sample_t m_gain = 1;

};

//-------------------------------------------------------------------------------------------------
class IOJack : public Node
//-------------------------------------------------------------------------------------------------
{
    WPN_OBJECT
    WPN_ENUM_INPUTS     (inputs)
    WPN_ENUM_OUTPUTS    (outputs)

    WPN_INPUT_DECLARE   (inputs, 0)
    WPN_OUTPUT_DECLARE  (outputs, 0)

    // additional Q_PROPERTY for non-audio qml properties
    Q_PROPERTY (int numInputs READ n_inputs WRITE setn_inputs)
    Q_PROPERTY (int numOutputs READ n_outputs WRITE setn_outputs)

    uint8_t m_n_inputs = 0, m_n_outputs = 2;

    public:

    //-------------------------------------------------------------------------------------------------
    IOJack()
    {

    }

    //-------------------------------------------------------------------------------------------------
    ~IOJack()
    {

    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}

    //-------------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    {

    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    run(QString target = {})
    {

    }

    //-------------------------------------------------------------------------------------------------
    Q_INVOKABLE void
    stop()
    {

    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool &inputs, pool &outputs, vector_t nframes) override
    {

    }

    //-------------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override
    {

    }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_inputs() const { return m_n_inputs; }

    //-------------------------------------------------------------------------------------------------
    uint8_t
    n_outputs() const { return m_n_outputs; }

    //-------------------------------------------------------------------------------------------------
    void
    setn_inputs(uint8_t n_inputs)
    {

    }

    //-------------------------------------------------------------------------------------------------
    void
    setn_outputs(uint8_t n_outputs)
    {

    }
};


#endif // QTWRAPPER2_HPP
