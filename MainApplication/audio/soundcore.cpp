#include "soundcore.h"

#include "sdcontentmodel.h"
#include "playlists/playlistmodel.h"

#include <QFileInfo>
#include <QDir>
#include <utility>

SoundCore::SoundCore(QObject *parent)
    : SoundCore(std::unique_ptr<AudioOutput>{}, parent)
{
}

SoundCore::SoundCore(std::unique_ptr<AudioOutput> audioOutput, QObject *parent)
    : QObject{parent},
      m_soundPlayer(std::move(audioOutput))
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

    connect(&m_soundPlayer, &SoundPlayer::playbackDurationChanged, this, &SoundCore::trackDurationChanged);
    connect(&m_soundPlayer, &SoundPlayer::errorOccurred, this,
            [this](const QString &errorString) {
        clearPlaybackSelection();
        emit errorOccured(errorString);
    });
    connect(&m_soundPlayer, &SoundPlayer::positionUpdated, this, &SoundCore::trackPositionChanged);
    connect(&m_soundPlayer, &SoundPlayer::playbackStateChanged, this, &SoundCore::playbackStateChanged);
    connect(&m_soundPlayer, &SoundPlayer::playingFinished, this, &SoundCore::playingFinished);
}

void SoundCore::playContent(const QModelIndex &contentIndex)
{
    const auto *proxy = qobject_cast<const SdProxyModel *>(contentIndex.model());
    if (!proxy || !contentIndex.isValid())
    {
        emit errorOccured(QObject::tr("Can't play the selected audio track."));
        return;
    }

    const auto *model = qobject_cast<const SdContentModel *>(proxy->sourceModel());
    if (!model)
    {
        emit errorOccured(QObject::tr("Can't play the selected audio track."));
        return;
    }

    const QModelIndex sourceIndex = proxy->mapToSource(contentIndex);
    if (!sourceIndex.isValid())
    {
        emit errorOccured(QObject::tr("Can't play the selected audio track."));
        return;
    }

    const QString filePath = model->filePath(sourceIndex);
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.isFile()
        || fileInfo.suffix().compare(QStringLiteral("wav"), Qt::CaseInsensitive) != 0)
        return;

    const QString absolutePath = fileInfo.absoluteFilePath();
    m_egoTrackPosition = 0;
    m_sdTrackPosition = 0;
    m_egoTrackDuration = 0;
    m_sdTrackDuration = 0;
    emit qmlTrackPositionChanged();
    emit qmlTrackDurationChanged();

    if (!m_soundPlayer.play(SoundPlayer::PlaySd, absolutePath))
    {
        stop();
        emit errorOccured(QObject::tr("Unable to open the selected audio track."));
        return;
    }

    m_wavPlaying = QDir::fromNativeSeparators(
        QDir(model->rootPath()).relativeFilePath(absolutePath));
    if (!m_wavPlaying.startsWith(QLatin1Char('/')))
        m_wavPlaying.prepend(QLatin1Char('/'));

    m_egoSongPlaying.clear();
    m_playingEgoTrack1Path.clear();
    m_playingEgoTrack2Path.clear();
    m_playingEgoModel.clear();
    m_playingEgoIndex = QPersistentModelIndex{};
    m_playingSdModel = const_cast<SdContentModel *>(model);
    m_playingSdFilePath = absolutePath;
    emit playingStringChanged();
}

void SoundCore::playEgo(const QModelIndex &contentIndex)
{
    const auto *plsModel = qobject_cast<const PlaylistModel *>(contentIndex.model());
    if (!plsModel || !contentIndex.isValid()
        || contentIndex.row() < 0 || contentIndex.row() >= plsModel->rowCount(QModelIndex()))
    {
        emit errorOccured(QObject::tr("Can't play the selected playlist track."));
        return;
    }

    if (plsModel->isCorrupted())
    {
        emit errorOccured(QObject::tr("Playback is disabled because the playlist contains invalid or unreadable .ego files."));
        return;
    }

    const QString t1StoredPath =
        plsModel->data(contentIndex, PlaylistModel::T1PathRole).toString();
    const QString t2StoredPath =
        plsModel->data(contentIndex, PlaylistModel::T2PathRole).toString();

    const bool hasTrack1 = !t1StoredPath.isEmpty();
    const bool hasTrack2 = !t2StoredPath.isEmpty();
    if (!hasTrack1 && !hasTrack2)
    {
        emit errorOccured(QObject::tr("The selected song has no linked audio tracks."));
        return;
    }

    QString t1Path;
    if (hasTrack1)
    {
        if (!plsModel->resolveMediaPath(t1StoredPath, &t1Path))
        {
            emit errorOccured(QObject::tr("Track 1 has an invalid workspace path."));
            return;
        }
        const QFileInfo t1Info(t1Path);
        if (!t1Info.exists() || !t1Info.isFile())
        {
            emit errorOccured(QObject::tr("Track 1 not found."));
            return;
        }
    }

    QString t2Path;
    if (hasTrack2)
    {
        if (!plsModel->resolveMediaPath(t2StoredPath, &t2Path))
        {
            emit errorOccured(QObject::tr("Track 2 has an invalid workspace path."));
            return;
        }
        const QFileInfo t2Info(t2Path);
        if (!t2Info.exists() || !t2Info.isFile())
        {
            emit errorOccured(QObject::tr("Track 2 not found."));
            return;
        }
    }

    m_egoTrackPosition = 0;
    m_sdTrackPosition = 0;
    m_egoTrackDuration = 0;
    m_sdTrackDuration = 0;
    emit qmlTrackPositionChanged();
    emit qmlTrackDurationChanged();

    if (!m_soundPlayer.play(SoundPlayer::PlayEgo, t1Path, t2Path))
    {
        stop();
        emit errorOccured(QObject::tr("Unable to open the selected audio track."));
        return;
    }

    const QString t1Name = plsModel->data(contentIndex, PlaylistModel::T1NameRole).toString();
    const QString t2Name = plsModel->data(contentIndex, PlaylistModel::T2NameRole).toString();
    m_egoSongPlaying = t1Name.isEmpty() ? t2Name
                                        : (t2Name.isEmpty() ? t1Name : t1Name + "|" + t2Name);
    m_playingEgoTrack1Path = t1StoredPath;
    m_playingEgoTrack2Path = t2StoredPath;
    m_playingEgoModel = const_cast<PlaylistModel *>(plsModel);
    m_playingEgoIndex = QPersistentModelIndex(contentIndex);
    m_playingSdModel.clear();
    m_playingSdFilePath.clear();

    m_wavPlaying.clear();
    emit playingStringChanged();

}

