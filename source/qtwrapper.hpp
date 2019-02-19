#pragma once

#include <QObject>
#include <QQmlParserStatus>
#include <QQmlListProperty>
#include <QQmlProperty>
#include <QQmlPropertyValueSource>
#include <QVector>
#include <QVector3D>
#include <QVariant>
#include <QThread>
#include <memory>
#include <external/rtaudio/RtAudio.h>
//=================================================================================================
#define WPN_TODO
#define WPN_REFACTOR
#define WPN_OPTIMIZE
#define WPN_DEPRECATED
#define WPN_UNSAFE
#define WPN_EXAMINE
#define WPN_REVISE
#define WPN_OK
//=================================================================================================
#ifdef WPN_SINGLE_PRECISION
using signal_t = float;
#define WPN_RT_PRECISION RTAUDIO_FLOAT32
#endif

#ifdef WPN_DOUBLE_PRECISION
using signal_t = qreal;
#define WPN_RT_PRECISION RTAUDIO_FLOAT64
#endif
//=================================================================================================
#define DEFAULT true
#define NONDEFAULT false
#define INPUT  polarity::input
#define OUTPUT polarity::output
//=================================================================================================
#define WPN_OBJECT                                                                                 \
Q_OBJECT                                                                                           \
protected:                                                                                         \
virtual void rwrite(node::pool& inputs, node::pool& outputs, size_t sz) override;                  \
virtual void configure(graph_properties properties) override;
//=================================================================================================
#define WPN_REGISTER_PIN(_s, _d, _p, _n)                                                           \
Q_PROPERTY(SVariant _s READ get##_s WRITE set##_s NOTIFY notify##_s)                               \
signals: void notify##_s();                                                                        \
protected: pin m_##_s { *this, _p, #_s, _n, _d };                                                  \
                                                                                                   \
SVariant get##_s() {                                                                               \
    return sgrd(m_##_s);                                                                           \
}                                                                                                  \
                                                                                                   \
void set##_s(SVariant v) {                                                                         \
    sgwr(m_##_s, v);                                                                               \
}
//=================================================================================================
#define lininterp(_x,_a,_b) _a+_x*(_b-_a)
#define atodb(_s) log10(_s)*20
#define dbtoa(_s) pow(10, _s*.05)
#define wrap(_a, _b) if( _a >=_b ) _a -= _b;
//=================================================================================================
enum class polarity { output = 0, input  = 1 };
//=================================================================================================
class channel
//=================================================================================================
{
    friend class slice;

    public:
    class slice;

    channel();
    channel(size_t size, signal_t fill = 0);

    slice operator()(size_t begin = 0, size_t sz = 0, size_t pos = 0);    
    operator slice();

    private:
    signal_t  m_rate = 0;
    signal_t* m_data = nullptr;
    size_t m_size = 0;

    public:
    //=============================================================================================
    class slice
    //=============================================================================================
    {
        friend class channel;

        public:
        slice(channel& parent);
        slice(slice const&);

        class iterator;
        iterator begin();
        iterator end();

        signal_t& operator[](size_t);
        size_t size() const;

        // stream operators --------------------------------
        slice& operator<<(slice const&);
        slice& operator>>(slice &);
        slice& operator<<(signal_t const);
        slice& operator>>(signal_t&);
        slice& operator<<=(slice &);
        slice& operator>>=(slice &);
        slice& operator<<=(signal_t const);

        // arithm. -----------------------------------------
        slice& operator+=(slice const&);
        slice& operator-=(slice const&);
        slice& operator*=(slice const&);
        slice& operator/=(slice const&);

        slice& operator+=(const signal_t);
        slice& operator-=(const signal_t);
        slice& operator*=(const signal_t);
        slice& operator/=(const signal_t);

        // signal-related -------------------
        signal_t min();
        signal_t max();
        signal_t rms();

        // transform  -------------------------------------------
        void lookup     ( slice&, slice&, bool increment = false );
        void drain      ();
        void normalize  ();

        // cast ---------------------
        operator signal_t*();

        private:
        slice(channel&, signal_t* begin, signal_t* end, signal_t* pos );

        channel& m_parent;
        signal_t* m_begin = nullptr;
        signal_t* m_end = nullptr;
        signal_t* m_pos = nullptr;
        size_t m_size = 0;

        public:
        //----------------------------------------------------------------------------------
        class iterator :
        public std::iterator<std::input_iterator_tag, signal_t>
        //----------------------------------------------------------------------------------
        {
            public:
            iterator(signal_t* data);
            iterator& operator++();
            signal_t& operator*();
            bool operator==(const iterator& s);
            bool operator!=(const iterator& s);

            private:
            signal_t* m_data;
        };
    };
};

// ========================================================================================
class stream
// a vector of channels
// ========================================================================================
{
    public:
    stream();
    stream(size_t nchannels);
    stream(size_t nchannels, size_t channel_size, signal_t fill = 0);

    void append(stream&);
    void append(channel&);

    void allocate(size_t nchannels, size_t size);

    ~stream();

    class slice;
    operator slice();
    slice operator()(size_t begin = 0, size_t size = 0, size_t pos = 0);
    channel& operator[](size_t);

    // -----------------------------------------------
    // syncs
    //
    // -----------------------------------------------
    struct sync
    {
        stream* _stream;
        size_t size;
        size_t pos;
    };

    private:
    void add_sync(sync& target, stream& s);
    stream::slice synchronize(sync& target, size_t sz);
    void synchronize_skip(sync& target, size_t sz);

    public:
    void add_upsync(stream&);
    void add_dnsync(stream&);

    stream::slice sync_draw(size_t);
    void sync_pour(size_t);

    void upsync_skip(size_t);
    void dnsync_skip(size_t);

    size_t size() const;
    size_t nchannels() const;

    protected:
    std::vector<channel*> m_channels;
    size_t m_size;
    size_t m_nchannels;
    sync m_upsync;
    sync m_dnsync;

    public:
    // ======================================================================================
    class slice
    // a temporary reference to a stream, or to a fragment of a stream
    // only accessors are allowed to read/write from a stream
    // ======================================================================================
    {
        friend class stream;
        public:
        class iterator;
        iterator begin();
        iterator end();
        channel::slice operator[](size_t);
        operator bool();

        // arithm. -------------------------
        slice& operator+=(signal_t);
        slice& operator-=(signal_t);
        slice& operator*=(signal_t);
        slice& operator/=(signal_t);

        slice& operator+=(slice const&);
        slice& operator-=(slice const&);
        slice& operator*=(slice const&);
        slice& operator/=(slice const&);

        slice& operator+=(channel::slice const&);
        slice& operator-=(channel::slice const&);
        slice& operator*=(channel::slice const&);
        slice& operator/=(channel::slice const&);

        slice& operator<<(slice const&);
        slice& operator>>(slice&);
        slice& operator<<=(slice&);
        slice& operator>>=(slice&);
        slice& operator<<=(signal_t const);

        // others ------------------------------------------------
        void lookup     (slice&, slice &, bool increment = false);
        void drain      ();
        void normalize  ();

        // properties ---------------
        size_t size         () const;
        size_t nchannels    () const;

        // cast ---------------------
        // note: these two are equivalent
        operator signal_t*();
        signal_t* interleaved();
        void interleaved(signal_t*);

        private:
        slice(stream& parent, size_t begin, size_t size, size_t pos);
        stream& m_parent;
        size_t m_begin = 0;
        size_t m_size = 0;
        size_t m_pos = 0;

        std::vector<channel::slice> m_cslices;

        public:
        //-----------------------------------------------------------------------------------
        class iterator :
        public std::iterator<std::input_iterator_tag, signal_t>
        //-----------------------------------------------------------------------------------
        {
            public:
            iterator(std::vector<channel::slice>::iterator data);
            iterator& operator++();
            channel::slice operator*();

            bool operator==(const iterator& s);
            bool operator!=(const iterator& s);

            private:
            std::vector<channel::slice>::iterator m_data;
        };
        //-----------------------------------------------------------------------------------
    };
};

//=================================================================================================
struct graph_properties
{
    signal_t rate = 44100;
    size_t vsz = 512;
    size_t fvsz = 64;
};

//=================================================================================================
class Dispatch : public QObject
{
    Q_OBJECT

    public:
    enum Values
    {
        Upwards    = 0,
        Downwards  = 1
    };

    Q_ENUM ( Values )
};

//=================================================================================================
class SVariant;
class node;
class connection;
//=============================================================================================
class pin : public QObject
//=============================================================================================
{
    Q_OBJECT

    friend class node;
    friend class connection;
    friend class graph;

    public:
    pin ( node& parent,
          polarity p,
          std::string label,
          size_t nchannels = 1,
          bool def = false );

    template<typename T>
    bool connected(T&) const;
    bool connected() const;

    void allocate(size_t sz);
    void add_connection(connection& con);
    void remove_connection(connection& con);

    node& get_parent        ();
    polarity get_polarity   () const;
    bool is_default         () const;
    std::string get_label   () const;
    stream& get_stream      ();

    std::vector<std::shared_ptr<connection>>
    connections();

    private:
    node& m_parent;
    bool m_default;
    const polarity m_polarity;

    std::vector<std::shared_ptr<connection>>
    m_connections;

    std::string m_label;
    stream m_stream;
    size_t m_nchannels = 0;
};

//=================================================================================================
class connection : public QObject
//=================================================================================================
{
    friend class node;
    friend class graph;

    Q_OBJECT

    public:
    enum class pattern
    {
        Append  = 0,
        Merge   = 1,
        Split   = 2
    };

    Q_ENUM(pattern)

    connection(pin& source, pin& dest, pattern pattern);
    connection(connection const&);
    connection(connection&&);

    connection& operator=(connection const&);
    connection& operator=(connection&&);

    bool operator==(connection const&);

    void mute    ();
    void unmute  ();
    void open    ();
    void close   ();

    connection& operator+=(signal_t);
    connection& operator-=(signal_t);
    connection& operator*=(signal_t);
    connection& operator/=(signal_t);
    connection& operator=(signal_t);

    template<typename T>
    bool is_source(T&) const;

    template<typename T>
    bool is_dest(T&) const;

    private:
    void pull      ( size_t sz );
    void allocate  ( size_t sz );

    bool m_feedback     = false;
    bool m_muted        = false;
    bool m_active       = true;
    signal_t m_level    = 1;

    pin& m_source;
    pin& m_dest;
    pattern m_pattern;
    stream m_stream;
};

//=================================================================================================
class node : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//=================================================================================================
{
    Q_OBJECT
    Q_PROPERTY   ( Dispatch::Values dispatch READ dispatch WRITE setDispatch )
    Q_PROPERTY   ( bool active READ active WRITE setActive NOTIFY activeChanged )
    Q_PROPERTY   ( bool muted READ muted WRITE setMuted NOTIFY mutedChanged )
    Q_PROPERTY   ( qreal level READ level WRITE setLevel NOTIFY levelChanged )
    Q_PROPERTY   ( QQmlListProperty<node> subnodes READ subnodes )
    Q_PROPERTY   ( node* parent READ parent WRITE setParent NOTIFY parentChanged )

    Q_CLASSINFO  ( "DefaultProperty", "subnodes" )
    Q_INTERFACES ( QQmlParserStatus QQmlPropertyValueSource )

    friend class connection;
    friend class graph;
    friend class pin;

    friend int
    rwrite (void*, void*, unsigned int, double,
           RtAudioStreamStatus, void*);

    protected:
    //=============================================================================================
    void        add_pin(pin& pin);
    void        sgwr(pin& pin, SVariant s);
    SVariant    sgrd(pin& pin);

    public:
    //=============================================================================================
    struct pstream
    //=============================================================================================
    {
        std::string const& label;
        stream::slice slice;
    };
    //=============================================================================================
    class pool
    //=============================================================================================
    {
        friend class node;
        pool(std::vector<pin*>& vector, size_t pos, size_t sz);
        std::vector<pstream> streams;

        public: stream::slice& operator[](std::string);
    };

    protected:
    //=============================================================================================
    virtual void setTarget(QQmlProperty const&) override;
    virtual void componentComplete() override;
    virtual void classBegin() override {}
    //=============================================================================================
    virtual void rwrite     ( pool& inputs, pool& outputs, size_t sz) = 0;
    virtual void configure  ( graph_properties properties) = 0;
    //=============================================================================================

    graph_properties m_properties;
    QVector3D m_position {};   

    signals:
    void activeChanged  ();
    void mutedChanged   ();
    void levelChanged   ();
    void parentChanged  ();

    public:
    // --------------------------------------------------------------------------------------------
    pin& inpin();
    pin& inpin(QString);
    pin& outpin();
    pin& outpin(QString);
    // --------------------------------------------------------------------------------------------
    Q_INVOKABLE SVariant connection(SVariant v, qreal level, QVariant map);
    Q_INVOKABLE qreal db(qreal a);
    // --------------------------------------------------------------------------------------------
    QQmlListProperty<node> subnodes();

    Q_INVOKABLE void append_subnode(node*);
    Q_INVOKABLE int nsubnodes() const;
    Q_INVOKABLE node* subnode(int) const;
    Q_INVOKABLE void clear_subnodes();

    static void append_subnode  ( QQmlListProperty<node>*, node*);
    static int nsubnodes        ( QQmlListProperty<node>*);
    static node* subnode        ( QQmlListProperty<node>*, int);
    static void clear_subnodes  ( QQmlListProperty<node>*);

    void setDispatch(Dispatch::Values);
    Dispatch::Values dispatch() const;

    void setParent(node*);
    node* parent() const;
    node& chainout();

    // --------------------------------------------------------------------------------------------

    bool active() const {
        return m_active;
    }

    bool muted() const {
        return m_muted;
    }

    qreal level() const {
        return m_level;
    }

    void setActive(bool active) {
        m_active = active;
    }

    void setMuted(bool muted) {
        m_muted = muted;
    }

    void setLevel(qreal level) {
        m_level = level;
    }

    // --------------------------------------------------------------------------------------------
    template<typename T>
    bool connected(T& other);
    //---------------------------------------------------------------------------------------------

    private:
    void initialize(graph_properties properties);
    void process();

    stream::slice collect(polarity);

    bool m_intertwined  = false;
    bool m_active = true;
    bool m_muted = false;
    qreal m_level = 1;

    node* m_parent = nullptr;
    std::vector<pin*> m_uppins;
    std::vector<pin*> m_dnpins;
    std::vector<node*> m_subnodes;

    std::vector<pin*>& pvector(polarity);

    Dispatch::Values m_dispatch
        = Dispatch::Values::Upwards;

    connection::pattern m_connection_pattern
        = connection::pattern::Merge;

    size_t m_stream_pos   = 0;
    signal_t m_height     = 0;
    signal_t m_width      = 0;
};

//=================================================================================================
class SVariant : public QObject
// convenience class allowing flexible qml-bindings
//=================================================================================================
{
    Q_OBJECT

    public:
    enum vtype
    {
        Undefined   = 0,
        Real        = 1,
        Variant     = 2,
        Pin         = 3,
        Connection  = 4
    };

    SVariant(pin&);
    SVariant(SVariant const&);
    SVariant(SVariant&&);

    SVariant(qreal v);
    SVariant(QVariant v);

    SVariant& operator=(SVariant const&);
    SVariant& operator=(SVariant&&);

    ~SVariant();

    vtype type() const;
    pin& source();
    template<typename T> T get();

    private:

    union u_value
    {
        QVariant u_var;
        qreal u_real;
        pin*  u_pin;

        u_value()  { memset(this, 0, sizeof(u_value)); }
        ~u_value() { }
        u_value& operator=(u_value const&);

    } value;

    pin* m_source = nullptr;
    vtype m_vtype = Undefined;
};

//=================================================================================================
class graph : public QObject
//=================================================================================================
{
    Q_OBJECT

    public:
    static connection&
    connect(pin& source, pin& dest,
            connection::pattern pattern = connection::pattern::Merge);

    static connection&
    connect(node& source, node& dest,
            connection::pattern pattern = connection::pattern::Merge);

    static connection&
    connect(node& source, pin& dest,
            connection::pattern pattern = connection::pattern::Merge);

    static connection&
    connect(pin& source, node& dest,
            connection::pattern pattern = connection::pattern::Merge);

    static void
    disconnect(pin& target);

    static void
    disconnect(pin& source, pin& dest);

    static void
    configure(signal_t rate = 44100.0,
              size_t vector_size = 512,
              size_t vector_feedback_size = 64);

    static void initialize();

    static stream::slice
    run(node& target);

    private:
    static std::vector<connection> s_connections;

    // TODO, emplace_back nodes
    // graph has ownership over them
    static std::vector<node*> s_nodes;
    static graph_properties s_properties;
};

class Audiostream;
//=================================================================================================
class Output : public node
//=================================================================================================
{
    WPN_OBJECT

    WPN_REGISTER_PIN  ( inputs, DEFAULT, INPUT, 0 )
    WPN_REGISTER_PIN  ( outputs, DEFAULT, OUTPUT, 0 )

    Q_PROPERTY  ( signal_t rate READ rate WRITE setRate NOTIFY rateChanged )
    Q_PROPERTY  ( int nchannels READ nchannels WRITE setNchannels NOTIFY nchannelsChanged )
    Q_PROPERTY  ( int offset READ offset WRITE setOffset NOTIFY offsetChanged )
    Q_PROPERTY  ( int vector READ vector WRITE setVector NOTIFY vectorChanged )
    Q_PROPERTY  ( int feedback READ feedback WRITE setFeedback NOTIFY feedbackChanged )
    Q_PROPERTY  ( QString api READ api WRITE setApi NOTIFY apiChanged )
    Q_PROPERTY  ( QString device READ device WRITE setDevice NOTIFY deviceChanged )

    public:
    Output();

    virtual void componentComplete() override;

    signal_t rate       () const { return m_rate; }
    quint16 nchannels   () const { return m_nchannels; }
    quint16 offset      () const { return m_offset; }
    quint16 vector      () const { return m_vector; }
    quint16 feedback    () const { return m_feedback; }
    QString api         () const { return m_api; }
    QString device      () const { return m_device; }

    void setRate        ( signal_t rate);
    void setNchannels   ( quint16 nchannels);
    void setOffset      ( quint16 offset);
    void setVector      ( quint16 vector);
    void setFeedback    ( quint16 feedback);
    void setApi         ( QString api);
    void setDevice      ( QString device);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void restart();

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
    signal_t m_rate      = 44100;
    quint16 m_nchannels  = 2;
    quint16 m_offset     = 0;
    quint16 m_vector     = 512;
    quint16 m_feedback   = 64;
    QString m_api;
    QString m_device;

    std::unique_ptr<Audiostream> m_stream;
    QThread m_audiothread;
};
//=================================================================================================
class Audiostream : public QObject
//=================================================================================================
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

//=================================================================================================
class Sinetest : public node
//=================================================================================================
{
    WPN_OBJECT
    WPN_REGISTER_PIN  ( frequency, DEFAULT, INPUT, 0 )
    WPN_REGISTER_PIN  ( output, DEFAULT, OUTPUT, 0 )

    public:
    Sinetest();

    private:
    stream m_wavetable;
};

//=================================================================================================
class VCA : public node
//=================================================================================================
{
    WPN_OBJECT
    WPN_REGISTER_PIN ( gain, NONDEFAULT, INPUT, 1)
    WPN_REGISTER_PIN ( input, DEFAULT, INPUT, 1 )
    WPN_REGISTER_PIN ( output, DEFAULT, OUTPUT, 1)

    public:
    VCA();
};

//=================================================================================================
class Pinktest : public node
//=================================================================================================
{
    WPN_OBJECT
    WPN_REGISTER_PIN ( output, DEFAULT, OUTPUT, 0 )

    public:
    Pinktest();
};
