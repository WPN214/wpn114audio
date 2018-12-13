#include "audio.hpp"
#include <QtDebug>
#include <qendian.h>
#include <cmath>
#include <memory>
#include <QtGlobal>

static const QStringList g_ignore =
{
    "parentStream", "subnodes", "exposeDevice", "objectName", "exposePath",
    "numInputs", "numOutputs", "parentChannels"
};

static const QStringList g_stream =
{
    "active", "mute", "level", "dBlevel"
};

StreamNode::StreamNode() : m_level(1.0), m_db_level(0.0),
    m_num_inputs(0), m_num_outputs(0), m_max_outputs(0), m_parent_channels(0),
    m_mute(false), m_active(true),
    m_in(nullptr), m_out(nullptr), m_parent_stream(nullptr)
{
    m_stream_properties.block_size      = 0;
    m_stream_properties.sample_rate     = 0;
}

StreamNode::~StreamNode()
{
    StreamNode::deleteBuffer( m_in, m_num_inputs, m_stream_properties.block_size );
    StreamNode::deleteBuffer( m_out, m_num_outputs, m_stream_properties.block_size );

    for ( const auto& subnode : m_subnodes )
          if ( !subnode->qml() ) delete subnode;
}

void StreamNode::classBegin()
{
    m_qml = true;
}

void StreamNode::componentComplete()
{

}

void StreamNode::setNumInputs(uint16_t num_inputs)
{
    if ( m_num_inputs != num_inputs )
    {
        m_num_inputs = num_inputs;
        emit numInputsChanged();
    }
}

void StreamNode::setNumOutputs(uint16_t num_outputs)
{    
    if ( m_num_outputs != num_outputs )
    {
        m_num_outputs = num_outputs;

        QVariantList list;

        for ( quint16 ch = 0; ch < num_outputs; ++ch)
        {
           list.push_back(QVariant(ch));
            m_parent_channels = list;
        }

        if ( m_subnodes.isEmpty() ) setMaxOutputs(num_outputs);
        emit numOutputsChanged();
    }
}

void StreamNode::setMaxOutputs(uint16_t max_outputs)
{
    m_max_outputs = max_outputs;

    QVariantList list;
    for ( quint16 i = 0; i < m_max_outputs; ++i ) list << i;

    m_parent_channels = list;
}

void StreamNode::setMute(bool mute)
{
    if ( mute != m_mute )
    {
        m_mute = mute;
        emit muteChanged();
    }
}

void StreamNode::setActive(bool active)
{
    if ( active != m_active )
    {
        m_active = active;
        emit activeChanged();
    }
}

void StreamNode::setLevel(qreal level)
{
    if ( level != m_level )
    {
        m_level = level;
        m_db_level = std::log10(level)*20.0;

        emit levelChanged   ( );
        emit dBlevelChanged ( );
    }
}

void StreamNode::setLevelNoDb(qreal level)
{
    if ( level != m_level )
    {
        m_level = level;

        emit levelChanged();
        emit dBlevelChanged();
    }
}

void StreamNode::setDBlevel(qreal db)
{
    m_db_level = db;
    setLevelNoDb( std::pow(10.f, db*.05) );
}

void StreamNode::setParentStream(StreamNode* stream)
{
    if ( stream != m_parent_stream)
    {
        m_parent_stream = stream;
        m_parent_stream->appendSubnode( this );
    }
}

//-------------------------------------------------------------------------------------------

QVector<quint16> StreamNode::parentChannelsVec() const
{
    QVector<quint16> res;

    if ( m_parent_channels.type() == QMetaType::QVariantList )
        for ( const auto& ch : m_parent_channels.toList() )
            res << ch.toInt();

    else if ( m_parent_channels.type() == QMetaType::Int )
        res << m_parent_channels.toInt();

    return res;
}

void StreamNode::setParentChannels(QVariant pch)
{
    m_parent_channels = pch;
}

QQmlListProperty<StreamNode> StreamNode::subnodes()
{
    return QQmlListProperty<StreamNode>( this, this,
                           &StreamNode::appendSubnode,
                           &StreamNode::subnodesCount,
                           &StreamNode::subnode,
                           &StreamNode::clearSubnodes );
}

