#ifndef QTWRAPPER2_HPP
#define QTWRAPPER2_HPP

#include <QObject>
#include <QQmlParserStatus>
#include <QQmlProperty>
#include <QQmlPropertyValueSource>
#include <QQmlListProperty>
#include <QVector>
#include <QVariant>

#include <wpn114audio/core/graph.h>
#include <wpn114audio/nodes/utilities/sinetest.h>

class Node;

#define WPN_SOCKET(_s)                                                        \
private: Q_PROPERTY(QVariant _s READ get##_s WRITE set##_s NOTIFY _s##Changed)                      \
protected: Socket m_##_s;                                               \
                                                                                                     \
QVariant get##_s() { return socket_get(m_##_s); }                                                    \
void set##_s(QVariant v) { socket_set(m_##_s, v); }                                                  \
Q_SIGNAL void _s##Changed();

//-------------------------------------------------------------------------------------------------
class Socket : public QObject
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    public:
    Socket();
    Socket(Socket const&);
    Socket(wpn_socket* csock);

    polarity_t polarity() { return c_socket->polarity; }
    wpn_socket* c_socket = nullptr;
};

Q_DECLARE_METATYPE(Socket)

//-------------------------------------------------------------------------------------------------
class Graph : public QObject, public QQmlParserStatus
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    Q_PROPERTY   (qreal rate READ rate WRITE setRate)
    Q_PROPERTY   (int mxv READ mxv WRITE setMxv)
    Q_PROPERTY   (int mnv READ mnv WRITE setMnv)
    Q_INTERFACES (QQmlParserStatus)

    Q_PROPERTY   (QQmlListProperty<Node> subnodes READ subnodes )
    Q_CLASSINFO  ("DefaultProperty", "subnodes" )

    public:
    Graph();
    ~Graph() override;

    virtual void classBegin() override {}
    virtual void componentComplete() override;

    static wpn_graph& instance();

    static wpn_connection&
    connect(Socket& source, Socket& dest, wpn_routing routing = {});

    static wpn_connection&
    connect(Node& source, Node& dest, wpn_routing routing = {});

    static wpn_connection&
    connect(Node& source, Socket& dest, wpn_routing routing = {});

    static wpn_connection&
    connect(Socket& source, Node& dest, wpn_routing routing = {});

    static void
    disconnect(Socket&, wpn_routing routing = {});

    qreal rate() const { return m_graph.properties.rate; }
    int mxv() const { return m_graph.properties.mxvsz; }
    int mnv() const { return m_graph.properties.mnvsz; }

    void setRate(qreal rate);
    void setMxv(int);
    void setMnv(int);

    QQmlListProperty<Node> subnodes();
    Q_INVOKABLE void append_subnode(Node*);
    Q_INVOKABLE int nsubnodes() const;
    Q_INVOKABLE Node* subnode(int) const;
    Q_INVOKABLE void clear_subnodes();

    static void append_subnode  (QQmlListProperty<Node>*, Node*);
    static int nsubnodes        (QQmlListProperty<Node>*);
    static Node* subnode        (QQmlListProperty<Node>*, int);
    static void clear_subnodes  (QQmlListProperty<Node>*);

    private:
    static wpn_graph m_graph;
    QVector<Node*> m_subnodes;
};

//-------------------------------------------------------------------------------------------------
class Connection : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT    
    Q_INTERFACES (QQmlParserStatus QQmlPropertyValueSource)

    public:
    Connection();
    Connection(Connection const&);
    Connection(wpn_connection* ccon);

    virtual void classBegin() override {}
    virtual void componentComplete() override;
    virtual void setTarget(const QQmlProperty &) override;

    wpn_socket* source()    { return c_connection->source; }
    wpn_socket* dest()      { return c_connection->dest; }
    wpn_routing routing()   { return c_connection->routing; }

    private:
    wpn_connection* c_connection = nullptr;
};

Q_DECLARE_METATYPE(Connection)

//-------------------------------------------------------------------------------------------------
class Dispatch : public QObject
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    public:
    enum Values { Upwards = 0, Downwards  = 1 };
    Q_ENUM (Values)
};

