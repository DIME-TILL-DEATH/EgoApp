#ifndef DECODESTREAM_H
#define DECODESTREAM_H

#include <QAudioDecoder>
#include <QFile>
#include <QMutex>
#include <QTemporaryFile>

#include <atomic>
#include <memory>

class DecodeStream final : public QIODevice
{
    Q_OBJECT

public:
    // PCM is stored as 44.1 kHz, stereo, signed 16-bit samples.
    // Keep at most one hour of decoded audio per stream (about 606 MiB).
    static constexpr qint64 MaxPcmBytes =
        44100LL * 2 * sizeof(qint16) * 60 * 60;

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
    friend class AudioTests;

    static bool canAppendPcmBytes(qint64 currentSize, qint64 appendSize);
    bool isCurrentSession(quint64 generation) const;
    void handleBufferReady(quint64 generation);
    void handleDecoderError(quint64 generation, QAudioDecoder::Error error);
    void reportDuration(qint64 duration);

    QString m_sourcePath;
    QFile *m_sourceFile{nullptr};
    std::unique_ptr<QTemporaryFile> m_pcmFile;
    QAudioDecoder *m_decoder{nullptr};
    QAudioFormat m_format;
    qint64 m_reportedDuration{0};
    quint64 m_generation{0};
    std::atomic_bool m_decodingFinished{true};
    std::atomic_bool m_decoderFailed{false};
    std::atomic_bool m_closing{false};
    mutable QMutex m_pcmMutex;
};

#endif // DECODESTREAM_H
