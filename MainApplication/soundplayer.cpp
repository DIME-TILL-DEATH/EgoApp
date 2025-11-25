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
}

SoundPlayer::~SoundPlayer()
{
    // if(m_audioSink)
    // {
    //     delete(m_audioSink);
    //     m_audioSink = nullptr;
    // }
}

void SoundPlayer::play(State mode, const QString& track1Path, const QString& track2Path)
{
    // if(mode != m_state)
    // {
    //     // stop();
    // }
    // else
    // {
    //     pauseResume();
    // }

    m_state = mode;
    QAudioDevice device = QMediaDevices::defaultAudioOutput();

    m_streamTrack1->open(track1Path, QIODevice::ReadOnly);
    if(!track1Path.isEmpty()) m_streamTrack2->open(track2Path, QIODevice::ReadOnly);

    m_format.setSampleRate(44100);
    m_format.setChannelCount(2);
    m_format.setSampleFormat(QAudioFormat::Int16);

    m_audioSink = new QAudioSink(m_format, this);
    m_audioSink->setVolume(0.1);
    connect(m_audioSink, &QAudioSink::stateChanged, this, &SoundPlayer::handlePlayerStateChanged);
    m_audioSink->start(this);
}

void SoundPlayer::pauseResume()
{

}

void SoundPlayer::stop()
{
    if(m_audioSink)
    {
        m_audioSink->stop();
        delete(m_audioSink);
        m_audioSink = nullptr;
    }
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
        if(m_audioSink)
        {
            delete(m_audioSink);
            m_audioSink = nullptr;
        }
        break;
    }
    case QAudio::IdleState:
    {
        if(m_audioSink) m_audioSink->stop();
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

        t1LeftEnabled = egoT1LeftEnabled;
        t1RightEnabled = egoT1RightEnabled;
        t2LeftEnabled = egoT2LeftEnabled;
        t2RightEnabled = egoT2RightEnabled;
        break;
    }
    case PlaySd:
    {
        t1Volume = sdVolume;

        t1LeftEnabled = sdLeftEnabled;
        t1RightEnabled = sdRightEnabled;
        break;
    }
    default: break;
    }

    for (int i = 0; i < numSamples; ++i, ++cursor)
    {
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
        }

        sample1 *= t1Volume;
        sample2 *= t2Volume;

        *cursor = combineSamples(sample1, sample2);
    }

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
    return 16384;
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
        egoT2RightEnabled = track1RightEnabled;
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

