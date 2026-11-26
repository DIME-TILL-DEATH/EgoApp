#include "decodestream.h"

#include <QAudioBuffer>
#include <QMutexLocker>

DecodeStream::DecodeStream(QObject *parent)
    : QIODevice(parent)
{
}

DecodeStream::~DecodeStream()
{
    close();
}

void DecodeStream::setFormat(const QAudioFormat &format)
{
    m_format = format;
    if (m_decoder)
        m_decoder->setAudioFormat(m_format);
}

bool DecodeStream::open(const QString &path, OpenMode mode)
{
    close();
    m_sourcePath = path;
    return open(mode);
}

bool DecodeStream::open(OpenMode mode)
{
    close();

    if (!(mode & QIODevice::ReadOnly) || (mode & QIODevice::WriteOnly))
        return false;

    {
        QMutexLocker locker(&m_pcmMutex);
        m_pcmFile = std::make_unique<QTemporaryFile>();
        if (!m_pcmFile->open())
        {
            m_pcmFile.reset();
            return false;
        }

        m_decoder = new QAudioDecoder(this);
        m_sourceFile = new QFile(m_sourcePath, m_decoder);
        if (!m_sourceFile->open(QIODevice::ReadOnly))
        {
            m_decoder->deleteLater();
            m_decoder = nullptr;
            m_sourceFile = nullptr;
            m_pcmFile.reset();
            return false;
        }

        // Disable QIODevice prefetching: byte availability and the spool cursor
        // are shared with the mixer and decoder append path.
        if (!QIODevice::open(QIODevice::ReadOnly | QIODevice::Unbuffered))
        {
            m_decoder->deleteLater();
            m_decoder = nullptr;
            m_sourceFile = nullptr;
            m_pcmFile.reset();
            return false;
        }
    }

    m_closing.store(false);
    m_reportedDuration = 0;
    m_decodingFinished.store(false);
    m_decoderFailed.store(false);
    const quint64 generation = ++m_generation;

    // The decoder and its source device belong to exactly one playback
    // session. Retiring them together prevents delayed backend work or signals
    // from an earlier source from touching a later source after a restart.
    m_decoder->setAudioFormat(m_format);

    connect(m_decoder, &QAudioDecoder::durationChanged, this,
            [this, generation](qint64 duration) {
                if (isCurrentSession(generation))
                    reportDuration(duration);
            });
    connect(m_decoder, &QAudioDecoder::bufferReady, this,
            [this, generation]() {
                handleBufferReady(generation);
            });
    connect(m_decoder, &QAudioDecoder::finished, this,
            [this, generation]() {
                if (!isCurrentSession(generation) || m_decoderFailed.load())
                    return;

                m_decodingFinished.store(true);
                emit decodingFinished();
                emit readyRead();
            });
    connect(m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this,
            [this, generation](QAudioDecoder::Error error) {
                handleDecoderError(generation, error);
            });

    m_decoder->setSourceDevice(m_sourceFile);
    m_decoder->start();
    return true;
}

void DecodeStream::close()
{
    if (m_closing.exchange(true))
        return;

    ++m_generation;
    if (m_decoder)
    {
        disconnect(m_decoder, nullptr, this, nullptr);
        m_decoder->stop();
        // Keep the source QFile alive as a decoder child until the decoder is
        // actually destroyed; the backend may still be unwinding its stop.
        m_decoder->deleteLater();
        m_decoder = nullptr;
        m_sourceFile = nullptr;
    }

    {
        QMutexLocker locker(&m_pcmMutex);
        if (m_pcmFile)
        {
            m_pcmFile->close();
            m_pcmFile.reset();
        }
        m_decodingFinished.store(true);
        QIODevice::close();
    }

    m_closing.store(false);
}

bool DecodeStream::isCurrentSession(quint64 generation) const
{
    return generation == m_generation && !m_closing.load()
           && !m_decoderFailed.load() && m_decoder;
}

qint64 DecodeStream::readData(char *data, qint64 maxSize)
{
    QMutexLocker locker(&m_pcmMutex);
    if (!m_pcmFile || !m_pcmFile->isOpen() || maxSize <= 0)
        return 0;

    const qint64 available = qMax<qint64>(0, m_pcmFile->size() - m_pcmFile->pos());
    return available == 0 ? 0 : m_pcmFile->read(data, qMin(maxSize, available));
}

