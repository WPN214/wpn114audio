#ifndef SOUNDFILE_HPP
#define SOUNDFILE_HPP

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QThread>

#define WAVE_METADATA_SIZE 44

struct WavMetadata
{
    qint32 chunk_id;
    qint32 chunk_size;
    qint32 format;

    qint32 subchunk1_id;
    qint32 subchunk1_size;
    qint16 audio_format;
    qint16 nchannels;
    qint32 sample_rate;
    qint32 byte_rate;
    qint16 block_align;
    qint16 bits_per_sample;

    qint32 subchunk2_id;
    qint32 subchunk2_size;
};

class Soundfile;

class SoundfileStreamer : public QObject
{
    Q_OBJECT

    public:
    SoundfileStreamer   ( Soundfile* sfile = 0 );
    ~SoundfileStreamer  ( );

    void setStartSample     ( quint64 index );
    void setEndSample       ( quint64 index );
    void setBufferSize      ( quint64 nsamples );
    void setWrap            ( bool wrap ) { m_wrap = wrap; }

    public slots:
    void next(float* target);
    void reset(float* target);

    signals:
    void bufferLoaded();

    private:
    QFile* m_file;
    Soundfile* m_soundfile;
    bool m_wrap;
    quint64 m_start_byte;
    quint64 m_end_byte;
    quint64 m_bufsize_byte;
    quint64 m_position_byte;

};

class Soundfile : public QObject
{
    Q_OBJECT

    friend class SoundfileStreamer;

    Q_PROPERTY  ( QString path READ path WRITE setPath )
    Q_PROPERTY  ( int nchannels READ nchannels )
    Q_PROPERTY  ( int nframes READ nframes )
    Q_PROPERTY  ( int nsamples READ nsamples )
    Q_PROPERTY  ( int sampleRate READ sampleRate )

    public:
    Soundfile   ( );
    Soundfile   ( QString path );

    ~Soundfile  ( );

    QString path        ( ) const { return m_path; }
    quint8 nchannels    ( ) const { return m_nchannels; }
    quint64 nframes     ( ) const { return m_nframes; }
    quint64 nsamples    ( ) const { return m_nsamples; }
    quint64 sampleRate  ( ) const { return m_sample_rate; }
    void metadataWav    ( );

    void setPath    ( QString path );
    void buffer     ( float* buffer, quint64 start_sample, quint64 len );
    void buffer     ( float** buffer, quint64 start_sample, quint64 len );

    protected:
    QFile* m_file;
    quint64 m_file_size;
    QString m_path;
    quint16 m_nchannels;
    quint32 m_sample_rate;
    quint64 m_nframes;
    quint64 m_nsamples;
    quint32 m_nbytes;
    quint16 m_bits_per_sample;
    quint32 m_metadata_size;
};

#endif // SOUNDFILE_HPP