//-------------------------------------------------------------------------------------------------
class Node : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    Q_PROPERTY    ( Node* parent READ parent WRITE setParent NOTIFY parentChanged )
    Q_PROPERTY    ( QQmlListProperty<Node> subnodes READ subnodes )

    Q_PROPERTY    ( Dispatch::Values dispatch READ dispatch WRITE setDispatch )
    Q_PROPERTY    ( bool active READ active WRITE setActive )
    Q_PROPERTY    ( bool muted READ muted WRITE setMuted )
    Q_PROPERTY    ( qreal level READ level WRITE setLevel )
    Q_PROPERTY    ( QVariantList routing READ routing WRITE setRouting )

    Q_CLASSINFO   ( "DefaultProperty", "subnodes" )
    Q_INTERFACES  ( QQmlParserStatus QQmlPropertyValueSource )

    friend class Graph;

    public:
    Node();
    virtual ~Node() override;

    void classBegin() override {}
    virtual void componentComplete() override;
    virtual void setTarget(const QQmlProperty &) override;

    QVariant socket_get(Socket&) const;
    void socket_set(Socket&, QVariant);

    Node& chainout();

    bool muted    () const { return m_muted; }
    bool active   () const { return m_active; }
    qreal level   () const { return m_level; }
    Node* parent  () const { return m_parent; }

    QVariantList routing() const { return m_routing; }
    wpn_routing parseRouting();

    void setMuted      (bool);
    void setActive     (bool);
    void setLevel      (qreal);
    void setParent     (Node*);
    void setRouting    (QVariantList);
    void setDispatch   (Dispatch::Values);

    QQmlListProperty<Node> subnodes();
    Q_INVOKABLE void append_subnode(Node*);
    Q_INVOKABLE int nsubnodes() const;
    Q_INVOKABLE Node* subnode(int) const;
    Q_INVOKABLE void clear_subnodes();

    static void append_subnode  (QQmlListProperty<Node>*, Node*);
    static int nsubnodes        (QQmlListProperty<Node>*);
    static Node* subnode        (QQmlListProperty<Node>*, int);
    static void clear_subnodes  (QQmlListProperty<Node>*);

    Dispatch::Values dispatch() const { return m_dispatch; }

    Q_INVOKABLE qreal db(qreal v);

    signals:
    void parentChanged();

    private:
    bool m_active   = true;
    bool m_muted    = false;
    Node* m_parent  = nullptr;
    qreal m_level   = 1;

    QVariantList m_routing;
    QVector<Node*> m_subnodes;    
    Dispatch::Values m_dispatch = Dispatch::Values::Upwards;
    wpn_node* c_node = nullptr;
};

#include <wpn114audio/nodes/io/io_jack.h>

//-------------------------------------------------------------------------------------------------
class JackIO : public Node
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    Q_PROPERTY  (int numInputs MEMBER m_n_inputs)
    Q_PROPERTY  (int numOutputs MEMBER m_n_outputs)

    WPN_SOCKET  (inputs)
    WPN_SOCKET  (outputs)

    public:
    JackIO();
    ~JackIO();

    virtual void componentComplete() override;

    Q_INVOKABLE void run(QString = {});

    protected:
    wpn_io_jack c_jack;
    int m_n_inputs  = 0;
    int m_n_outputs = 2;
};

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
class Sinetest : public Node
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT
    Q_PROPERTY  (qreal frequency READ frequency WRITE setFrequency)
    WPN_SOCKET  (output)

    public:
    Sinetest();

    qreal frequency() const { return c_sinetest.frequency; }
    void setFrequency(qreal f) { c_sinetest.frequency = f; }

    private:
    wpn_sinetest c_sinetest;
};

//-------------------------------------------------------------------------------------------------
class VCA : public Node
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT
    WPN_SOCKET  (inputs)
    WPN_SOCKET  (gain)
    WPN_SOCKET  (outputs)

    public:
    VCA();
};

#endif // QTWRAPPER2_HPP
