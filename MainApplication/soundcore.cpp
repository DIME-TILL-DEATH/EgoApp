#include "soundcore.h"

#include "sdcontentmodel.h"
#include "playlistmodel.h"

SoundCore::SoundCore(QObject *parent)
    : QObject{parent}
{
    m_track1Volume = m_settings.value("track1Volume", 1.0).toFloat();
    m_track2Volume = m_settings.value("track2Volume", 1.0).toFloat();
    m_sdTrackVolume = m_settings.value("sdTrackVolume", 1.0).toFloat();

    m_track1LEnabled = m_settings.value("track1LEnabled", true).toBool();
    m_track1REnabled = m_settings.value("track1REnabled", true).toBool();
    m_track2LEnabled = m_settings.value("track2LEnabled", true).toBool();
    m_track2REnabled = m_settings.value("track2REnabled", true).toBool();
    m_sdTrackLEnabled = m_settings.value("sdTrackLEnabled", true).toBool();
    m_sdTrackREnabled = m_settings.value("sdTrackREnabled", true).toBool();

    m_track1Muted = m_settings.value("track1Muted", false).toBool();
    m_track2Muted = m_settings.value("track2Muted", false).toBool();
    m_sdTrackMuted = m_settings.value("sdTrackMuted", false).toBool();

    m_soundPlayer.setTracksVolume(SoundPlayer::PlayEgo, m_track1Volume, m_track2Volume);
    m_soundPlayer.setTracksEn(SoundPlayer::PlayEgo, m_track1LEnabled, m_track1REnabled, m_track2LEnabled, m_track2REnabled);
    m_soundPlayer.setTracksVolume(SoundPlayer::PlaySd, m_sdTrackVolume);
    m_soundPlayer.setTracksEn(SoundPlayer::PlaySd, m_sdTrackLEnabled, m_sdTrackREnabled);

    m_soundPlayer.setMuted(m_track1Muted, m_track2Muted, m_sdTrackMuted);

    connect(&m_soundPlayer, &SoundPlayer::track1DurationChanged, this, &SoundCore::trackDurationChanged);
    connect(&m_soundPlayer, &SoundPlayer::positionUpdated, this, &SoundCore::trackPositionChanged);
}

void SoundCore::playContent(const QModelIndex &contentIndex)
{    
    const SdProxyModel* sdProxyModel = qobject_cast<const SdProxyModel*>(contentIndex.model());
    if(sdProxyModel)
    {
        const SdContentModel* sdModel = qobject_cast<const SdContentModel*>(sdProxyModel->sourceModel());

        QModelIndex realIndex = sdProxyModel->mapToSource(contentIndex);
        QString filePath =  sdModel->filePath(realIndex);

        QFileInfo fileInfo(filePath);

        if(fileInfo.isFile())
        {
            if(fileInfo.suffix().compare("wav", Qt::CaseSensitivity::CaseInsensitive) == 0)
            {
                m_soundPlayer.play(SoundPlayer::PlaySd, sdModel->rootPath() + sdModel->selectedPath());
            }
        }
    }
}

void SoundCore::playEgo(const QModelIndex &contentIndex)
{
    const PlaylistModel* plsModel = qobject_cast<const PlaylistModel*>(contentIndex.model());

    QDir wrkSpaceDir = plsModel->dir();
    wrkSpaceDir.cd("../..");
    QString wrkSpacePath = wrkSpaceDir.absolutePath();

    QString t1Path = wrkSpacePath + plsModel->data(contentIndex, PlaylistModel::T1PathRole).toString();
    QString t2Path = wrkSpacePath + plsModel->data(contentIndex, PlaylistModel::T2PathRole).toString();

    m_soundPlayer.play(SoundPlayer::PlayEgo, t1Path, t2Path);
}

void SoundCore::stop()
{
    m_soundPlayer.stop();
}

void SoundCore::setPosition(SoundPlayer::State mode, qint64 msPosition)
{
    if(mode != m_soundPlayer.state()) return;

    switch (m_soundPlayer.state())
    {
    case SoundPlayer::PlayEgo:
    {
        m_egoTrackPosition = msPosition;
        break;
    }

    case SoundPlayer::PlaySd:
    {
        m_sdTrackPosition = msPosition;
        break;
    }

    default:
        break;
    }

    m_soundPlayer.setPlayPosition(msPosition);
    emit qmlTrackPositionChanged();
}

