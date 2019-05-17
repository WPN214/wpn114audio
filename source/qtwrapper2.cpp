#include "qtwrapper2.hpp"

wpn_graph Graph::m_graph;

Graph::Graph()
{
    m_graph = wpn_graph_create(44100, 512, 64);
}

wpn_graph& Graph::instance()
{
    return m_graph;
}

Graph::~Graph()
{
    // this also takes care of cleaning up nodes and connections
    wpn_graph_free(&m_graph);
}

void Graph::setRate(qreal rate)
{
    m_graph.properties.rate = rate;
}

void Graph::setMxv(int mxv)
{
    m_graph.properties.mxvsz = mxv;
}

void Graph::setMnv(int mnv)
{
    m_graph.properties.mnvsz = mnv;
}

void Graph::componentComplete()
{
    // dunno what to do here..
}

inline wpn_connection&
Graph::connect(Node& source, Node& dest, wpn_routing routing)
{
    return *wpn_graph_nconnect(&m_graph, source.c_node, dest.c_node, routing);
}

inline wpn_connection&
Graph::connect(Socket& source, Socket& dest, wpn_routing routing)
{
    return *wpn_graph_sconnect(&m_graph, source.c_socket, dest.c_socket, routing);
}

inline wpn_connection&
Graph::connect(Node& source, Socket& dest, wpn_routing routing)
{
    return *wpn_graph_nsconnect(&m_graph, source.c_node, dest.c_socket, routing);
}

inline wpn_connection&
Graph::connect(Socket &source, Node &dest, wpn_routing routing)
{
    return *wpn_graph_snconnect(&m_graph, source.c_socket, dest.c_node, routing);
}

inline void Graph::disconnect(Socket& s, wpn_routing routing)
{
    wpn_graph_sdisconnect(&m_graph, s.c_socket, routing);
}

QQmlListProperty<Node> Graph::subnodes()
{
    return QQmlListProperty<Node>(
        this, this,
        &Graph::append_subnode,
        &Graph::nsubnodes,
        &Graph::subnode,
        &Graph::clear_subnodes);
}

Node* Graph::subnode(int index) const
{
    return m_subnodes[index];
}

void Graph::append_subnode(Node* n)
{
    m_subnodes.push_back(n);
}

int Graph::nsubnodes() const
{
    return m_subnodes.count();
}

void Graph::clear_subnodes()
{
    m_subnodes.clear();
}

void Graph::append_subnode(QQmlListProperty<Node>* l, Node* n)
{
    reinterpret_cast<Graph*>(l->data)->append_subnode(n);
}

void Graph::clear_subnodes(QQmlListProperty<Node>* l)
{
    reinterpret_cast<Graph*>(l->data)->clear_subnodes();
}

Node* Graph::subnode(QQmlListProperty<Node>* l, int index)
{
    return reinterpret_cast<Graph*>(l->data)->subnode(index);
}