qint64 DecodeStream::writeData(const char *, qint64)
{
    return -1;
}

qint64 DecodeStream::bytesAvailable() const
{
    QMutexLocker locker(&m_pcmMutex);
    if (!m_pcmFile || !m_pcmFile->isOpen())
        return 0;

    return qMax<qint64>(0, m_pcmFile->size() - m_pcmFile->pos());
}

qint64 DecodeStream::size() const
{
    QMutexLocker locker(&m_pcmMutex);
    return m_pcmFile && m_pcmFile->isOpen() ? m_pcmFile->size() : 0;
}

qint64 DecodeStream::pos() const
{
    QMutexLocker locker(&m_pcmMutex);
    return m_pcmFile && m_pcmFile->isOpen() ? m_pcmFile->pos() : 0;
}

bool DecodeStream::seek(qint64 position)
{
    QMutexLocker locker(&m_pcmMutex);
    if (!m_pcmFile || !m_pcmFile->isOpen() || position < 0
        || !m_pcmFile->seek(position))
        return false;

    return QIODevice::seek(position);
}

bool DecodeStream::atEnd() const
{
    return m_decodingFinished.load() && bytesAvailable() == 0;
}

bool DecodeStream::isDecodingFinished() const
{
    return m_decodingFinished.load();
}

void DecodeStream::handleBufferReady(quint64 generation)
{
    if (!isCurrentSession(generation))
        return;

    const QAudioBuffer buffer = m_decoder->read();
    if (!isCurrentSession(generation) || !buffer.isValid() || buffer.byteCount() <= 0)
        return;

    QString writeError;
    {
        QMutexLocker locker(&m_pcmMutex);
        if (!isCurrentSession(generation) || !m_pcmFile || !m_pcmFile->isOpen())
            return;

        const qint64 currentSize = m_pcmFile->size();
        if (buffer.byteCount() > MaxPcmBytes - currentSize)
        {
            writeError = tr("Decoded audio exceeds the one-hour per-track limit.");
        }
        else
        {
            const qint64 readPosition = m_pcmFile->pos();
            if (!m_pcmFile->seek(currentSize))
                writeError = m_pcmFile->errorString();
            else
            {
                const qint64 written = m_pcmFile->write(
                    buffer.constData<char>(), buffer.byteCount());
                const bool flushed = m_pcmFile->flush();
                writeError = m_pcmFile->errorString();
                m_pcmFile->seek(readPosition);

                if (written == buffer.byteCount() && flushed)
                    writeError.clear();
                else if (writeError.isEmpty())
                    writeError = tr("Unable to store decoded audio data.");
            }
        }
    }

    if (!writeError.isEmpty())
    {
        if (!m_decoderFailed.exchange(true))
        {
            m_decodingFinished.store(true);
            if (m_decoder && generation == m_generation)
                m_decoder->stop();
            emit errorOccurred(writeError);
        }
        return;
    }

    const qint64 bytesPerFrame = m_format.bytesPerFrame();
    const int sampleRate = m_format.sampleRate();
    if (bytesPerFrame > 0 && sampleRate > 0)
    {
        qint64 decodedBytes = 0;
        {
            QMutexLocker locker(&m_pcmMutex);
            if (m_pcmFile && m_pcmFile->isOpen())
                decodedBytes = m_pcmFile->size();
        }

        const qint64 decodedFrames = decodedBytes / bytesPerFrame;
        reportDuration(decodedFrames * 1000 / sampleRate);
    }

    emit readyRead();
}

void DecodeStream::reportDuration(qint64 duration)
{
    if (duration <= m_reportedDuration)
        return;

    m_reportedDuration = duration;
    emit durationChanged(duration);
}

void DecodeStream::handleDecoderError(quint64 generation, QAudioDecoder::Error error)
{
    if (!isCurrentSession(generation) || error == QAudioDecoder::NoError
        || m_decoderFailed.exchange(true))
        return;

    m_decodingFinished.store(true);
    QString message = m_decoder->errorString();
    if (message.isEmpty())
        message = tr("Audio decoder failed.");

    // An error is a terminal failure, not a successful decodingFinished event.
    // The owner handles it by stopping and closing this stream.
    emit errorOccurred(message);
}