void SoundCore::trackDurationChanged(qint64 duration)
{
    switch (m_soundPlayer.state())
    {
        case SoundPlayer::PlayEgo:
        {
            m_egoTrackDuration = duration;
            m_sdTrackDuration = 0;
            break;
        }

        case SoundPlayer::PlaySd:
        {
            m_sdTrackDuration = duration;
            m_egoTrackDuration = 0;
            break;
        }

    default:
        break;
    }

    emit qmlTrackDurationChanged();
}

void SoundCore::trackPositionChanged(qint64 posisiton)
{
    switch (m_soundPlayer.state())
    {
    case SoundPlayer::PlayEgo:
    {
        m_egoTrackPosition = posisiton;
        m_sdTrackPosition = 0;
        break;
    }

    case SoundPlayer::PlaySd:
    {
        m_sdTrackPosition = posisiton;
        m_egoTrackPosition = 0;
        break;
    }

    default:
        break;
    }

    emit qmlTrackPositionChanged();
}

//----------------------Getters/setters-------------------
float SoundCore::track1Volume() const
{
    return m_track1Volume;
}

void SoundCore::setTrack1Volume(float newTrack1Volume)
{
    if (m_track1Volume == newTrack1Volume)
        return;

    m_track1Volume = newTrack1Volume;
    emit track1VolumeChanged();

    m_settings.setValue("track1Volume", m_track1Volume);
    m_soundPlayer.setTracksVolume(SoundPlayer::PlayEgo, m_track1Volume, m_track2Volume);
}

float SoundCore::track2Volume() const
{
    return m_track2Volume;
}

void SoundCore::setTrack2Volume(float newTrack2Volume)
{
    if (m_track2Volume == newTrack2Volume)
        return;
    m_track2Volume = newTrack2Volume;
    emit track2VolumeChanged();

    m_settings.setValue("track2Volume", m_track2Volume);
    m_soundPlayer.setTracksVolume(SoundPlayer::PlayEgo, m_track1Volume, m_track2Volume);
}

float SoundCore::sdTrackVolume() const
{
    return m_sdTrackVolume;
}

void SoundCore::setSdTrackVolume(float newSdTrackVolume)
{
    if (m_sdTrackVolume == newSdTrackVolume)
        return;
    m_sdTrackVolume = newSdTrackVolume;
    emit sdTrackVolumeChanged();

    m_settings.setValue("sdTrackVolume", m_sdTrackVolume);
    m_soundPlayer.setTracksVolume(SoundPlayer::PlaySd, m_sdTrackVolume);
}

bool SoundCore::track1LEnabled() const
{
    return m_track1LEnabled;
}

void SoundCore::setTrack1LEnabled(bool newTrack1LEnabled)
{
    if (m_track1LEnabled == newTrack1LEnabled)
        return;
    m_track1LEnabled = newTrack1LEnabled;
    emit tracksEnabledChanged();

    m_settings.setValue("track1LEnabled", m_track1LEnabled);
    m_soundPlayer.setTracksEn(SoundPlayer::PlayEgo, m_track1LEnabled, m_track1REnabled, m_track2LEnabled, m_track2REnabled);
}

bool SoundCore::track1REnabled() const
{
    return m_track1REnabled;
}

void SoundCore::setTrack1REnabled(bool newTrack1REnabled)
{
    if (m_track1REnabled == newTrack1REnabled)
        return;
    m_track1REnabled = newTrack1REnabled;
    emit tracksEnabledChanged();

    m_settings.setValue("track1REnabled", m_track1REnabled);
    m_soundPlayer.setTracksEn(SoundPlayer::PlayEgo, m_track1LEnabled, m_track1REnabled, m_track2LEnabled, m_track2REnabled);
}

bool SoundCore::track2LEnabled() const
{
    return m_track2LEnabled;
}

