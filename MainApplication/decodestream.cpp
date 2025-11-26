#include "decodestream.h"

#include <QDebug>

DecodeStream::DecodeStream()
    : m_inputBuf(&m_data)
    , m_outputBuf(&m_data)
{
    setOpenMode(QIODevice::ReadOnly);

    m_outputBuf.open(QIODevice::ReadOnly);
    m_inputBuf.open(QIODevice::WriteOnly);

    // connect(&m_decoder, &QAudioDecoder::durationChanged, this, &DecodeStream::durationChanged);
    connect(&m_decoder, &QAudioDecoder::bufferReady, this, [this]() {
        const QAudioBuffer buffer = m_decoder.read();
        m_inputBuf.write(buffer.data<char>(), buffer.byteCount());
    });
}

DecodeStream::~DecodeStream()
{
}

void DecodeStream::setFormat(const QAudioFormat &format)
{
    m_format = format;
    m_decoder.setAudioFormat(m_format);
}

bool DecodeStream::open(const QString &filePath, OpenMode mode)
{
    close();

    m_sourceFile.setFileName(filePath);
    return open(mode);
}

bool DecodeStream::open(OpenMode mode)
{
    if(m_sourceFile.open(QIODevice::ReadOnly))
    {
        m_decoder.setSourceDevice(&m_sourceFile);
        m_decoder.start();
        emit durationChanged(m_decoder.duration());
        return true;
    }
    else
    {
        return false;
    }
}

void DecodeStream::close()
{
    m_sourceFile.close();
    m_decoder.stop();

    m_inputBuf.seek(0);
    m_outputBuf.seek(0);
    m_data.clear();
}

qint64 DecodeStream::readData(char *data, qint64 maxSize)
{
    // qDebug() << __FUNCTION__ << m_outputBuf.pos();

    memset(data, 0, maxSize);

    if(m_outputBuf.size())
        maxSize = m_outputBuf.read(data, maxSize);
    // if (m_outputBuf.size() && m_outputBuf.atEnd())
    // {
    //     emit finished();
    // }

    return maxSize;
}

qint64 DecodeStream::writeData(const char *data, qint64 maxSize)
{
    Q_ASSERT_X(false, "writeData", "not implemented");
    return 0;
}

qint64 DecodeStream::bytesAvailable() const
{
    return m_outputBuf.bytesAvailable();// + QIODevice::bytesAvailable();
}

qint64 DecodeStream::size() const
{
    return m_data.size();
}

qint64 DecodeStream::pos() const
{
    return m_outputBuf.pos();
}

bool DecodeStream::seek(qint64 pos)
{
    return m_outputBuf.seek(pos);
}

bool DecodeStream::atEnd() const
{
    return m_outputBuf.atEnd();
}
