#include "decodestream.h"
#include <QAudioBuffer>
#include <QMutexLocker>
DecodeStream::DecodeStream(QObject *parent) : QIODevice(parent)
{
    connect(&m_decoder, &QAudioDecoder::durationChanged, this, &DecodeStream::durationChanged);
    connect(&m_decoder, &QAudioDecoder::bufferReady, this, &DecodeStream::handleBufferReady);
    connect(&m_decoder, &QAudioDecoder::finished, this, [this]() {
        if (m_closing)
            return;
        m_decodingFinished = true;
        emit decodingFinished();
        emit readyRead();
    });
    connect(&m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error),
            this, &DecodeStream::handleDecoderError);
}
DecodeStream::~DecodeStream() { close(); }
void DecodeStream::setFormat(const QAudioFormat &format) { m_format=format; m_decoder.setAudioFormat(m_format); }
bool DecodeStream::open(const QString &path, OpenMode mode) { close(); m_sourceFile.setFileName(path); return open(mode); }
bool DecodeStream::open(OpenMode mode)
{
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

        if (!m_sourceFile.open(QIODevice::ReadOnly))
        {
            m_pcmFile.reset();
            return false;
        }

        if (!QIODevice::open(QIODevice::ReadOnly))
        {
            m_sourceFile.close();
            m_pcmFile.reset();
            return false;
        }
        m_decodingFinished = false;
        m_closing = false;
    }

    m_decoder.setSourceDevice(&m_sourceFile);
    m_decoder.start();
    return true;
}
void DecodeStream::close()
{
    if (m_closing) return;
    m_closing = true;
    m_decoder.stop();
    QMutexLocker locker(&m_pcmMutex);
    m_sourceFile.close();
    if (m_pcmFile) { m_pcmFile->close(); m_pcmFile.reset(); }
    m_decodingFinished = true;
    QIODevice::close();
    m_closing = false;
}
qint64 DecodeStream::readData(char *data, qint64 maxSize)
{
    QMutexLocker locker(&m_pcmMutex);
    if (!m_pcmFile || !m_pcmFile->isOpen() || maxSize<=0) return 0;
    const qint64 available=qMax<qint64>(0,m_pcmFile->size()-m_pcmFile->pos());
    return available==0 ? 0 : m_pcmFile->read(data,qMin(maxSize,available));
}
qint64 DecodeStream::writeData(const char *, qint64) { return -1; }
qint64 DecodeStream::bytesAvailable() const
{
    QMutexLocker locker(&m_pcmMutex);
    if (!m_pcmFile || !m_pcmFile->isOpen()) return 0;
    return qMax<qint64>(0,m_pcmFile->size()-m_pcmFile->pos());
}
qint64 DecodeStream::size() const { QMutexLocker locker(&m_pcmMutex); return m_pcmFile && m_pcmFile->isOpen() ? m_pcmFile->size() : 0; }
qint64 DecodeStream::pos() const { QMutexLocker locker(&m_pcmMutex); return m_pcmFile && m_pcmFile->isOpen() ? m_pcmFile->pos() : 0; }
bool DecodeStream::seek(qint64 position)
{
    QMutexLocker locker(&m_pcmMutex);
    if (!m_pcmFile || !m_pcmFile->isOpen() || position<0 || !m_pcmFile->seek(position)) return false;
    return QIODevice::seek(position);
}
bool DecodeStream::atEnd() const { return m_decodingFinished && bytesAvailable()==0; }
bool DecodeStream::isDecodingFinished() const { return m_decodingFinished; }
void DecodeStream::handleBufferReady()
{
    if (m_closing)
        return;

    const QAudioBuffer buffer = m_decoder.read();
    if (!buffer.isValid() || buffer.byteCount() <= 0)
        return;

    QString writeError;
    {
        QMutexLocker locker(&m_pcmMutex);
        if (m_closing || !m_pcmFile || !m_pcmFile->isOpen())
            return;

        const qint64 readPosition = m_pcmFile->pos();
        if (!m_pcmFile->seek(m_pcmFile->size()))
            writeError = m_pcmFile->errorString();
        else
        {
            const qint64 written = m_pcmFile->write(buffer.constData<char>(), buffer.byteCount());
            const bool flushed = m_pcmFile->flush();
            writeError = m_pcmFile->errorString();
            m_pcmFile->seek(readPosition);
            if (written == buffer.byteCount() && flushed)
                writeError.clear();
            else if (writeError.isEmpty())
                writeError = tr("Unable to store decoded audio data.");
        }
    }

    if (!writeError.isEmpty())
    {
        emit errorOccurred(writeError);
        return;
    }
    emit readyRead();
}

void DecodeStream::handleDecoderError(QAudioDecoder::Error error)
{
    if (m_closing || error==QAudioDecoder::NoError) return;
    m_decodingFinished=true; emit errorOccurred(m_decoder.errorString());
    emit decodingFinished(); emit readyRead();
}
