#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H
#include <QIODevice>
#include <QPointer>
#include <QMutex>
class AudioMixer final : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioMixer(QObject *parent = nullptr);
    void setTracks(QIODevice *track1, QIODevice *track2);
    void setTrackFinished(bool track1Finished, bool track2Finished);
    void setTrack1Settings(float volume, bool leftEnabled, bool rightEnabled, bool muted);
    void setTrack2Settings(float volume, bool leftEnabled, bool rightEnabled, bool muted);
    bool seekToPosition(qint64 milliseconds);
    qint64 position() const;
    qint64 bytesAvailable() const override;
signals:
    void positionUpdated(qint64 milliseconds);
protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;
private:
    struct TrackSettings { float volume{1.0f}; bool leftEnabled{true}; bool rightEnabled{true}; bool muted{false}; };
    static constexpr qint64 BytesPerFrame = 2 * sizeof(qint16);
    static constexpr qint64 SampleRate = 44100;
    QPointer<QIODevice> m_track1;
    QPointer<QIODevice> m_track2;
    TrackSettings m_track1Settings;
    TrackSettings m_track2Settings;
    bool m_track1Finished{true};
    bool m_track2Finished{true};
    qint64 m_positionFrames{0};
    mutable QMutex m_mutex;
};
#endif // AUDIOMIXER_H
