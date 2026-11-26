#include "soundplayer.h"

#include <utility>

namespace {
QAudioFormat playbackFormat()
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    return format;
}
}

SoundPlayer::SoundPlayer(QObject *parent)
    : SoundPlayer(std::make_unique<QAudioSinkOutput>(playbackFormat()), parent)
{
}

SoundPlayer::SoundPlayer(std::unique_ptr<AudioOutput> audioOutput, QObject *parent)
    : QObject(parent),
      m_format(playbackFormat()),
      m_audioOutput(std::move(audioOutput))
{
    if (!m_audioOutput)
        m_audioOutput = std::make_unique<QAudioSinkOutput>(m_format);

    m_streamTrack1.setFormat(m_format);
    m_streamTrack2.setFormat(m_format);

    connect(&m_streamTrack1, &DecodeStream::durationChanged, this, [this](qint64 duration) {
        m_track1Duration = duration;
        emit track1DurationChanged(duration);
        emit playbackDurationChanged(qMax(m_track1Duration, m_track2Duration));
    });
    connect(&m_streamTrack2, &DecodeStream::durationChanged, this, [this](qint64 duration) {
        m_track2Duration = duration;
        emit track2DurationChanged(duration);
        emit playbackDurationChanged(qMax(m_track1Duration, m_track2Duration));
    });

    connect(&m_streamTrack1, &QIODevice::readyRead,
            this, &SoundPlayer::handleTrackDataAvailable);
    connect(&m_streamTrack2, &QIODevice::readyRead,
            this, &SoundPlayer::handleTrackDataAvailable);
    connect(&m_streamTrack1, &DecodeStream::decodingFinished,
            this, &SoundPlayer::handleTrackFinished);
    connect(&m_streamTrack2, &DecodeStream::decodingFinished,
            this, &SoundPlayer::handleTrackFinished);

    const auto handleDecodeError = [this](const QString &error) {
        if (m_stopping || m_state == State::Idle)
            return;

        const QString message = tr("Audio decoding failed: %1").arg(error);
        stop();
        emit errorOccurred(message);
    };
    connect(&m_streamTrack1, &DecodeStream::errorOccurred, this, handleDecodeError);
    connect(&m_streamTrack2, &DecodeStream::errorOccurred, this, handleDecodeError);

    m_audioOutput->setStateChangedHandler(
        [this](QtAudio::State state) { handlePlayerStateChanged(state); });
    connect(&m_mixer, &AudioMixer::positionUpdated, this, &SoundPlayer::positionUpdated);
}

SoundPlayer::~SoundPlayer()
{
    if (m_audioOutput) {
        m_audioOutput->setStateChangedHandler({});
        m_audioOutput->stop();
    }
}

bool SoundPlayer::play(State mode, const QString &track1Path, const QString &track2Path)
{
    if (mode != PlaySd && mode != PlayEgo)
        return false;

    stop();

    m_hasTrack1 = !track1Path.isEmpty();
    m_hasTrack2 = !track2Path.isEmpty();
    if (!m_hasTrack1 && !m_hasTrack2)
        return false;

    m_track1Duration = 0;
    m_track2Duration = 0;
    m_state = mode;
    updateMixerSettings();

    if (m_hasTrack1 && !m_streamTrack1.open(track1Path))
    {
        stop();
        return false;
    }

    if (m_hasTrack2 && !m_streamTrack2.open(track2Path))
    {
        stop();
        return false;
    }

    m_mixer.setTracks(m_hasTrack1 ? &m_streamTrack1 : nullptr,
                      m_hasTrack2 ? &m_streamTrack2 : nullptr);
    m_mixer.setTrackFinished(!m_hasTrack1 || m_streamTrack1.isDecodingFinished(),
                             !m_hasTrack2 || m_streamTrack2.isDecodingFinished());
    startOutputIfReady();
    emit playbackStateChanged();
    return true;
}

void SoundPlayer::pauseResume()
{
    if (!m_audioOutput || m_state == State::Idle)
        return;

    m_paused = !m_paused;
    if (m_paused)
    {
        if (m_audioOutput->state() == QAudio::ActiveState)
            m_audioOutput->suspend();
        emit playbackStateChanged();
        return;
    }

    if (m_audioOutput->state() == QAudio::SuspendedState)
    {
        m_audioOutput->resume();
        emit playbackStateChanged();
        return;
    }

    if (allTracksFinishedAndDrained())
    {
        finishPlayback();
        return;
    }

    startOutputIfReady();
    emit playbackStateChanged();
}

bool SoundPlayer::isPaused() const { return m_paused; }

void SoundPlayer::stop()
{
    m_stopping = true;
    m_paused = false;
    m_outputStarted = false;
    if (m_audioOutput)
        m_audioOutput->stop();

    m_streamTrack1.close();
    m_streamTrack2.close();
    m_mixer.setTracks(nullptr, nullptr);

    m_hasTrack1 = false;
    m_hasTrack2 = false;
    m_track1Duration = 0;
    m_track2Duration = 0;

    emit track1DurationChanged(0);
    emit track2DurationChanged(0);
    emit playbackDurationChanged(0);
    emit positionUpdated(0);

    m_state = State::Idle;
    m_stopping = false;
    emit playbackStateChanged();
}

