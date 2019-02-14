#pragma once

#include <QObject>
#include <QQmlParserStatus>
#include <QVector>
#include <QVector3D>
#include <QVariant>
#include <memory>

using signal_t = qreal;
#define lininterp(_x,_a,_b) _a+_x*(_b-_a)

//=================================================================================================
enum class polarity { output = 0, input  = 1 };
//=================================================================================================

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

    private:
    signal_t  m_rate;
    signal_t* m_data;
    signal_t  m_size;

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

        // merge operators ---------------------------------
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

        void drain();
        void normalize();

        void lookup(slice&, slice&, bool increment = false);

        private:
        slice(channel&, signal_t* begin, signal_t* end, signal_t* pos );
        slice* minsz(slice& lhs, slice const& rhs);
        channel& m_parent;
        signal_t* m_begin;
        signal_t* m_end;
        signal_t* m_pos;
        size_t m_size;

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

    void add_upsync(stream&);
    void add_dnsync(stream&);

    void draw_skip(size_t);
    void pour_skip(size_t);

    slice draw(size_t);
    void pour(size_t);

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

        void lookup(slice&, slice &, bool increment = false);

        size_t size() const;
        size_t nchannels() const;
        void drain();
        void normalize();

        private:
        slice(stream& parent, size_t begin, size_t size, size_t pos);
        stream& m_parent;
        size_t m_begin;
        size_t m_size;
        size_t m_pos;

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
    signal_t rate;
    size_t vsz;
    size_t fvsz;
};

//=================================================================================================
class connection;

class Dispatch
{
    enum Values
    {
        Parent  = 0,
        FXChain = 1
    };
};

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
Q_PROPERTY(signal _s READ get##_s WRITE set##_s NOTIFY notify##_s)                                 \
signals: void notify##_s();                                                                        \
protected: node::pin m_##_s { *this, _p, #_s, _n, _d };                                            \
                                                                                                   \
signal get##_s() {                                                                                 \
    return sgrd(m_##_s);                                                                           \
}                                                                                                  \
                                                                                                   \
void set##_s(signal v) {                                                                           \
    sgwr(m_##_s, v);                                                                               \
}
//=================================================================================================
class signal;
//=================================================================================================
class node : public QObject, public QQmlParserStatus
//=================================================================================================
{
    Q_OBJECT
    Q_PROPERTY ( Dispatch dispatch READ dispatch WRITE setDispatch )

    friend class connection;
    friend class graph;
    public: class pin;

    void add_pin(node::pin& pin);

    protected:
    //=============================================================================================
    signal sgrd(node::pin& pin);
    void sgwr(node::pin& pin, signal s);

    public:
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

        node& parent();
        polarity polarity() const;
        bool is_default() const;
        std::string label() const;
        stream& stream();

        std::vector<connection*> connections();

        private:
        node& m_parent;
        bool m_default;
        const enum polarity m_polarity;
        std::vector<connection*> m_connections;
        std::string m_label;
        class stream m_stream;
        size_t m_nchannels;
    };
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
        std::vector<pstream> streams;
        pool(std::vector<pin*>& vector, size_t pos, size_t sz);
        public: stream::slice& operator[](std::string);
    };

    protected:
    //=============================================================================================

    virtual void componentComplete() override {}
    virtual void classBegin() override {}

    virtual void rwrite     ( pool& inputs, pool& outputs, size_t sz) = 0;
    virtual void configure  ( graph_properties properties) = 0;

    graph_properties m_properties;
    QVector3D m_position {};

    public:
    Q_INVOKABLE pin& inpin();
    Q_INVOKABLE pin& inpin(QString);

    Q_INVOKABLE pin& outpin();
    Q_INVOKABLE pin& outpin(QString);

    template<typename T>
    bool connected(T& other) const;

    //---------------------------------------------------------------------------------------------
    private:
    void process     ();
    void initialize  ( graph_properties properties );

    bool m_intertwined  = false;

    std::vector<node::pin*> m_uppins;
    std::vector<node::pin*> m_dnpins;

    size_t m_stream_pos   = 0;
    signal_t m_height     = 0;
    signal_t m_width      = 0;
};

//=================================================================================================
class signal : public QObject
//=================================================================================================
{
    Q_OBJECT

    public:

    enum qtype
    {
        UNDEFINED = 0,
        REAL = 1,
        VAR  = 2,
        PIN  = 3
    };

    signal(qreal v);
    signal(QVariant v);
    signal(node::pin&);
    signal(signal const&);

    ~signal();

    bool is_real() const;
    bool is_qvariant() const;
    bool is_pin() const;

    qreal to_real() const;
    QVariant to_qvariant() const;
    node::pin& to_pin() const;

    private:
    QVariant uvar    = 0;
    qreal ureal      = 0;
    node::pin* upin  = nullptr;
    qtype m_qtype    = UNDEFINED;
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
    connection(node::pin& source, node::pin& dest, pattern pattern);

    void pull      ( size_t sz );
    void allocate  ( size_t sz );

    bool m_feedback     = false;
    bool m_muted        = false;
    bool m_active       = true;
    signal_t m_level    = 1;

    node::pin& m_source;
    node::pin& m_dest;
    pattern m_pattern;
    stream m_stream;
};

//=================================================================================================
class graph : public QObject
//=================================================================================================
{
    Q_OBJECT

    public:
    static connection&
    connect(node::pin& source, node::pin& dest,
            connection::pattern pattern);

    static connection&
    connect(node& source, node& dest,
            connection::pattern pattern);

    static connection&
    connect(node& source, node::pin& dest,
            connection::pattern pattern);

    static connection&
    connect(node::pin& source, node& dest,
            connection::pattern pattern);

    static void
    disconnect(node::pin& target);

    static void
    configure(signal_t rate = 44100.0,
              size_t vector_size = 512,
              size_t vector_feedback_size = 64);

    static stream::slice
    run(node& target);

    private:
    static QVector<connection> s_connections;
    static QVector<node*> s_nodes;
    static graph_properties s_properties;
};


//=================================================================================================
class Output : public node
//=================================================================================================
{
    WPN_OBJECT

    WPN_REGISTER_PIN  ( inputs, DEFAULT, INPUT, 0 )
    WPN_REGISTER_PIN  ( outputs, DEFAULT, OUTPUT, 0 )

    Q_PROPERTY  ( signal_t rate READ rate WRITE setRate NOTIFY rateChanged )
    Q_PROPERTY  ( int vector READ vector WRITE setVector NOTIFY vectorChanged )
    Q_PROPERTY  ( int feedback READ feedback WRITE setFeedback NOTIFY feedbackChanged )
    Q_PROPERTY  ( QString api READ api WRITE setApi NOTIFY apiChanged )
    Q_PROPERTY  ( QString device READ device WRITE setDevice NOTIFY deviceChanged )

    public:
    Output();

    private:
    signal_t m_rate;
    quint16 m_vector;
    quint16 m_feedback;
    QString m_api;
    QString m_device;
};

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
