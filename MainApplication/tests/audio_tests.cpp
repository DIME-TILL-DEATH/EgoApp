#include <QAudioFormat>
#include <QBuffer>
#include <QDataStream>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest>
#include <cstring>
#include <initializer_list>
#include <memory>
#include "audiooutput.h"
#include "audiomixer.h"
#include "decodestream.h"
#include "soundplayer.h"

#include <utility>


class FakeAudioOutput final : public AudioOutput
{
public:
    void setStateChangedHandler(StateChangedHandler handler) override { m_handler = std::move(handler); }
    QtAudio::State state() const override { return m_state; }
    QAudio::Error error() const override { return m_error; }
    void start(QIODevice *source) override { m_source = source; ++m_startCount; setState(QtAudio::ActiveState); }
    void stop() override { m_source = nullptr; setState(QtAudio::IdleState); }
    void suspend() override { setState(QtAudio::SuspendedState); }
    void resume() override { setState(QtAudio::ActiveState); }
    int startCount() const { return m_startCount; }
    QByteArray consumeAvailable() { return m_source ? m_source->readAll() : QByteArray{}; }
    void setError(QAudio::Error error) { m_error = error; }
    void simulateState(QtAudio::State state) { setState(state); }
private:
    void setState(QtAudio::State state) { m_state = state; if (m_handler) m_handler(state); }
    StateChangedHandler m_handler;
    QIODevice *m_source{nullptr};
    QtAudio::State m_state{QtAudio::IdleState};
    QAudio::Error m_error{QAudio::NoError};
    int m_startCount{0};
};

class AudioTests final : public QObject
{
    Q_OBJECT
private:
    static QByteArray pcm(std::initializer_list<qint16> samples)
    {
        QByteArray result(static_cast<qsizetype>(samples.size() * sizeof(qint16)), '\0');
        qsizetype offset = 0;
        for (qint16 sample : samples) {
            std::memcpy(result.data() + offset, &sample, sizeof(sample));
            offset += sizeof(sample);
        }
        return result;
    }
    static qint16 sampleAt(const QByteArray &data, qsizetype index)
    {
        qint16 sample = 0;
        std::memcpy(&sample, data.constData() + index * sizeof(qint16), sizeof(sample));
        return sample;
    }
    static std::unique_ptr<QTemporaryFile> makeWaveFile(int durationMs)
    {
        constexpr quint32 sampleRate = 44100;
        constexpr quint16 channels = 2;
        constexpr quint16 bitsPerSample = 16;
        constexpr quint32 bytesPerFrame = channels * bitsPerSample / 8;
        const quint32 dataSize = static_cast<quint32>(
            quint64(sampleRate) * bytesPerFrame * quint64(durationMs) / 1000);
        auto file = std::make_unique<QTemporaryFile>();
        if (!file->open())
            return {};
        QDataStream stream(file.get());
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF", 4);
        stream << quint32(36 + dataSize);
        stream.writeRawData("WAVEfmt ", 8);
        stream << quint32(16) << quint16(1) << channels << sampleRate
               << quint32(sampleRate * bytesPerFrame)
               << quint16(bytesPerFrame) << bitsPerSample;
        stream.writeRawData("data", 4);
        stream << dataSize;
        if (stream.status() != QDataStream::Ok)
            return {};
        if (file->write(QByteArray(static_cast<qsizetype>(dataSize), '\0'))
                != static_cast<qint64>(dataSize) || !file->flush())
            return {};
        return file;
    }
    static QAudioFormat pcmFormat()
    {
        QAudioFormat format;
        format.setSampleRate(44100);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Int16);
        return format;
    }

