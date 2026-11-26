#include "soundplayer.h"

#include <QDebug>

#include <QAudioDevice>
#include <QMediaDevices>
#include <QAudioOutput>
#include <QAudioSink>

SoundPlayer::SoundPlayer(QObject *parent)
    : QIODevice{parent}
{
    setOpenMode(QIODevice::ReadOnly);

    m_streamTrack1 = new DecodeStream;
    m_streamTrack2 = new DecodeStream;

    m_format.setSampleRate(44100);
    m_format.setChannelCount(2);
    m_format.setSampleFormat(QAudioFormat::Int16);

    m_streamTrack1->setFormat(m_format);
    m_streamTrack2->setFormat(m_format);

    connect(m_streamTrack1, &DecodeStream::durationChanged, this, [this](qint64 duration) {
        m_track1Duration = duration;
        emit track1DurationChanged(duration);
        emit playbackDurationChanged(qMax(m_track1Duration, m_track2Duration));
    });
    connect(m_streamTrack2, &DecodeStream::durationChanged, this, [this](qint64 duration) {
        m_track2Duration = duration;
        emit track2DurationChanged(duration);
        emit playbackDurationChanged(qMax(m_track1Duration, m_track2Duration));
    });

    const auto handleDecodeError = [this](const QString &error) {
        if (m_stopping || m_state == State::Idle)
            return;

        const QString message = QObject::tr("Audio decoding failed: %1").arg(error);
        stop();
        emit errorOccurred(message);
    };
    connect(m_streamTrack1, &DecodeStream::errorOccurred, this, handleDecodeError);
    connect(m_streamTrack2, &DecodeStream::errorOccurred, this, handleDecodeError);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    m_audioSink = new QAudioSink(device, m_format, this);
    connect(m_audioSink, &QAudioSink::stateChanged, this, &SoundPlayer::handlePlayerStateChanged);
}

SoundPlayer::~SoundPlayer()
{
    if(m_audioSink) delete(m_audioSink);
}

bool SoundPlayer::play(State mode, const QString& track1Path, const QString& track2Path)
{
    stop();

    const bool hasTrack1 = !track1Path.isEmpty();
    const bool hasTrack2 = !track2Path.isEmpty();
    if (!hasTrack1 && !hasTrack2)
        return false;

    m_track1Duration = 0;
    m_track2Duration = 0;

    if (hasTrack1 && !m_streamTrack1->open(track1Path, QIODevice::ReadOnly))
    {
        stop();
        return false;
    }
    if (hasTrack2 && !m_streamTrack2->open(track2Path, QIODevice::ReadOnly))
    {
        stop();
        return false;
    }

    m_state = mode;
    samplePos = 0;
    m_audioSink->start(this);
    return true;
}

void SoundPlayer::pauseResume()
{

}

void SoundPlayer::stop()
{
    m_stopping = true;
    m_audioSink->reset();

    m_streamTrack1->close();
    m_streamTrack2->close();

    m_track1Duration = 0;
    m_track2Duration = 0;
    emit track1DurationChanged(0);
    emit track2DurationChanged(0);
    emit playbackDurationChanged(0);

    samplePos = 0;
    emit positionUpdated(samplePos * 1000/44100);

    m_state = State::Idle;
    m_stopping = false;
}

void SoundPlayer::setPlayPosition(qint64 msPosition)
{
    samplePos = 44100/1000 * msPosition;

    m_streamTrack1->seek(samplePos * m_format.channelCount() * m_format.bytesPerSample());
    m_streamTrack2->seek(samplePos * m_format.channelCount() * m_format.bytesPerSample());
}

void SoundPlayer::handlePlayerStateChanged(QtAudio::State state)
{
    qDebug() << __FUNCTION__ << state;
    switch(state)
    {

    case QAudio::ActiveState: break;
    case QAudio::SuspendedState: break;
    case QAudio::StoppedState:
    {
        break;
    }
    case QAudio::IdleState:
    {
        // m_streamTrack1->close();
        // m_streamTrack2->close();

        samplePos = 0;
        emit positionUpdated(samplePos * 1000/44100);

        // m_state = State::Idle;
        emit playingFinished();
        break;
    }
    }
}