StreamNode* StreamNode::subnode(int index) const
{
    return m_subnodes.at(index);
}

void StreamNode::appendSubnode(StreamNode* subnode)
{
    m_subnodes.append(subnode);
    if ( !m_num_inputs ) setMaxOutputs(subnode->maxOutputs());
}

int StreamNode::subnodesCount() const
{
    return m_subnodes.count();
}

void StreamNode::clearSubnodes()
{
    m_subnodes.clear();
}

// statics --

void StreamNode::appendSubnode(QQmlListProperty<StreamNode>* list, StreamNode* subnode)
{
    reinterpret_cast<StreamNode*>(list->data)->appendSubnode(subnode);
}

void StreamNode::clearSubnodes(QQmlListProperty<StreamNode>* list )
{
    reinterpret_cast<StreamNode*>(list->data)->clearSubnodes();
}

StreamNode* StreamNode::subnode(QQmlListProperty<StreamNode>* list, int i)
{
    return reinterpret_cast<StreamNode*>(list->data)->subnode(i);
}

int StreamNode::subnodesCount(QQmlListProperty<StreamNode>* list)
{
    return reinterpret_cast<StreamNode*>(list->data)->subnodesCount();
}

//-------------------------------------------------------------------------------------------

void StreamNode::allocateBuffer(float**& buffer, quint16 nchannels, quint64 nsamples )
{
    buffer = new float* [ nchannels ]();
    for ( uint16_t ch = 0; ch < nchannels; ++ch )
        buffer[ch] = new float [ nsamples ]();
}

void StreamNode::deleteBuffer(float**& buffer, quint16 nchannels, quint16 nsamples )
{
    if ( !buffer ) return;

    for ( uint16_t ch = 0; ch < nchannels; ++ch )
        delete [ ] buffer[ch];

    delete [ ] buffer;
}

void StreamNode::resetBuffer(float**& buffer, quint16 nchannels, quint16 nsamples )
{
    for ( uint16_t ch = 0; ch < nchannels; ++ch )
        memset(buffer[ch], 0.f, sizeof(float)*nsamples);
}

void StreamNode::applyGain(float**& buffer, quint16 nchannels, quint16 nsamples, float gain)
{
    if ( gain == 1.f ) return;

    for ( quint16 ch = 0; ch < nchannels; ++ch )
        for ( quint16 s = 0; s < nsamples; ++s )
            buffer[ch][s] *= gain;
}

void StreamNode::mergeBuffers(float**& lhs, float** rhs, quint16 lnchannels,
                              quint16 rnchannels, quint16 nsamples )
{
    for ( quint16 ch = 0; ch < rnchannels; ++ch )
        for ( quint16 s = 0; s < nsamples; ++s )
            lhs[ch][s] += rhs[ch][s];
}

void StreamNode::preinitialize(StreamProperties properties)
{
    m_stream_properties = properties;

    if ( m_stream_properties.block_size != properties.block_size && m_out )
    {
        StreamNode::deleteBuffer(m_in, m_num_inputs, m_stream_properties.block_size );
        StreamNode::deleteBuffer(m_out, m_num_outputs, m_stream_properties.block_size);
    }

    StreamNode::allocateBuffer(m_in, m_num_inputs, properties.block_size);
    StreamNode::allocateBuffer(m_out, m_num_outputs, properties.block_size);

    initialize( properties.block_size );

    for ( const auto& subnode : m_subnodes )
          subnode->preinitialize( properties );
}

inline float** StreamNode::mergeInputs(float** buf, qint64 nsamples)
{
    for ( const auto& subnode : m_subnodes )
    {
        if ( subnode->active() )
        {
            auto pch     = subnode->parentChannelsVec();
            auto genbuf  = subnode->preprocess( nullptr, nsamples );

            for ( quint16 ch = 0; ch < pch.size(); ++ch )
                for ( quint16 s = 0; s < nsamples; ++s )
                    buf[pch[ch]][s] += genbuf[ch][s];
        }
    }
}