void SoundPlayer::setPlayPosition(qint64 msPosition)
{
    if (m_state == State::Idle)
        return;

    if (m_mixer.seekToPosition(msPosition))
        startOutputIfReady();
}

SoundPlayer::State SoundPlayer::state() const
{
    return m_state;
}

void SoundPlayer::setTracksVolume(State mode, float track1Volume, float track2Volume)
{
    if (mode == PlayEgo)
    {
        egoT1Volume = track1Volume;
        egoT2Volume = track2Volume;
    }
    else if (mode == PlaySd)
    {
        sdVolume = track1Volume;
    }

    updateMixerSettings();
}

void SoundPlayer::setTracksEn(State mode, bool track1LeftEnabled, bool track1RightEnabled,
                              bool track2LeftEnabled, bool track2RightEnabled)
{
    if (mode == PlayEgo)
    {
        egoT1LeftEnabled = track1LeftEnabled;
        egoT1RightEnabled = track1RightEnabled;
        egoT2LeftEnabled = track2LeftEnabled;
        egoT2RightEnabled = track2RightEnabled;
    }
    else if (mode == PlaySd)
    {
        sdLeftEnabled = track1LeftEnabled;
        sdRightEnabled = track1RightEnabled;
    }

    updateMixerSettings();
}

void SoundPlayer::setMuted(bool track1Mute, bool track2Mute, bool sdMute)
{
    egoT1Muted = track1Mute;
    egoT2Muted = track2Mute;
    sdMuted = sdMute;
    updateMixerSettings();
}

void SoundPlayer::handlePlayerStateChanged(QtAudio::State state)
{
    if (m_stopping || m_state == State::Idle)
        return;

    switch (state)
    {
    case QAudio::ActiveState:
    case QAudio::SuspendedState:
        break;

    case QAudio::StoppedState:
    {
        const QAudio::Error sinkError = m_audioOutput->error();
        const QString error = sinkError == QAudio::OpenError
                                  ? tr("Unable to open the audio output device.")
                                  : tr("Audio output failed.");
        stop();
        emit errorOccurred(error);
        break;
    }

    case QAudio::IdleState:
        if (allTracksFinishedAndDrained())
            finishPlayback();
        else
            startOutputIfReady();
        break;
    }
}

void SoundPlayer::handleTrackDataAvailable()
{
    startOutputIfReady();
}

void SoundPlayer::handleTrackFinished()
{
    m_mixer.setTrackFinished(!m_hasTrack1 || m_streamTrack1.isDecodingFinished(),
                             !m_hasTrack2 || m_streamTrack2.isDecodingFinished());

    if (!allTracksFinishedAndDrained())
    {
        startOutputIfReady();
        return;
    }

    if (!m_outputStarted
        || (!m_paused && m_audioOutput
            && m_audioOutput->state() != QAudio::ActiveState
            && m_audioOutput->state() != QAudio::SuspendedState))
        finishPlayback();
}

bool SoundPlayer::allTracksFinishedAndDrained() const
{
    const bool track1Done = !m_hasTrack1
                            || (m_streamTrack1.isDecodingFinished()
                                && m_streamTrack1.bytesAvailable() == 0);
    const bool track2Done = !m_hasTrack2
                            || (m_streamTrack2.isDecodingFinished()
                                && m_streamTrack2.bytesAvailable() == 0);
    return track1Done && track2Done;
}

void SoundPlayer::startOutputIfReady()
{
    if (m_stopping || m_state == State::Idle || m_paused || !m_audioOutput
        || m_mixer.bytesAvailable() == 0)
        return;

    if (m_audioOutput->state() == QAudio::ActiveState
        || m_audioOutput->state() == QAudio::SuspendedState)
        return;

    m_outputStarted = true;
    m_audioOutput->start(&m_mixer);
}

void SoundPlayer::finishPlayback()
{
    if (m_state == State::Idle || m_stopping)
        return;

    m_stopping = true;
    m_paused = false;
    m_outputStarted = false;
    if (m_audioOutput)
        m_audioOutput->stop();

    emit positionUpdated(0);
    m_track1Duration = 0;
    m_track2Duration = 0;
    emit playbackDurationChanged(0);
    m_streamTrack1.close();
    m_streamTrack2.close();
    m_mixer.setTracks(nullptr, nullptr);
    m_hasTrack1 = false;
    m_hasTrack2 = false;
    m_state = State::Idle;
    m_stopping = false;
    emit playbackStateChanged();
    emit playingFinished();
}

void SoundPlayer::updateMixerSettings()
{
    const bool egoMode = m_state == PlayEgo;
    m_mixer.setTrack1Settings(egoMode ? egoT1Volume : sdVolume,
                              egoMode ? egoT1LeftEnabled : sdLeftEnabled,
                              egoMode ? egoT1RightEnabled : sdRightEnabled,
                              egoMode ? egoT1Muted : sdMuted);
    m_mixer.setTrack2Settings(egoT2Volume, egoT2LeftEnabled,
                              egoT2RightEnabled, egoT2Muted);
}