SoundPlayer::State SoundPlayer::state() const
{
    return m_state;
}

qint64 SoundPlayer::readData(char *data, qint64 maxSize)
{
    memset(data, 0, maxSize);

    constexpr qint16 bitDepth = sizeof(qint16);
    const qint16 numSamples = maxSize / bitDepth;


    auto *cursor = reinterpret_cast<qint16 *>(data);
    qint16 sample1 = 0;
    qint16 sample2 = 0;

    float t1Volume = 0.0;
    float t2Volume = 0.0;

    bool t1LeftEnabled = false;
    bool t1RightEnabled = false;
    bool t2LeftEnabled = false;
    bool t2RightEnabled = false;

    switch(m_state)
    {
    case PlayEgo:
    {
        t1Volume = egoT1Volume;
        t2Volume = egoT2Volume;

        t1LeftEnabled = egoT1LeftEnabled & !egoT1Muted;
        t1RightEnabled = egoT1RightEnabled & !egoT1Muted;
        t2LeftEnabled = egoT2LeftEnabled & !egoT2Muted;
        t2RightEnabled = egoT2RightEnabled & !egoT2Muted;
        break;
    }
    case PlaySd:
    {
        t1Volume = sdVolume;

        t1LeftEnabled = sdLeftEnabled & !sdMuted;
        t1RightEnabled = sdRightEnabled & !sdMuted;
        break;
    }
    default: break;
    }

    for (int i = 0; i < numSamples; ++i, ++cursor)
    {
        // Either stream may be absent or may finish before the other. Start
        // each sample at silence so an exhausted stream cannot repeat stale data.
        sample1 = 0;
        sample2 = 0;
        m_streamTrack1->read(reinterpret_cast<char *>(&sample1), bitDepth);
        m_streamTrack2->read(reinterpret_cast<char *>(&sample2), bitDepth);

        if(i%2 == 0)
        {
            //left
            sample1 *= t1LeftEnabled;
            sample2 *= t2LeftEnabled;
        }
        else
        {
            //right
            sample1 *= t1RightEnabled;
            sample2 *= t2RightEnabled;

            samplePos++;
        }

        sample1 *= t1Volume;
        sample2 *= t2Volume;

        *cursor = combineSamples(sample1, sample2);
    }

    qint64 msPos = samplePos * 1000/44100;
    emit positionUpdated(msPos);

    return maxSize;
}

qint64 SoundPlayer::writeData([[maybe_unused]]const char *data,
                              [[maybe_unused]]qint64 maxSize)
{
    Q_ASSERT_X(false, "writeData", "not implemented");
    return 0;
}

qint64 SoundPlayer::bytesAvailable() const
{
    return qMax(m_streamTrack1->bytesAvailable(), m_streamTrack2->bytesAvailable())
           + QIODevice::bytesAvailable();
}

void SoundPlayer::setTracksVolume(State mode, float track1Volume, float track2Volume)
{
    switch(mode)
    {
    case PlayEgo:
    {
        egoT1Volume = track1Volume;
        egoT2Volume = track2Volume;
        break;
    }
    case PlaySd:
    {
        sdVolume = track1Volume;
        break;
    }
    default: break;
    }
}

void SoundPlayer::setTracksEn(State mode, bool track1LeftEnabled, bool track1RightEnabled, bool track2LeftEnabled, bool track2RightEnabled)
{
    switch(mode)
    {
    case PlayEgo:
    {
        egoT1LeftEnabled = track1LeftEnabled;
        egoT1RightEnabled = track1RightEnabled;
        egoT2LeftEnabled = track2LeftEnabled;
        egoT2RightEnabled = track2RightEnabled;
        break;
    }
    case PlaySd:
    {
        sdLeftEnabled = track1LeftEnabled;
        sdRightEnabled = track1RightEnabled;
        break;
    }
    default: break;
    }
}

void SoundPlayer::setMuted(bool track1Mute, bool track2Mute, bool sdTrackMute)
{
    egoT1Muted = track1Mute;
    egoT2Muted = track2Mute;
    sdMuted = sdTrackMute;
}

