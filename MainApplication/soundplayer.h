#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include <QObject>
#include <QIODevice>

#include <QAudioFormat>
#include <QAudioSink>

#include <decodestream.h>
#include <qqmlintegration.h>

class DecodeStream;

class SoundPlayer : public QIODevice
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum State{
        Idle,
        PlaySd,
        PlayEgo
    };
    Q_ENUM(State)

    explicit SoundPlayer(QObject *parent = nullptr);
    ~SoundPlayer();

    void play(State mode, const QString& track1Path, const QString& track2Path = "");
    void pauseResume();
    void stop();

    void setPlayPosition(qint64 msPosition);

    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;
    qint64 bytesAvailable() const override;

    State state() const;

    void setTracksVolume(State mode, float track1Volume, float track2Volume = 1.0);
    void setTracksEn(State mode, bool track1LeftEnabled, bool track1RightEnabled,
                        bool track2LeftEnabled = false, bool track2RightEnabled = false);
    void setMuted(bool track1Mute, bool track2Mute, bool sdMute);
signals:
    void track1DurationChanged(qint64 duration);
    void track2DurationChanged(qint64 duration);

    void positionUpdated(qint64 newTimePosition);

private slots:
    void handlePlayerStateChanged(QtAudio::State state);

private:
    State m_state{State::Idle};

    DecodeStream* m_streamTrack1;
    DecodeStream* m_streamTrack2;

    QAudioFormat m_format;

    QAudioSink* m_audioSink{nullptr};

    float egoT1Volume{1};
    float egoT2Volume{1};
    float sdVolume{1};

    bool egoT1Muted{false};
    bool egoT2Muted{false};
    bool sdMuted{false};

    bool egoT1LeftEnabled{true};
    bool egoT1RightEnabled{true};
    bool egoT2LeftEnabled{true};
    bool egoT2RightEnabled{true};
    bool sdLeftEnabled{true};
    bool sdRightEnabled{true};

    qint64 samplePos{0};

    inline qint16 combineSamples(qint32 samp1, qint32 samp2)
    {
        const auto sum = samp1 + samp2;
        if (std::numeric_limits<qint16>::max() < sum)
            return std::numeric_limits<qint16>::max();

        if (std::numeric_limits<qint16>::min() > sum)
            return std::numeric_limits<qint16>::min();

        return sum;
    }
};

#endif // SOUNDPLAYER_H
