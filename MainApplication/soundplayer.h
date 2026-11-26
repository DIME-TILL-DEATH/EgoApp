#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include <QObject>
#include <QAudioFormat>
#include <QAudioSink>

#include <audiomixer.h>
#include <decodestream.h>
#include <qqmlintegration.h>

// QML_ELEMENT types must remain inheritable: Qt generates a QQmlElement<T> wrapper.
class SoundPlayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum State
    {
        Idle,
        PlaySd,
        PlayEgo
    };
    Q_ENUM(State)

    explicit SoundPlayer(QObject *parent = nullptr);
    ~SoundPlayer() override = default;

    bool play(State mode, const QString &track1Path, const QString &track2Path = {});
    void pauseResume();
    bool isPaused() const;
    void stop();
    void setPlayPosition(qint64 msPosition);

    State state() const;

    void setTracksVolume(State mode, float track1Volume, float track2Volume = 1.0f);
    void setTracksEn(State mode, bool track1LeftEnabled, bool track1RightEnabled,
                     bool track2LeftEnabled = false, bool track2RightEnabled = false);
    void setMuted(bool track1Mute, bool track2Mute, bool sdMute);

signals:
    void playbackStateChanged();
    void track1DurationChanged(qint64 duration);
    void track2DurationChanged(qint64 duration);
    void playbackDurationChanged(qint64 duration);
    void errorOccurred(QString errorString);
    void positionUpdated(qint64 newTimePosition);
    void playingFinished();

private slots:
    void handlePlayerStateChanged(QtAudio::State state);
    void handleTrackDataAvailable();
    void handleTrackFinished();

private:
    bool allTracksFinishedAndDrained() const;
    void startOutputIfReady();
    void finishPlayback();
    void updateMixerSettings();

    State m_state{State::Idle};
    DecodeStream m_streamTrack1;
    DecodeStream m_streamTrack2;
    AudioMixer m_mixer;
    QAudioFormat m_format;
    QAudioSink *m_audioSink{nullptr};

    float egoT1Volume{1.0f};
    float egoT2Volume{1.0f};
    float sdVolume{1.0f};

    bool egoT1Muted{false};
    bool egoT2Muted{false};
    bool sdMuted{false};

    bool egoT1LeftEnabled{true};
    bool egoT1RightEnabled{true};
    bool egoT2LeftEnabled{true};
    bool egoT2RightEnabled{true};
    bool sdLeftEnabled{true};
    bool sdRightEnabled{true};

    bool m_hasTrack1{false};
    bool m_hasTrack2{false};
    bool m_stopping{false};
    bool m_paused{false};
    bool m_outputStarted{false};
    qint64 m_track1Duration{0};
    qint64 m_track2Duration{0};
};

#endif // SOUNDPLAYER_H
