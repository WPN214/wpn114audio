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
#include <wpn114audio/nodes/io/io_jack.h>

class Node;

#define WPN_SOCKET(_s, _p)                                                                        \
private: Q_PROPERTY(QVariant _s READ get##_s WRITE set##_s NOTIFY _s##Changed)                    \
    protected: Socket m_##_s {this, #_s, _p };                                                    \
                                                                                                  \
QVariant get##_s() { return socket_get(m_##_s); }                                                 \
void set##_s(QVariant v) { socket_set(m_##_s, v); }                                               \
Q_SIGNAL void _s##Changed();

//-------------------------------------------------------------------------------------------------
class Socket : public QObject
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    friend class Node;
    friend class Graph;
    friend class Connection;

    public:
    Socket();
    Socket(Socket const&);
    Socket(wpn_socket* csock);
    Socket(Node* parent, const char* name, polarity_t polarity);

    void operator=(Socket const&);

    polarity_t polarity() { return m_polarity; }
    polarity_t m_polarity;

    public slots:
    void lookup();

    private:
    wpn_socket* c_socket    = nullptr;
    const char* m_name      = nullptr;
    Node* m_parent          = nullptr;
    qreal m_pending_value   = 0;
};

Q_DECLARE_METATYPE(Socket)

//-------------------------------------------------------------------------------------------------
class Graph : public QObject, public QQmlParserStatus
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    Q_PROPERTY    (qreal rate READ rate WRITE setRate)
    Q_PROPERTY    (int mxv READ mxv WRITE setMxv)
    Q_PROPERTY    (int mnv READ mnv WRITE setMnv)
    Q_INTERFACES  (QQmlParserStatus)

    Q_PROPERTY    (QQmlListProperty<Node> subnodes READ subnodes )
    Q_CLASSINFO   ("DefaultProperty", "subnodes" )

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

    qreal rate  () const { return m_graph.properties.rate; }
    int mxv     () const { return m_graph.properties.mxvsz; }
    int mnv     () const { return m_graph.properties.mnvsz; }

    void setRate  (qreal rate);
    void setMxv   (int);
    void setMnv   (int);

    QQmlListProperty<Node> subnodes   ( );
    Q_INVOKABLE void append_subnode   ( Node*);
    Q_INVOKABLE int nsubnodes         ( ) const;
    Q_INVOKABLE Node* subnode         ( int) const;
    Q_INVOKABLE void clear_subnodes   ( );

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
    Q_PROPERTY   (Socket source READ source WRITE setSource)
    Q_PROPERTY   (Socket dest READ dest WRITE setDest)
    Q_PROPERTY   (QVariantList routing READ routing WRITE setRouting)
    Q_PROPERTY   (qreal level READ level WRITE setLevel)
    Q_PROPERTY   (bool muted READ muted WRITE setMuted)
    Q_PROPERTY   (bool active READ active WRITE setActive)

    Q_INTERFACES (QQmlParserStatus QQmlPropertyValueSource)

    public:
    Connection();
    Connection(Connection const&);
    Connection(wpn_connection* ccon);

    virtual void classBegin() override {}
    virtual void componentComplete() override;
    virtual void setTarget(const QQmlProperty &) override;

    Socket source  () const { return Socket(c_connection->source); }
    Socket dest    () const { return Socket(c_connection->dest); }
    qreal level    () const { return c_connection->level; }
    bool muted     () const { return c_connection->muted; }
    bool active    () const { return c_connection->active; }

    QVariantList routing() const;

    wpn_socket* csource   () { return c_connection->source; }
    wpn_socket* cdest     () { return c_connection->dest; }
    wpn_routing crouting  () const { return c_connection->routing; }

    void setSource    (Socket source);
    void setDest      (Socket dest);
    void setLevel     (qreal level);
    void setMuted     (bool muted);
    void setActive    (bool active);
    void setRouting   (QVariantList routing);

    protected slots:
    void onSourceRegistered();
    void onDestRegistered();

    private:
    void cconnect();

    wpn_connection* c_connection = nullptr;
    Socket m_source;
    Socket m_dest;
    bool m_muted = false;
    bool m_active = true;
    qreal m_level = 1;
    QVariantList m_routing;
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
    friend class Connection;
    friend class Socket;

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

    bool is_registered() const { return c_node; }

    QVariantList routing() const { return m_routing; }
    static wpn_routing parseRouting(QVariantList);

    void setMuted      ( bool);
    void setActive     ( bool);
    void setLevel      ( qreal);
    void setParent     ( Node*);
    void setRouting    ( QVariantList);
    void setDispatch   ( Dispatch::Values);

    QQmlListProperty<Node> subnodes  ();
    Q_INVOKABLE void append_subnode  (Node*);
    Q_INVOKABLE int nsubnodes        () const;
    Q_INVOKABLE Node* subnode        (int) const;
    Q_INVOKABLE void clear_subnodes  ();

    static void append_subnode  (QQmlListProperty<Node>*, Node*);
    static int nsubnodes        (QQmlListProperty<Node>*);
    static Node* subnode        (QQmlListProperty<Node>*, int);
    static void clear_subnodes  (QQmlListProperty<Node>*);

    Dispatch::Values dispatch() const { return m_dispatch; }

    Q_INVOKABLE qreal db(qreal v);

    signals:
    void parentChanged();
    void registered();

    protected:
    bool m_active   = true;
    bool m_muted    = false;
    Node* m_parent  = nullptr;
    qreal m_level   = 1;
    qreal m_pending_value = 0;

    QVariantList m_routing;
    QVector<Node*> m_subnodes;    
    Dispatch::Values m_dispatch = Dispatch::Values::Upwards;
    wpn_node* c_node = nullptr;
};

//-------------------------------------------------------------------------------------------------
class JackIO : public Node
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    Q_PROPERTY  (int numInputs MEMBER m_n_inputs)
    Q_PROPERTY  (int numOutputs MEMBER m_n_outputs)
    WPN_SOCKET  (inputs, INPUT)
    WPN_SOCKET  (outputs, OUTPUT)

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
class Sinetest : public Node
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT
    Q_PROPERTY  (qreal frequency READ frequency WRITE setFrequency)
    WPN_SOCKET  (output, OUTPUT)

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
    WPN_SOCKET  (inputs, INPUT)
    WPN_SOCKET  (gain, INPUT)
    WPN_SOCKET  (outputs, OUTPUT)

    public:
    VCA();
};

#endif // QTWRAPPER2_HPP