private slots:
    void waitsUntilBothActiveTracksHaveData()
    {
        QBuffer track1, track2;
        track1.setData(pcm({100, 100, 100, 100}));
        track2.setData({});
        QVERIFY(track1.open(QIODevice::ReadOnly));
        QVERIFY(track2.open(QIODevice::ReadOnly));
        AudioMixer mixer;
        mixer.setTracks(&track1, &track2);
        mixer.setTrackFinished(false, false);
        QCOMPARE(mixer.bytesAvailable(), qint64(0));
        char output[8]{};
        QCOMPARE(mixer.read(output, sizeof(output)), qint64(0));
        track2.close();
        track2.setData(pcm({200, 200, 200, 200}));
        QVERIFY(track2.open(QIODevice::ReadOnly));
        QCOMPARE(mixer.read(output, sizeof(output)), qint64(sizeof(output)));
        const QByteArray mixed(output, sizeof(output));
        QCOMPARE(sampleAt(mixed, 0), qint16(300));
        QCOMPARE(sampleAt(mixed, 1), qint16(300));
        QCOMPARE(sampleAt(mixed, 2), qint16(300));
        QCOMPARE(sampleAt(mixed, 3), qint16(300));
    }
    void drainsFinishedTrackThenContinuesWithSilence()
    {
        QBuffer shorter, longer;
        shorter.setData(pcm({100, 100, 100, 100}));
        longer.setData(pcm({200, 200, 200, 200, 200, 200, 200, 200}));
        QVERIFY(shorter.open(QIODevice::ReadOnly));
        QVERIFY(longer.open(QIODevice::ReadOnly));
        AudioMixer mixer;
        mixer.setTracks(&shorter, &longer);
        mixer.setTrackFinished(true, false);
        char first[8]{};
        QCOMPARE(mixer.read(first, sizeof(first)), qint64(sizeof(first)));
        QCOMPARE(sampleAt(QByteArray(first, sizeof(first)), 0), qint16(300));
        char second[8]{};
        QCOMPARE(mixer.read(second, sizeof(second)), qint64(sizeof(second)));
        const QByteArray tail(second, sizeof(second));
        QCOMPARE(sampleAt(tail, 0), qint16(200));
        QCOMPARE(sampleAt(tail, 1), qint16(200));
        QCOMPARE(sampleAt(tail, 2), qint16(200));
        QCOMPARE(sampleAt(tail, 3), qint16(200));
    }
    void supportsTrackTwoOnly()
    {
        QBuffer track2;
        track2.setData(pcm({-50, 50, -100, 100}));
        QVERIFY(track2.open(QIODevice::ReadOnly));
        AudioMixer mixer;
        mixer.setTracks(nullptr, &track2);
        mixer.setTrackFinished(true, false);
        char output[8]{};
        QCOMPARE(mixer.read(output, sizeof(output)), qint64(sizeof(output)));
        const QByteArray result(output, sizeof(output));
        QCOMPARE(sampleAt(result, 0), qint16(-50));
        QCOMPARE(sampleAt(result, 1), qint16(50));
        QCOMPARE(sampleAt(result, 2), qint16(-100));
        QCOMPARE(sampleAt(result, 3), qint16(100));
    }
    void appliesVolumeMuteAndChannelRouting()
    {
        QBuffer first, second;
        first.setData(pcm({1000, 1000, 1000, 1000}));
        second.setData(pcm({2000, 2000, 2000, 2000}));
        QVERIFY(first.open(QIODevice::ReadOnly));
        QVERIFY(second.open(QIODevice::ReadOnly));
        AudioMixer mixer;
        mixer.setTracks(&first, &second);
        mixer.setTrackFinished(false, false);
        mixer.setTrack1Settings(0.5f, true, false, false);
        mixer.setTrack2Settings(1.0f, true, true, true);
        char output[8]{};
        QCOMPARE(mixer.read(output, sizeof(output)), qint64(sizeof(output)));
        const QByteArray result(output, sizeof(output));
        QCOMPARE(sampleAt(result, 0), qint16(500));
        QCOMPARE(sampleAt(result, 1), qint16(0));
        QCOMPARE(sampleAt(result, 2), qint16(500));
        QCOMPARE(sampleAt(result, 3), qint16(0));
    }
    void seekUpdatesBothStreamsAndPosition()
    {
        const QByteArray samples(50000, '\0');
        QBuffer first, second;
        first.setData(samples);
        second.setData(samples);
        QVERIFY(first.open(QIODevice::ReadOnly));
        QVERIFY(second.open(QIODevice::ReadOnly));
        AudioMixer mixer;
        mixer.setTracks(&first, &second);
        QSignalSpy positionSpy(&mixer, &AudioMixer::positionUpdated);
        QVERIFY(positionSpy.isValid());
        QVERIFY(mixer.seekToPosition(100));
        QCOMPARE(first.pos(), qint64(44100 * 4 / 10));
        QCOMPARE(second.pos(), qint64(44100 * 4 / 10));
        QCOMPARE(mixer.position(), qint64(100));
        QVERIFY(positionSpy.count() >= 1);
        QCOMPARE(positionSpy.last().at(0).toLongLong(), qint64(100));
    }
    void reportsConsumedPlaybackPosition()
    {
        const QByteArray samples(176400, '\0');
        QBuffer first, second;
        first.setData(samples);
        second.setData(samples);
        QVERIFY(first.open(QIODevice::ReadOnly));
        QVERIFY(second.open(QIODevice::ReadOnly));
        AudioMixer mixer;
        mixer.setTracks(&first, &second);
        QSignalSpy positionSpy(&mixer, &AudioMixer::positionUpdated);
        QVERIFY(positionSpy.isValid());
        QByteArray output(samples.size(), '\0');
        QCOMPARE(mixer.read(output.data(), output.size()), qint64(output.size()));
        QCOMPARE(mixer.position(), qint64(1000));
        QVERIFY(positionSpy.count() >= 1);
        QCOMPARE(positionSpy.last().at(0).toLongLong(), qint64(1000));
    }
    void pcmSpoolIsLimitedToOneHourPerTrack()
    {
        QCOMPARE(DecodeStream::MaxPcmBytes, qint64(635040000));
    }
    void missingSourceFailsWithoutOpeningStream()
    {
        DecodeStream decoder;
        decoder.setFormat(pcmFormat());
        QVERIFY(!decoder.open(QStringLiteral("/path/that/does/not/exist.wav")));
        QVERIFY(!decoder.isOpen());
        QVERIFY(decoder.isDecodingFinished());
    }
    void stopDuringDecodeAndRestartIgnoresOldSession()
    {
        auto longWave = makeWaveFile(30000);
        auto shortWave = makeWaveFile(250);
        QVERIFY(longWave);
        QVERIFY(shortWave);
        DecodeStream decoder;
        decoder.setFormat(pcmFormat());
        QSignalSpy finishedSpy(&decoder, &DecodeStream::decodingFinished);
        QSignalSpy errorSpy(&decoder, &DecodeStream::errorOccurred);
        QVERIFY(finishedSpy.isValid());
        QVERIFY(errorSpy.isValid());
        QVERIFY(decoder.open(longWave->fileName()));
        decoder.close();
        QTest::qWait(100);
        QCOMPARE(finishedSpy.count(), 0);
        QCOMPARE(errorSpy.count(), 0);
        QVERIFY(!decoder.isOpen());
        QVERIFY(decoder.open(shortWave->fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() == 1 || errorSpy.count() > 0, 5000);
        QCOMPARE(errorSpy.count(), 0);
        QCOMPARE(finishedSpy.count(), 1);
        QVERIFY(decoder.isDecodingFinished());
    }
    void decoderFailureIsNotReportedAsSuccessfulCompletion()
    {
        QTemporaryFile invalidSource;
        QVERIFY(invalidSource.open());
        QVERIFY(invalidSource.write("not an audio file") > 0);
        QVERIFY(invalidSource.flush());
        DecodeStream decoder;
        decoder.setFormat(pcmFormat());
        QSignalSpy finishedSpy(&decoder, &DecodeStream::decodingFinished);
        QSignalSpy errorSpy(&decoder, &DecodeStream::errorOccurred);
        QVERIFY(decoder.open(invalidSource.fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(errorSpy.count() > 0, 5000);
        QCOMPARE(finishedSpy.count(), 0);
        QVERIFY(decoder.isDecodingFinished());
    }
    void soundPlayerStartsOutputAfterDecodedDataArrives()
    {
        auto wave = makeWaveFile(250); QVERIFY(wave);
        auto output = std::make_unique<FakeAudioOutput>(); auto *fake = output.get();
        SoundPlayer player(std::move(output));
        QSignalSpy errors(&player, &SoundPlayer::errorOccurred); QVERIFY(errors.isValid());
        QVERIFY(player.play(SoundPlayer::PlaySd, wave->fileName()));
        QCOMPARE(player.state(), SoundPlayer::PlaySd);
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QCOMPARE(errors.count(), 0); player.stop();
    }

    void soundPlayerPauseBeforeDataPreventsOutputStart()
    {
        auto wave = makeWaveFile(30000); QVERIFY(wave);
        auto output = std::make_unique<FakeAudioOutput>(); auto *fake = output.get();
        SoundPlayer player(std::move(output));
        QSignalSpy duration(&player, &SoundPlayer::track1DurationChanged); QVERIFY(duration.isValid());
        QVERIFY(player.play(SoundPlayer::PlaySd, wave->fileName()));
        player.pauseResume(); QVERIFY(player.isPaused());
        QTRY_VERIFY_WITH_TIMEOUT(duration.count() > 0, 5000);
        QTest::qWait(100); QCOMPARE(fake->startCount(), 0);
        player.pauseResume(); QVERIFY(!player.isPaused());
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000); player.stop();
    }

    void soundPlayerPauseAndResumeControlsOutput()
    {
        auto wave = makeWaveFile(30000); QVERIFY(wave);
        auto output = std::make_unique<FakeAudioOutput>(); auto *fake = output.get();
        SoundPlayer player(std::move(output));
        QVERIFY(player.play(SoundPlayer::PlaySd, wave->fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        player.pauseResume(); QVERIFY(player.isPaused());
        QCOMPARE(fake->state(), QtAudio::SuspendedState);
        player.pauseResume(); QVERIFY(!player.isPaused());
        QCOMPARE(fake->state(), QtAudio::ActiveState); player.stop();
    }

    void soundPlayerCanStopAndRestartWithAnotherSource()
    {
        auto first = makeWaveFile(30000); auto second = makeWaveFile(250);
        QVERIFY(first); QVERIFY(second);
        auto output = std::make_unique<FakeAudioOutput>(); auto *fake = output.get();
        SoundPlayer player(std::move(output));
        QSignalSpy errors(&player, &SoundPlayer::errorOccurred); QVERIFY(errors.isValid());
        QVERIFY(player.play(SoundPlayer::PlaySd, first->fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        player.stop(); QCOMPARE(player.state(), SoundPlayer::Idle);
        QVERIFY(player.play(SoundPlayer::PlayEgo, second->fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() >= 2, 5000);
        QCOMPARE(player.state(), SoundPlayer::PlayEgo); QCOMPARE(errors.count(), 0); player.stop();
    }

    void soundPlayerOutputErrorStopsPlayback()
    {
        auto wave = makeWaveFile(30000); QVERIFY(wave);
        auto output = std::make_unique<FakeAudioOutput>(); auto *fake = output.get();
        SoundPlayer player(std::move(output));
        QSignalSpy errors(&player, &SoundPlayer::errorOccurred); QVERIFY(errors.isValid());
        QVERIFY(player.play(SoundPlayer::PlaySd, wave->fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        fake->setError(QAudio::OpenError); fake->simulateState(QtAudio::StoppedState);
        QCOMPARE(player.state(), SoundPlayer::Idle); QCOMPARE(errors.count(), 1);
        QVERIFY(errors.first().first().toString().contains(QStringLiteral("Unable to open the audio output device.")));
    }

    void soundPlayerFinishesAfterOutputBufferIsDrained()
    {
        auto wave = makeWaveFile(100); QVERIFY(wave);
        auto output = std::make_unique<FakeAudioOutput>(); auto *fake = output.get();
        SoundPlayer player(std::move(output));
        QSignalSpy finished(&player, &SoundPlayer::playingFinished); QVERIFY(finished.isValid());
        QVERIFY(player.play(SoundPlayer::PlaySd, wave->fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QByteArray consumed;
        QTRY_VERIFY_WITH_TIMEOUT([&]() { consumed += fake->consumeAvailable(); return consumed.size() >= 44100 * 4 / 10; }(), 5000);
        QTest::qWait(100); fake->simulateState(QtAudio::IdleState);
        QTRY_COMPARE_WITH_TIMEOUT(player.state(), SoundPlayer::Idle, 5000);
        QCOMPARE(finished.count(), 1);
    }

};
QTEST_GUILESS_MAIN(AudioTests)
#include "audio_tests.moc"