float** StreamNode::preprocess(float** buf, qint64 le)
{   
    if ( m_type == StreamType::Generator )
    {
        // process and pass buffer down the effects chain
        float** ubuf = process(buf, le);
        StreamNode::applyGain(ubuf, m_num_outputs, le, m_level);

        for ( const auto& subnode : m_subnodes )
            if ( subnode->active() && subnode->numInputs() == m_num_outputs )
                 ubuf = subnode->preprocess(ubuf, le);

        return ubuf;
    }

    else if ( m_type == StreamType::Mixer )
    {
        // merge all subnodes and apply master gain
        auto out = m_out;
        StreamNode::resetBuffer( out, m_num_outputs, le );
        mergeInputs( out, le );
        return out;
    }

    else if ( m_type == StreamType::Effect )
    {
        // otherwise mix all sources down to an array of channels
        // and then process it
        float** in = m_in, **out = nullptr;
        StreamNode::resetBuffer(in, m_num_inputs, le);

        if  ( buf != nullptr )
            StreamNode::mergeBuffers(in, buf, m_num_inputs, m_num_inputs, le);

        mergeInputs( in, le );

        out = process( in, le );
        StreamNode::applyGain(out, m_num_outputs, le, m_level);
        return out;
    }   
}

//-----------------------------------------------------------------------------------------------

WorldStream::WorldStream() : m_sample_rate( 44100 ), m_block_size( 512 )
{
    SETTYPE( StreamType::Mixer );
}

WorldStream::~WorldStream()
{
    emit exit();
    m_stream_thread.quit();

    if ( !m_stream_thread.wait(3000) )
    {
        m_stream_thread.terminate();
        m_stream_thread.wait();
        m_stream->deleteLater();
    }
}

void WorldStream::setSampleRate(uint32_t sample_rate)
{
    if ( sample_rate != m_sample_rate ) emit sampleRateChanged();
    m_sample_rate = sample_rate;
}

void WorldStream::setBlockSize(uint16_t block_size)
{
    if ( block_size != m_block_size ) emit blockSizeChanged();
    m_block_size = block_size;
}

void WorldStream::setInDevice(QString device)
{
    if ( device != m_in_device ) emit inDeviceChanged();
    m_in_device = device;
}

void WorldStream::setOutDevice(QString device)
{
    if ( device != m_out_device ) emit outDeviceChanged();
    m_out_device = device;
}

void WorldStream::componentComplete()
{
    RtAudio audio;
    RtAudio::StreamParameters parameters;
    RtAudio::DeviceInfo info;
    RtAudio::StreamOptions options;

    auto ndevices = audio.getDeviceCount();

    if ( !m_out_device.isEmpty())
    {
        for ( quint32 d = 0; d < ndevices; ++d )
        {
            info = audio.getDeviceInfo(d);
            auto name = QString::fromStdString(info.name);
            qDebug() << name;

            if ( name.contains(m_out_device) )
            {
                parameters.deviceId = d;
                break;
            }
        }
    }

    else parameters.deviceId = audio.getDefaultOutputDevice();

    parameters.nChannels = m_num_outputs;
    options.streamName = "WPN114";
    options.flags = RTAUDIO_SCHEDULE_REALTIME;
    options.priority = 10;

    m_stream = new AudioStream( *this, parameters, info, options);
    m_stream->moveToThread  ( &m_stream_thread );

    QObject::connect( this, &StreamNode::activeChanged, this, &WorldStream::onActiveChanged );
    QObject::connect( this, &WorldStream::startStream, m_stream, &AudioStream::start);
    QObject::connect( this, &WorldStream::stopStream, m_stream, &AudioStream::stop);
    QObject::connect( this, &WorldStream::configure, m_stream, &AudioStream::configure);
    QObject::connect( m_stream, &AudioStream::tick, this, &WorldStream::tick );

    emit configure();
    m_stream_thread.start ( QThread::TimeCriticalPriority );
}

void WorldStream::onActiveChanged()
{
    if ( m_active ) start();
    else stop();
}

void WorldStream::start()
{
    emit startStream();
}

void WorldStream::stop()
{
    emit stopStream();
}

QQmlListProperty<StreamNode> WorldStream::inserts()
{
    return QQmlListProperty<StreamNode>( this, this,
                           &WorldStream::appendInsert,
                           &WorldStream::insertsCount,
                           &WorldStream::insert,
                           &WorldStream::clearInserts );
}

