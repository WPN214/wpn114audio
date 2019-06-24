#pragma once

#include <QObject>
#include <QQmlParserStatus>
#include <QQmlPropertyValueSource>
#include <QVector>
#include <QVariant>
#include <QDataStream>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

namespace WPN114   {
namespace Network  {

class Tree;

//---------------------------------------------------------------------------------------------
class Node : public QObject, public QQmlParserStatus, public QQmlPropertyValueSource
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_INTERFACES (QQmlParserStatus QQmlPropertyValueSource)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString name READ name WRITE set_name)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QString path READ path WRITE set_path NOTIFY pathChanged)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (QVariant value READ value WRITE set_value NOTIFY valueChanged)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool critical READ critical WRITE set_critical NOTIFY criticalChanged)

public:

    //---------------------------------------------------------------------------------------------
    Node()
    //---------------------------------------------------------------------------------------------
    {        

    }

    //---------------------------------------------------------------------------------------------
    virtual
    ~Node() override
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    virtual void
    classBegin() override {}

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    virtual void
    setTarget(const QQmlProperty& target) override
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    QVariant
    value() const { return m_value; }

    Q_SIGNAL void
    valueChanged(QVariant new_value);

    Q_SIGNAL void
    valueReceived(QVariant value);

    //---------------------------------------------------------------------------------------------
    void
    set_value(QVariant value)
    //---------------------------------------------------------------------------------------------
    {
        emit valueReceived(value);

        if (value != m_value) {
            emit valueChanged(value);
            m_value = value;
        }
    }

    //---------------------------------------------------------------------------------------------
    QString
    name() const { return m_name; }

    void
    set_name(QString name)
    {
        m_name = name;
    }

    //---------------------------------------------------------------------------------------------
    QString
    path() const { return m_path; }

    void
    set_path(QString path)
    {
        m_path = path;
    }

    //---------------------------------------------------------------------------------------------
    Node*
    parent_node() { return m_parent_node; }

    void
    set_parent_node(Node* node)
    {
        m_parent_node = node;
    }

    //---------------------------------------------------------------------------------------------
    bool
    critical() const { return m_critical; }

    void
    set_critical(bool critical)
    {
        m_critical = critical;
    }

    //---------------------------------------------------------------------------------------------
    QVector<Node*>&
    subnodes() { return m_subnodes; }

    //---------------------------------------------------------------------------------------------
    Node*
    create_subnode(QString name)
    //---------------------------------------------------------------------------------------------
    {
        Node* n = new Node;
        n->set_name(name);
        n->set_parent_node(this);
        m_subnodes.push_back(n);
        return n;
    }

    //---------------------------------------------------------------------------------------------
    void
    add_subnode(Node* subnode)
    //---------------------------------------------------------------------------------------------
    {
        m_subnodes.push_back(subnode);
    }

    //---------------------------------------------------------------------------------------------
    void
    remove_subnode(Node* subnode)
    //---------------------------------------------------------------------------------------------
    {
        m_subnodes.removeOne(subnode);
    }

    //---------------------------------------------------------------------------------------------
    Node*
    subnode(QString name, bool recursive = true)
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    Node*
    subnode(size_t index)
    //---------------------------------------------------------------------------------------------
    {
        return m_subnodes[index];
    }

    //---------------------------------------------------------------------------------------------
    bool
    has_subnode(Node* subnode) { return m_subnodes.contains(subnode); }

private:

    QString
    m_name,
    m_path;

    QVariant
    m_value;

    Node*
    m_parent_node = nullptr;

    Tree*
    m_tree = nullptr;

    QVector<Node*>
    m_subnodes;

    bool
    m_critical = false;
};