int Graph::nsubnodes(QQmlListProperty<Node>* l)
{
    return reinterpret_cast<Graph*>(l->data)->nsubnodes();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

Socket::Socket() {}
Socket::Socket(Socket const& cp) : c_socket(cp.c_socket) {}
Socket::Socket(wpn_socket* csock) : c_socket(csock) {}

Connection::Connection() {}
Connection::Connection(Connection const&) {}
Connection::Connection(wpn_connection* ccon) : c_connection(ccon) {}

void Connection::componentComplete()
{

}

void Connection::setTarget(const QQmlProperty& property)
{
    // 'connection' on 'something'
}

//-------------------------------------------------------------------------------------------------
// NODE
//-------------------------------------------------------------------------------------------------

Node::Node()   {}
Node::~Node()  {}

void Node::setMuted(bool muted)
{
    // mute all output connections
    m_muted = muted;
}

void Node::setActive(bool active)
{
    // deactivate all input/output connections
    m_active = active;
}

void Node::setLevel(qreal level)
{
    // set
    m_level = level;
}

void Node::setParent(Node* parent)
{
    m_parent = parent;
}

void Node::setRouting(QVariantList routing)
{
    // routing for direct & explicit outputs
}

void Node::setDispatch(Dispatch::Values dispatch)
{
    m_dispatch = dispatch;
}

inline Node& Node::chainout()
{
    switch (m_dispatch)
    {
    case Dispatch::Values::Upwards:
    {
        return *this;
    }
    case Dispatch::Values::Downwards:
    {
        if  (m_subnodes.isEmpty())
             return *this;
        else return *m_subnodes.back();
    }
    }

    assert(false);
}

wpn_routing Node::parseRouting()
{
    wpn_routing routing;

    if (m_routing.isEmpty())
        return routing;

    for ( int n = 0; n < m_routing.size(); n += 2) {
        routing.cables[n][0] = m_routing[n].toInt();
        routing.cables[n][1] = m_routing[n+1].toInt();
        routing.ncables++;
    }

    return routing;
}

qreal Node::db(qreal v)
{
    // this is not an a->db function
    // it is a useful, readable and declarative way to signal that the value
    // used within qml is of the db unit.
    // consequently, it returns the value as a normal linear amplitude value.
    // in general, to ease-up calculations, the db should be calculated once and upfront
    return pow(10, v*.05);
}

void Node::componentComplete()
{
    // handle parent/children connections

    if (m_subnodes.isEmpty())
        return;

    for (auto& subnode : m_subnodes)
         subnode->setParent(this);

    switch(m_dispatch)
    {
    case Dispatch::Values::Upwards:
    {
        // all subnodes connnect to this node
        for (auto& subnode : m_subnodes) {
            auto& source = subnode->chainout();
            Graph::connect(source, *this, source.parseRouting());
        }

        break;
    }
    case Dispatch::Values::Downwards:
    {
        auto& front = *m_subnodes.front();
        Graph::connect(*this, front.chainout());

        for (int n = 0; n < m_subnodes.count()-1; ++n) {
            auto& source = m_subnodes[n]->chainout();
            auto& dest = m_subnodes[n+1]->chainout();
            Graph::connect(source, dest);
        }
    }
    }
}

void Node::setTarget(const QQmlProperty& property)
{
    // 'this' on 'property'
    auto node = qobject_cast<Node*>(property.object());

}

QVariant Node::socket_get(Socket& s) const
{
    return QVariant::fromValue(s);
}

void Node::socket_set(Socket& s, QVariant v)
{
    // single-assignment case

    if (v.canConvert<Socket>())
    {
        Socket target = v.value<Socket>();
        Graph::disconnect(s);
        Graph::connect(s, target);
    }
    else if (v.canConvert<Connection*>())
    {
        auto connection = v.value<Connection>();
        if (s.polarity() == INPUT) {
            auto source = connection.source();
            auto routing = connection.routing();
            wpn_graph_sconnect(&Graph::instance(), source, s.c_socket, routing);
        } else {
            auto dest = connection.dest();
            auto routing = connection.routing();
            wpn_graph_sconnect(&Graph::instance(), s.c_socket, dest, routing);
        }
    }
    else if (v.canConvert<QVariantList>())
    {
        // TODO!
    }
    else if (v.canConvert<qreal>())
    {

    }
}

QQmlListProperty<Node> Node::subnodes()
{
    return QQmlListProperty<Node>(
        this, this,
        &Node::append_subnode,
        &Node::nsubnodes,
        &Node::subnode,
        &Node::clear_subnodes);
}

Node* Node::subnode(int index) const
{
    return m_subnodes[index];
}

void Node::append_subnode(Node* n)
{
    m_subnodes.push_back(n);
}

int Node::nsubnodes() const
{
    return m_subnodes.count();
}

void Node::clear_subnodes()
{
    m_subnodes.clear();
}

void Node::append_subnode(QQmlListProperty<Node>* l, Node* n)
{
    reinterpret_cast<Node*>(l->data)->append_subnode(n);
}

void Node::clear_subnodes(QQmlListProperty<Node>* l)
{
    reinterpret_cast<Node*>(l->data)->clear_subnodes();
}

Node* Node::subnode(QQmlListProperty<Node>* l, int index)
{
    return reinterpret_cast<Node*>(l->data)->subnode(index);
}

int Node::nsubnodes(QQmlListProperty<Node>* l)
{
    return reinterpret_cast<Node*>(l->data)->nsubnodes();
}

JackIO::JackIO() {}
JackIO::~JackIO() {}

void JackIO::componentComplete()
{
    wpn_io_jack_register(&c_jack, &Graph::instance(), static_cast<nchannels_t>(m_n_outputs));
}

void JackIO::run(QString s)
{
    std::string str = s.toStdString();

    if  (s.isEmpty())
         wpn_io_jack_run(&c_jack);
    else wpn_io_jack_connect_run(&c_jack, nullptr, str.c_str());
}

//-------------------------------------------------------------------------------------------------

Sinetest::Sinetest()
{
    wpn_sinetest_register(&c_sinetest, &Graph::instance());
}
