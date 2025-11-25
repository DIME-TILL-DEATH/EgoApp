#include "soundcore.h"

#include "sdcontentmodel.h"

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

    m_soundPlayer.setTracksVolume(SoundPlayer::PlayEgo, m_track1Volume, m_track2Volume);
    m_soundPlayer.setTracksEn(SoundPlayer::PlayEgo, m_track1LEnabled, m_track1REnabled, m_track2LEnabled, m_track2REnabled);
    m_soundPlayer.setTracksVolume(SoundPlayer::PlaySd, m_sdTrackVolume);
    m_soundPlayer.setTracksEn(SoundPlayer::PlaySd, m_sdTrackLEnabled, m_sdTrackREnabled);
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
