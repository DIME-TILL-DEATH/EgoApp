#include <QBuffer>
#include <QSignalSpy>
#include <QtTest>
#include <cstring>
#include <initializer_list>

#include "audiomixer.h"

class AudioMixerTests final : public QObject
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
        QCOMPARE(positionSpy.last().at(0).toLongLong(), qint64(1000));
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
};

QTEST_GUILESS_MAIN(AudioMixerTests)
#include "audio_tests.moc"
