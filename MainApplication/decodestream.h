#ifndef DECODESTREAM_H
#define DECODESTREAM_H

#include <QAudioDecoder>
#include <QFile>
#include <QTemporaryFile>
#include <QMutex>

#include <memory>

class DecodeStream final : public QIODevice
{
    Q_OBJECT

public:
    explicit DecodeStream(QObject *parent = nullptr);
    ~DecodeStream() override;

    void setFormat(const QAudioFormat &format);

    bool open(const QString &filePath, OpenMode mode = QIODevice::ReadOnly);
    bool open(OpenMode mode) override;
    void close() override;

    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;
    qint64 bytesAvailable() const override;
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    bool atEnd() const override;

    bool isDecodingFinished() const;

signals:
    void durationChanged(qint64 duration);
    void decodingFinished();
    void errorOccurred(QString errorString);

private:
    void handleBufferReady();
    void handleDecoderError(QAudioDecoder::Error error);

    QFile m_sourceFile;
    std::unique_ptr<QTemporaryFile> m_pcmFile;
    QAudioDecoder m_decoder;
    QAudioFormat m_format;
    bool m_decodingFinished{true};
    bool m_closing{false};
    mutable QMutex m_pcmMutex;
};

#endif // DECODESTREAM_H
