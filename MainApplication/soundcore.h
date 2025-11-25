#ifndef SOUNDCORE_H
#define SOUNDCORE_H

#include <QObject>
#include <QQmlEngine>

#include <QModelIndex>

#include <QSettings>

#include <soundplayer.h>

class SoundCore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(float track1Volume READ track1Volume WRITE setTrack1Volume NOTIFY track1VolumeChanged FINAL)
    Q_PROPERTY(float track2Volume READ track2Volume WRITE setTrack2Volume NOTIFY track2VolumeChanged FINAL)
    Q_PROPERTY(float sdTrackVolume READ sdTrackVolume WRITE setSdTrackVolume NOTIFY sdTrackVolumeChanged FINAL)

    Q_PROPERTY(bool track1LEnabled READ track1LEnabled WRITE setTrack1LEnabled NOTIFY tracksEnabledChanged FINAL)
    Q_PROPERTY(bool track1REnabled READ track1REnabled WRITE setTrack1REnabled NOTIFY tracksEnabledChanged FINAL)
    Q_PROPERTY(bool track2LEnabled READ track2LEnabled WRITE setTrack2LEnabled NOTIFY tracksEnabledChanged FINAL)
    Q_PROPERTY(bool track2REnabled READ track2REnabled WRITE setTrack2REnabled NOTIFY tracksEnabledChanged FINAL)
    Q_PROPERTY(bool sdTrackLEnabled READ sdTrackLEnabled WRITE setSdTrackLEnabled NOTIFY tracksEnabledChanged FINAL)
    Q_PROPERTY(bool sdTrackREnabled READ sdTrackREnabled WRITE setSdTrackREnabled NOTIFY tracksEnabledChanged FINAL)
public:
    explicit SoundCore(QObject *parent = nullptr);

    Q_INVOKABLE void playContent(const QModelIndex &contentIndex);

    float track1Volume() const;
    void setTrack1Volume(float newTrack1Volume);

    float track2Volume() const;
    void setTrack2Volume(float newTrack2Volume);

    float sdTrackVolume() const;
    void setSdTrackVolume(float newSdTrackVolume);

    bool track1LEnabled() const;
    void setTrack1LEnabled(bool newTrack1LEnabled);

    bool track1REnabled() const;
    void setTrack1REnabled(bool newTrack1REnabled);

    bool track2LEnabled() const;
    void setTrack2LEnabled(bool newTrack2LEnabled);

    bool track2REnabled() const;
    void setTrack2REnabled(bool newTrack2REnabled);

    bool sdTrackLEnabled() const;
    void setSdTrackLEnabled(bool newSdTrackLEnabled);

    bool sdTrackREnabled() const;
    void setSdTrackREnabled(bool newSdTrackREnabled);

signals:
    void track1VolumeChanged();
    void track2VolumeChanged();
    void sdTrackVolumeChanged();

    void tracksEnabledChanged();

private:
    QSettings m_settings;
    SoundPlayer m_soundPlayer;

    float m_track1Volume{1.0};
    float m_track2Volume{1.0};
    float m_sdTrackVolume{1.0};

    bool m_track1LEnabled;
    bool m_track1REnabled;
    bool m_track2LEnabled;
    bool m_track2REnabled;
    bool m_sdTrackLEnabled;
    bool m_sdTrackREnabled;
};

#endif // SOUNDCORE_H