void SoundCore::setTrack2LEnabled(bool newTrack2LEnabled)
{
    if (m_track2LEnabled == newTrack2LEnabled)
        return;
    m_track2LEnabled = newTrack2LEnabled;
    emit tracksEnabledChanged();

    m_settings.setValue("track2LEnabled", m_track2LEnabled);
    m_soundPlayer.setTracksEn(SoundPlayer::PlayEgo, m_track1LEnabled, m_track1REnabled, m_track2LEnabled, m_track2REnabled);
}

bool SoundCore::track2REnabled() const
{
    return m_track2REnabled;
}

void SoundCore::setTrack2REnabled(bool newTrack2REnabled)
{
    if (m_track2REnabled == newTrack2REnabled)
        return;
    m_track2REnabled = newTrack2REnabled;
    emit tracksEnabledChanged();

    m_settings.setValue("track2REnabled", m_track2REnabled);
    m_soundPlayer.setTracksEn(SoundPlayer::PlayEgo, m_track1LEnabled, m_track1REnabled, m_track2LEnabled, m_track2REnabled);
}

bool SoundCore::sdTrackLEnabled() const
{
    return m_sdTrackLEnabled;
}

void SoundCore::setSdTrackLEnabled(bool newSdTrackLEnabled)
{
    if (m_sdTrackLEnabled == newSdTrackLEnabled)
        return;
    m_sdTrackLEnabled = newSdTrackLEnabled;
    emit tracksEnabledChanged();

    m_settings.setValue("sdTrackLEnabled", m_sdTrackLEnabled);
    m_soundPlayer.setTracksEn(SoundPlayer::PlaySd, m_sdTrackLEnabled, m_sdTrackREnabled);
}

bool SoundCore::sdTrackREnabled() const
{
    return m_sdTrackREnabled;
}

void SoundCore::setSdTrackREnabled(bool newSdTrackREnabled)
{
    if (m_sdTrackREnabled == newSdTrackREnabled)
        return;
    m_sdTrackREnabled = newSdTrackREnabled;
    emit tracksEnabledChanged();

    m_settings.setValue("sdTrackREnabled", m_sdTrackREnabled);
    m_soundPlayer.setTracksEn(SoundPlayer::PlaySd, m_sdTrackLEnabled, m_sdTrackREnabled);
}

qint64 SoundCore::egoTrackDuration() const
{
    return m_egoTrackDuration;
}

qint64 SoundCore::sdTrackDuration() const
{
    return m_sdTrackDuration;
}

qint64 SoundCore::egoTrackPosition() const
{
    return m_egoTrackPosition;
}

qint64 SoundCore::sdTrackPosition() const
{
    return m_sdTrackPosition;
}

bool SoundCore::track1LMuted() const
{
    return m_track1Muted;
}

void SoundCore::setTrack1LMuted(bool newTrack1Muted)
{
    if (m_track1Muted == newTrack1Muted)
        return;
    m_track1Muted = newTrack1Muted;
    emit tracksMutedChanged();

    m_settings.setValue("track1Muted", m_track1Muted);
    m_soundPlayer.setMuted(m_track1Muted, m_track2Muted, m_sdTrackMuted);
}

bool SoundCore::track2LMuted() const
{
    return m_track2Muted;
}

void SoundCore::setTrack2LMuted(bool newTrack2Muted)
{
    if (m_track2Muted == newTrack2Muted)
        return;
    m_track2Muted = newTrack2Muted;
    emit tracksMutedChanged();

    m_settings.setValue("track2Muted", m_track2Muted);
    m_soundPlayer.setMuted(m_track1Muted, m_track2Muted, m_sdTrackMuted);
}

bool SoundCore::sdTrackLMuted() const
{
    return m_sdTrackMuted;
}

void SoundCore::setSdTrackLMuted(bool newSdTrackMuted)
{
    if (m_sdTrackMuted == newSdTrackMuted)
        return;
    m_sdTrackMuted = newSdTrackMuted;
    emit tracksMutedChanged();

    m_settings.setValue("sdTrackMuted", m_sdTrackMuted);
    m_soundPlayer.setMuted(m_track1Muted, m_track2Muted, m_sdTrackMuted);
}
