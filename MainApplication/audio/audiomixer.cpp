#include "audiomixer.h"
#include <cstring>
#include <limits>
#include <QMutexLocker>
// Disable QIODevice's read buffer: it may prefetch a whole frame and then
// return only the caller's requested byte count, splitting a stereo frame.
AudioMixer::AudioMixer(QObject *parent) : QIODevice(parent)
{
    QIODevice::open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}
void AudioMixer::setTracks(QIODevice *track1, QIODevice *track2)
{
    QMutexLocker locker(&m_mutex);
    m_track1=track1; m_track2=track2;
    m_track1Finished=track1==nullptr; m_track2Finished=track2==nullptr;
    m_positionFrames=0;
}
void AudioMixer::setTrackFinished(bool a, bool b) { QMutexLocker locker(&m_mutex); m_track1Finished=a; m_track2Finished=b; }
void AudioMixer::setTrack1Settings(float v,bool l,bool r,bool m) { QMutexLocker locker(&m_mutex); m_track1Settings={v,l,r,m}; }
void AudioMixer::setTrack2Settings(float v,bool l,bool r,bool m) { QMutexLocker locker(&m_mutex); m_track2Settings={v,l,r,m}; }
bool AudioMixer::seekToPosition(qint64 milliseconds)
{
    QMutexLocker locker(&m_mutex);
    if (milliseconds < 0
        || milliseconds > std::numeric_limits<qint64>::max() / SampleRate)
        return false;

    const qint64 targetFrame = milliseconds * SampleRate / 1000;
    if (targetFrame > std::numeric_limits<qint64>::max() / BytesPerFrame)
        return false;

    const qint64 targetByte = targetFrame * BytesPerFrame;
    const qint64 previousTrack1Position = m_track1 ? m_track1->pos() : 0;
    const qint64 previousTrack2Position = m_track2 ? m_track2->pos() : 0;

    const bool track1Seeked = !m_track1 || m_track1->seek(targetByte);
    const bool track2Seeked = track1Seeked && (!m_track2 || m_track2->seek(targetByte));
    if (!track1Seeked || !track2Seeked)
    {
        if (m_track1)
            m_track1->seek(previousTrack1Position);
        if (m_track2)
            m_track2->seek(previousTrack2Position);
        return false;
    }

    m_positionFrames = targetFrame;
    const qint64 updatedPosition = m_positionFrames * 1000 / SampleRate;
    locker.unlock();
    emit positionUpdated(updatedPosition);
    return true;
}

qint64 AudioMixer::position() const { QMutexLocker locker(&m_mutex); return m_positionFrames*1000/SampleRate; }
qint64 AudioMixer::bytesAvailable() const
{
    QMutexLocker locker(&m_mutex);
    const qint64 a=m_track1?qMax<qint64>(0,m_track1->bytesAvailable()):0;
    const qint64 b=m_track2?qMax<qint64>(0,m_track2->bytesAvailable()):0;
    if(m_track1 && !m_track1Finished && a==0) return 0;
    if(m_track2 && !m_track2Finished && b==0) return 0;
    qint64 available=0;
    if(m_track1 && a>0) available=a;
    if(m_track2 && b>0) available=available==0?b:qMin(available,b);
    return available;
}
qint64 AudioMixer::readData(char *data,qint64 maxSize)
{
    QMutexLocker locker(&m_mutex);
    if(maxSize<BytesPerFrame) return 0;
    const qint64 a=m_track1?qMax<qint64>(0,m_track1->bytesAvailable()):0;
    const qint64 b=m_track2?qMax<qint64>(0,m_track2->bytesAvailable()):0;
    if(m_track1 && !m_track1Finished && a==0) return 0;
    if(m_track2 && !m_track2Finished && b==0) return 0;
    qint64 available=0;
    if(m_track1 && a>0) available=a;
    if(m_track2 && b>0) available=available==0?b:qMin(available,b);
    if(available==0) return 0;
    const qint64 count=qMin(maxSize,available)/BytesPerFrame*BytesPerFrame;
    if(count==0) return 0;
    QByteArray first(static_cast<qsizetype>(count),'\0'), second(static_cast<qsizetype>(count),'\0');
    if(m_track1 && a>0 && m_track1->read(first.data(),count)!=count) return 0;
    if(m_track2 && b>0 && m_track2->read(second.data(),count)!=count) return 0;
    for(qint64 i=0;i<count/sizeof(qint16);++i) {
        qint16 s1=0,s2=0; const qsizetype off=static_cast<qsizetype>(i*sizeof(qint16));
        std::memcpy(&s1,first.constData()+off,sizeof(s1)); std::memcpy(&s2,second.constData()+off,sizeof(s2));
        const bool left=(i%2)==0;
        const bool e1=!m_track1Settings.muted&&(left?m_track1Settings.leftEnabled:m_track1Settings.rightEnabled);
        const bool e2=!m_track2Settings.muted&&(left?m_track2Settings.leftEnabled:m_track2Settings.rightEnabled);
        const qint32 sum=(e1?qint32(s1*m_track1Settings.volume):0)+(e2?qint32(s2*m_track2Settings.volume):0);
        const qint16 out=static_cast<qint16>(qBound<qint32>(static_cast<qint32>(std::numeric_limits<qint16>::min()),
                                                   sum,
                                                   static_cast<qint32>(std::numeric_limits<qint16>::max())));
        std::memcpy(data+off,&out,sizeof(out));
    }
    m_positionFrames+=count/BytesPerFrame;
    const qint64 updatedPosition=m_positionFrames*1000/SampleRate;
    locker.unlock();
    emit positionUpdated(updatedPosition);
    return count;
}
qint64 AudioMixer::writeData(const char *,qint64) { return -1; }