void SoundCore::PlayPause(SoundPlayer::State mode, const QModelIndex &contentIndex)
{
    if (mode == SoundPlayer::PlayEgo)
    {
        const auto *model = qobject_cast<const PlaylistModel *>(contentIndex.model());
        if (!model || !contentIndex.isValid())
        {
            playEgo(contentIndex);
            return;
        }

        const QString track1 = model->data(contentIndex, PlaylistModel::T1PathRole).toString();
        const QString track2 = model->data(contentIndex, PlaylistModel::T2PathRole).toString();
        const bool sameSelection =
            m_soundPlayer.state() == mode
            && m_playingEgoModel == model
            && m_playingEgoIndex == QPersistentModelIndex(contentIndex)
            && m_playingEgoTrack1Path == track1
            && m_playingEgoTrack2Path == track2;

        if (sameSelection)
            m_soundPlayer.pauseResume();
        else
            playEgo(contentIndex);
        return;
    }

    if (mode == SoundPlayer::PlaySd)
    {
        const auto *proxy = qobject_cast<const SdProxyModel *>(contentIndex.model());
        if (!proxy || !contentIndex.isValid())
        {
            playContent(contentIndex);
            return;
        }

        const auto *model = qobject_cast<const SdContentModel *>(proxy->sourceModel());
        const QModelIndex sourceIndex = proxy->mapToSource(contentIndex);
        if (!model || !sourceIndex.isValid())
        {
            playContent(contentIndex);
            return;
        }

        const QFileInfo fileInfo(model->filePath(sourceIndex));
        if (!fileInfo.isFile()
            || fileInfo.suffix().compare(QStringLiteral("wav"), Qt::CaseInsensitive) != 0)
            return;

        const QString requestedPath = fileInfo.absoluteFilePath();
        const bool sameSelection =
            m_soundPlayer.state() == mode
            && m_playingSdModel == model
            && m_playingSdFilePath == requestedPath;

        if (sameSelection)
            m_soundPlayer.pauseResume();
        else
            playContent(contentIndex);
    }
}

void SoundCore::stop()
{
    clearPlaybackSelection();
    m_soundPlayer.stop();
}

void SoundCore::setPosition(SoundPlayer::State mode, qint64 msPosition)
{
    if (mode != m_soundPlayer.state())
        return;

    m_soundPlayer.setPlayPosition(msPosition);
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

void SoundCore::playingFinished()
{
    clearPlaybackSelection();
}

void SoundCore::clearPlaybackSelection()
{
    m_egoSongPlaying.clear();
    m_wavPlaying.clear();
    m_playingEgoTrack1Path.clear();
    m_playingEgoTrack2Path.clear();
    m_playingEgoModel.clear();
    m_playingEgoIndex = QPersistentModelIndex{};
    m_playingSdModel.clear();
    m_playingSdFilePath.clear();
    emit playingStringChanged();
}

//----------------------Getters/setters-------------------
bool SoundCore::playbackActive() const
{
    return m_soundPlayer.state() != SoundPlayer::Idle;
}

bool SoundCore::playbackPaused() const
{
    return m_soundPlayer.isPaused();
}

bool SoundCore::egoPlaybackActive() const
{
    return m_soundPlayer.state() == SoundPlayer::PlayEgo;
}

bool SoundCore::sdPlaybackActive() const
{
    return m_soundPlayer.state() == SoundPlayer::PlaySd;
}

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

QString SoundCore::egoSongPlaying() const
{
    return m_egoSongPlaying;
}

QString SoundCore::wavPlaying() const
{
    return m_wavPlaying;
}
