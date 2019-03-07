#ifndef QTWRAPPER_HPP
#define QTWRAPPER_HPP

#include "graph.h"
#include <QObject>
#include <QQmlParserStatus>
#include <QQmlPropertyValueSource>
#include <QQmlProperty>
#include <QQmlListProperty>
#include <QVariant>
#include <QThread>
#include <QVector>
#include <external/rtaudio/RtAudio.h>

//-------------------------------------------------------------------------------------------------
#define WPN114_OBJECT(_n)                                                                          \
Q_OBJECT                                                                                           \
public:                                                                                            \
virtual void rwrite(pool114& inputs, pool114& outputs, vector_t sz) override;                      \
virtual void configure(wpn_graph_properties properties) override;                                  \
virtual QString nreference() const override { return #_n; }

//-------------------------------------------------------------------------------------------------
#define WPN114_REGISTER_PIN(_s, _d, _p, _n)                                                        \
private: Q_PROPERTY(QVariant _s READ get##_s WRITE set##_s NOTIFY _s##Changed)                     \
protected: Socket m_##_s { *this, _p, #_s, _n, _d };                                               \
                                                                                                   \
QVariant get##_s() { return sgrd(m_##_s); }                                                        \
void set##_s(QVariant v) { sgwr(m_##_s, v); }                                                      \
Q_SIGNAL void _s##Changed();

#define DEFAULT true
#define NONDEFAULT false

//-------------------------------------------------------------------------------------------------
#ifdef WPN_EXTERN_DEF_DOUBLE_PRECISION
    #define WPN114_RT_PRECISION RTAUDIO_FLOAT64
#elif defined(WPN_EXTERN_DEF_SINGLE_PRECISION)
    #define WPN114_RT_PRECISION RTAUDIO_FLOAT32
#else
    #define WPN114_RT_PRECISION RTAUDIO_FLOAT32
#endif

//-------------------------------------------------------------------------------------------------
class Node;
//-------------------------------------------------------------------------------------------------
class Socket
//-------------------------------------------------------------------------------------------------
{
    public:
    Socket(Node& parent, polarity_t p, QString l, nchn_t n, bool df);        

    Node& parent;
    const char* label;
    wpn_socket* csocket = nullptr;
};

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
class Graph : public QObject
//-------------------------------------------------------------------------------------------------
{
    Q_OBJECT

    public:

    static wpn_graph&
    instance();

    static wpn_node*
    lookup(Node&);

    static wpn_node*
    registerNode(Node&);

    static wpn_connection&
    connect(Socket& source, Socket& dest);

    static wpn_connection&
    connect(Node& source, Node& dest);

    static wpn_connection&
    connect(Node& source, Socket& dest);

    static wpn_connection&
    connect(Socket& source, Node& dest);

    static void
    disconnect(Socket&);

    static void
    initialize();

    static void
    configure(sample_t rate, vector_t vec, vector_t fb);

    private:
    static wpn_graph m_graph;
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

    Q_CLASSINFO   ( "DefaultProperty", "subnodes" )
    Q_INTERFACES  ( QQmlParserStatus QQmlPropertyValueSource )

    friend class Graph;

    public:
    Node();

    virtual void setTarget(QQmlProperty const&) final override;
    virtual void classBegin() final override {}
    virtual void componentComplete() override;

    virtual void configure(wpn_graph_properties properties) = 0;
    virtual void rwrite(pool114& inputs, pool114& outputs, vector_t sz) = 0;
    virtual QString nreference() const = 0;

    void addSocket(polarity_t p, QString l, nchn_t n, bool df);    

    bool muted      () const { return m_muted; }
    bool active     () const { return m_active; }
    qreal level     () const { return m_level; }
    Node* parent    () const { return m_parent; }

    Dispatch::Values dispatch() const { return m_dispatch; }

    void setMuted   (bool);
    void setActive  (bool);
    void setLevel   (qreal);
    void setParent  (Node*);

    void setDispatch(Dispatch::Values);

    QQmlListProperty<Node> subnodes();
    Q_INVOKABLE void append_subnode(Node*);
    Q_INVOKABLE int nsubnodes() const;
    Q_INVOKABLE Node* subnode(int) const;
    Q_INVOKABLE void clear_subnodes();

    static void append_subnode  ( QQmlListProperty<Node>*, Node*);
    static int nsubnodes        ( QQmlListProperty<Node>*);
    static Node* subnode        ( QQmlListProperty<Node>*, int);
    static void clear_subnodes  ( QQmlListProperty<Node>*);

    Q_INVOKABLE QVariant connection(QVariant var, qreal lvl, QVariant map);
    Q_INVOKABLE qreal db(qreal v);
    Node& chainout();

    signals:
    void parentChanged();

    protected:
    QVariant sgrd(Socket& s);
    void sgwr(Socket& s, QVariant v);

    private:
    bool m_active   = true;
    bool m_muted    = false;
    Node* m_parent  = nullptr;
    qreal m_level   = 1;

    Dispatch::Values m_dispatch;
    QVector<Node*> m_subnodes;

    wpn_node* cnode = nullptr;
};

//-------------------------------------------------------------------------------------------------

inline void node_cfg(wpn_graph_properties p, void* udata)
{
    static_cast<Node*>(udata)->configure(p);
}

inline void node_prc(pool114* i, pool114* o, void* u, vector_t sz)
{
    static_cast<Node*>(u)->rwrite(*i, *o, sz);
}

//-------------------------------------------------------------------------------------------------
class Audiostream;
//-------------------------------------------------------------------------------------------------
class Output : public Node
//-------------------------------------------------------------------------------------------------
{
    WPN114_OBJECT (Output)

    Q_PROPERTY  ( qreal rate READ rate WRITE setRate NOTIFY rateChanged )
    Q_PROPERTY  ( int nchannels READ nchannels WRITE setNchannels NOTIFY nchannelsChanged )
    Q_PROPERTY  ( int offset READ offset WRITE setOffset NOTIFY offsetChanged )
    Q_PROPERTY  ( int vector READ vector WRITE setVector NOTIFY vectorChanged )
    Q_PROPERTY  ( int feedback READ feedback WRITE setFeedback NOTIFY feedbackChanged )
    Q_PROPERTY  ( QString api READ api WRITE setApi NOTIFY apiChanged )
    Q_PROPERTY  ( QString device READ device WRITE setDevice NOTIFY deviceChanged )

    WPN114_REGISTER_PIN  ( inputs, DEFAULT, INPUT, 0 )
    WPN114_REGISTER_PIN  ( outputs, DEFAULT, OUTPUT, 0 )

    public:
    Output();
    ~Output() override;

    virtual void componentComplete() override;

    sample_t rate       () const { return m_rate; }
    quint16 nchannels   () const { return m_nchannels; }
    quint16 offset      () const { return m_offset; }
    quint16 vector      () const { return m_vector; }
    quint16 feedback    () const { return m_feedback; }
    QString api         () const { return m_api; }
    QString device      () const { return m_device; }

    void setRate        ( sample_t rate );
    void setNchannels   ( quint16 nchannels );
    void setOffset      ( quint16 offset );
    void setVector      ( quint16 vector );
    void setFeedback    ( quint16 feedback );
    void setApi         ( QString api );
    void setDevice      ( QString device );

    Q_INVOKABLE void start    ();
    Q_INVOKABLE void stop     ();
    Q_INVOKABLE void restart  ();

    signals:
    void rateChanged        ();
    void nchannelsChanged   ();
    void offsetChanged      ();
    void vectorChanged      ();
    void feedbackChanged    ();
    void apiChanged         ();
    void deviceChanged      ();

    void configureStream    ();
    void startStream        ();
    void stopStream         ();
    void restartStream      ();
    void exitStream         ();

    private:
    sample_t m_rate        = 44100;
    quint16 m_nchannels    = 2;
    quint16 m_offset       = 0;
    quint16 m_vector       = 512;
    quint16 m_feedback     = 64;
    QString m_api;
    QString m_device;

    QThread m_audiothread;
    std::unique_ptr<Audiostream> m_stream;
};

//------------------------------------------------------------------------------------------------
class Audiostream : public QObject
//------------------------------------------------------------------------------------------------=
{
    Q_OBJECT

    public:
    Audiostream ( Output& out,
                  RtAudio::StreamParameters parameters,
                  RtAudio::DeviceInfo info,
                  RtAudio::StreamOptions options);

    public slots:
    void preconfigure   ();
    void start          ();
    void stop           ();
    void restart        ();
    void exit           ();

    private:
    Output& m_outmodule;
    std::unique_ptr<RtAudio> m_stream;
    RtAudioFormat m_format;
    RtAudio::DeviceInfo m_device_info;
    RtAudio::StreamParameters m_parameters;
    RtAudio::StreamOptions m_options;
    quint32 m_vector;
};

// the main audio callback
int rwrite(void* out, void* in, unsigned int nframes,
           double time, RtAudioStreamStatus status,
           void* udata);

//-------------------------------------------------------------------------------------------------
sstream(sstream16384, 16384, sample_t);
class Sinetest : public Node
//-------------------------------------------------------------------------------------------------
{
    WPN114_OBJECT  ( Sinetest )

    WPN114_REGISTER_PIN ( inputs, DEFAULT, INPUT, 0)
    WPN114_REGISTER_PIN ( outputs, DEFAULT, OUTPUT, 0)

    public:
    Sinetest();

    private:
    sstream16384 m_wtable;

};

#endif // QTWRAPPER_HPP