StreamNode* WorldStream::insert(int index) const
{
    return m_inserts.at(index);
}

void WorldStream::appendInsert(StreamNode* insert)
{
    m_inserts.append(insert);
}

int WorldStream::insertsCount() const
{
    return m_inserts.count();
}

void WorldStream::clearInserts()
{
    m_inserts.clear();
}

// statics --

void WorldStream::appendInsert(QQmlListProperty<StreamNode>* list, StreamNode* insert)
{
    reinterpret_cast<WorldStream*>(list->data)->appendInsert(insert);
}

void WorldStream::clearInserts(QQmlListProperty<StreamNode>* list )
{
    reinterpret_cast<WorldStream*>(list->data)->clearInserts();
}

StreamNode* WorldStream::insert(QQmlListProperty<StreamNode>* list, int i)
{
    return reinterpret_cast<WorldStream*>(list->data)->insert(i);
}

int WorldStream::insertsCount(QQmlListProperty<StreamNode>* list)
{
    return reinterpret_cast<WorldStream*>(list->data)->insertsCount();
}

// -----------------------------------------------------------------------------------------

AudioStream::AudioStream( WorldStream& world,
                          RtAudio::StreamParameters parameters,
                          RtAudio::DeviceInfo info,
                          RtAudio::StreamOptions options) :

    m_world(world), m_parameters(parameters),
    m_device_info(info), m_options(options), m_stream(new RtAudio)
{

}

AudioStream::~AudioStream()
{
    StreamNode::deleteBuffer(m_pool, m_world.numOutputs(), m_world.blockSize());
    delete m_stream;
}

void AudioStream::exit()
{
    m_stream->stopStream();
    m_stream->closeStream();
}

void AudioStream::configure()
{
    m_blocksize = m_world.blockSize();
    m_format = RTAUDIO_FLOAT32;

    try
    {
        m_stream->openStream( &m_parameters, nullptr, m_format,
            m_world.sampleRate(), &m_blocksize,
            &readData, (void*) &m_world, &m_options, nullptr );
    }

    catch(const RtAudioError& e)
    {
        qDebug() << "OPENSTREAM_ERROR:" << QString::fromStdString(e.getMessage());
    }
}

void AudioStream::start()
{
    StreamProperties properties = { m_world.m_sample_rate, m_world.m_block_size };
    m_world.preinitialize( properties );

    for ( const auto& insert : m_world.m_inserts )
        insert->preinitialize( properties );

    try     { m_stream->startStream(); }
    catch   ( const RtAudioError& e )
    {
        qDebug() << "STARTSTREAM_ERROR:" << QString::fromStdString(e.getMessage());
    }
}

void AudioStream::stop()
{
    try     { m_stream->stopStream(); }
    catch   ( const RtAudioError& e )
    {
        e.printMessage();
    }
}

void AudioStream::restart()
{
    // no need to reinitialize buffers
}

qint64 AudioStream::uclock() const
{
    return 0;
}

void AudioStream::onBufferProcessed(double time)
{
    qint64 msecs = time*1000;
    if ( !m_clock ) m_clock = msecs;

    emit tick( msecs-m_clock );
    m_clock = msecs;
}

int readData( void* out, void* in, unsigned int nframes,
              double time, RtAudioStreamStatus status, void *udata)
{
    WorldStream& world = *((WorldStream*) udata);
    world.stream()->onBufferProcessed(time);

    auto inserts    = world.m_inserts;
    quint16 nout    = world.m_num_outputs;
    quint16 bsize   = world.m_block_size;
    float* data     = ( float* ) out;

    auto buf = world.preprocess( nullptr, bsize );

    for ( const auto& insert : inserts )
    {
        if ( !insert->active() ) continue;
        buf = insert->preprocess( buf, bsize );
    }

    StreamNode::applyGain(buf, nout, bsize, world.m_level);

    for ( quint16 s = 0; s < bsize; ++s )
        for ( quint16 ch = 0; ch < nout; ++ch )
            *data++ = buf[ch][s];

    return 0;
}