//-------------------------------------------------------------------------------------------------
struct OSCMessage
//-------------------------------------------------------------------------------------------------
{
    QString address;
    QVariant arguments;

    //---------------------------------------------------------------------------------------------
    static QByteArray
    encode(OSCMessage const& message)
    //---------------------------------------------------------------------------------------------
    {
        QByteArray data;
        QString typetag = OSCMessage::typetag(message.arguments).prepend(',');
        data.append(message.address);

        auto pads = 4-(message.address.count()%4);
        while (pads--) data.append((char)0);

        data.append(typetag);
        pads = 4-(typetag.count()%4);

        while (pads--) data.append((char)0);
        append(data, message.arguments);

        return data;
    }

    //---------------------------------------------------------------------------------------------
    template<typename _Valuetype> static void
    from_stream(QVariantList& arglst, QDataStream& stream)
    //---------------------------------------------------------------------------------------------
    {
        _Valuetype target_value;
        stream >> target_value;
        arglst << target_value;
    }

    //---------------------------------------------------------------------------------------------
    template<> static void
    from_stream<QString>(QVariantList& arglist, QDataStream& stream)
    //---------------------------------------------------------------------------------------------
    {
        uint8_t byte;
        QByteArray data;

        stream >> byte;
        while (byte) { data.append(byte); stream >> byte; }

        uint8_t padding = 4-(data.count()%4);
        stream.skipRawData(padding-1);
        arglist << QString::fromUtf8(data);
    }

    //---------------------------------------------------------------------------------------------
    static OSCMessage
    decode(QByteArray const& data)
    //---------------------------------------------------------------------------------------------
    {
        OSCMessage message;
        QDataStream stream;
        QString address, typetag;
        QVariantList arguments;

        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        auto split = data.split(',');
        message.address = split[0];
        typetag = split[1].split('\0')[0];

        uint8_t adpads = 4-(message.address.count()%4);
        uint8_t ttpads = 4-(typetag.count()%4);
        uint32_t total = message.address.count()+adpads+typetag.count()+ttpads+1;

        stream.skipRawData(total);

        for (const auto& c : typetag) {
            switch(c.toLatin1()) {
                case 'i': OSCMessage::from_stream<int>(arguments, stream); break;
                case 'f': OSCMessage::from_stream<float>(arguments, stream); break;
                case 's': OSCMessage::from_stream<QString>(arguments, stream); break;
                case 'T': arguments << true; break;
                case 'F': arguments << false; break;
            }}

        message.address = address;

        switch(arguments.count()) {
            case 0: return message;
            case 1: message.arguments = arguments[0]; break;
            default: message.arguments = arguments;
        }

        return message;
    }

    //---------------------------------------------------------------------------------------------
    static QString
    typetag(QVariant const& argument)
    //---------------------------------------------------------------------------------------------
    {
        switch (argument.type()) {
            case QVariant::Bool: return argument.value<bool>() ? "T" : "F";
            case QVariant::Int: return "i";
            case QVariant::Double: return "f";
            case QVariant::String: return "s";
            case QVariant::Vector2D: return "ff";
            case QVariant::Vector3D: return "fff";
            case QVariant::Vector4D: return "ffff";
        }

        if (argument.type() == QVariant::List ||
            strcmp(argument.typeName(), "QJSValue") == 0)
        {
            // if argument is QVariantList or QJSValue
            // recursively parse arguments
            QString tag;
            for (const auto& sub: argument.toList())
                 tag.append(OSCMessage::typetag(sub));
            return tag;
        }

        return "N";
    }

    //---------------------------------------------------------------------------------------------
    static void
    append(QByteArray& data, QVariant const& argument)
    //---------------------------------------------------------------------------------------------
    {
        QDataStream stream(&data, QIODevice::ReadWrite);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream.skipRawData(data.size());

        switch(argument.type())
        {
        case QVariant::Int: stream << argument.value<int>(); break;
        case QVariant::Double: stream << argument.value<float>(); break;

        case QVariant::Vector2D: stream << argument.value<QVector2D>(); break;
        case QVariant::Vector3D: stream << argument.value<QVector3D>(); break;
        case QVariant::Vector4D: stream << argument.value<QVector4D>(); break;

        case QVariant::String: {
            QByteArray str = argument.toString().toUtf8();
            auto pads = 4-(str.count()%4);
            while (pads--) str.append('\0');
            data.append(str);
            return;
        }
        case QVariant::List: {
            for (const auto& sub : argument.value<QVariantList>())
                OSCMessage::append(data, sub);
        }
        }
    }
};

//---------------------------------------------------------------------------------------------
class Tree : public QObject
//---------------------------------------------------------------------------------------------
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool singleton READ singleton WRITE set_singleton)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (bool exposed READ exposed WRITE set_exposed)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (int tcp READ tcp WRITE set_tcp)

    Q_PROPERTY (int udp READ udp WRITE set_udp)

    //---------------------------------------------------------------------------------------------
    Q_PROPERTY (Node* root READ root)

    //---------------------------------------------------------------------------------------------
    QString
    m_name = "wpn114";

    Node
    m_root;

    bool
    m_exposed = false;

    static Tree*
    s_singleton;

public:

    //---------------------------------------------------------------------------------------------
    Tree()
    {
        m_root.set_path("/");
    }

    //---------------------------------------------------------------------------------------------
    static Tree&
    instance() { return *s_singleton; }

    //---------------------------------------------------------------------------------------------
    Node*
    root() { return &m_root; }

    //---------------------------------------------------------------------------------------------
    bool
    singleton() const  { return s_singleton == this; }

    //---------------------------------------------------------------------------------------------
    void
    set_singleton(bool singleton)
    //---------------------------------------------------------------------------------------------
    {
        if (singleton)
            s_singleton = this;
    }

    //---------------------------------------------------------------------------------------------
    bool
    exposed() const { return m_exposed; }

    void
    set_exposed(bool exposed)
    {
        m_exposed = exposed;
    }

    //---------------------------------------------------------------------------------------------
    uint16_t
    tcp() const
    {
        return 0;
    }

    //---------------------------------------------------------------------------------------------
    uint16_t
    udp() const
    {
        return 0;
    }

    //---------------------------------------------------------------------------------------------
    Node*
    find_node(QString path)
    //---------------------------------------------------------------------------------------------
    {
        return m_root.subnode(path, true);
    }

    //---------------------------------------------------------------------------------------------
    Node*
    find_or_create_node(QString path)
    //---------------------------------------------------------------------------------------------
    {
        if (auto node = m_root.subnode(path, true))
             return node;
        else return m_root.create_subnode(path);
    }
};


}
}
