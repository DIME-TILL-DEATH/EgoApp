#ifndef DECODESTREAM_H
#define DECODESTREAM_H

#include <QAudioDecoder>
#include <QBuffer>
#include <QFile>

class DecodeStream : public QIODevice
{
    Q_OBJECT

public:

    explicit DecodeStream();
    ~DecodeStream();

    void setFormat(const QAudioFormat &format);

    bool open(const QString& filePath, OpenMode mode);
    bool open(OpenMode mode) override;
    void close() override;

    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;
    qint64 bytesAvailable() const override;

    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    bool atEnd() const override;
signals:
    void finished();

private:
    QFile m_sourceFile;
    QByteArray m_data;
    QBuffer m_inputBuf;
    QBuffer m_outputBuf;
    QAudioDecoder m_decoder;
    QAudioFormat m_format;
};

#endif // DECODESTREAM_H
