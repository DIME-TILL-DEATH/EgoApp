#include <QAudioFormat>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>
#include <utility>

#include "audiooutput.h"
#include "soundcore.h"
#include "sdcontentmodel.h"
#include "playlists/playlistmodel.h"
#include "playlists/song.h"

class FakeAudioOutput final : public AudioOutput
{
public:
    void setStateChangedHandler(StateChangedHandler handler) override { m_handler = std::move(handler); }
    QtAudio::State state() const override { return m_state; }
    QAudio::Error error() const override { return m_error; }
    void start(QIODevice *source) override { m_source = source; ++m_startCount; setState(QtAudio::ActiveState); }
    void stop() override { setState(QtAudio::IdleState); }
    void suspend() override { setState(QtAudio::SuspendedState); }
    void resume() override { setState(QtAudio::ActiveState); }
    int startCount() const { return m_startCount; }
    qint64 consume(qint64 bytes) { return m_source ? m_source->read(bytes).size() : 0; }
private:
    void setState(QtAudio::State state) { m_state = state; if (m_handler) m_handler(state); }
    StateChangedHandler m_handler;
    QIODevice *m_source{nullptr};
    QtAudio::State m_state{QtAudio::IdleState};
    QAudio::Error m_error{QAudio::NoError};
    int m_startCount{0};
};

class SoundCoreTests final : public QObject
{
    Q_OBJECT
private:
    static bool writeWave(const QString &path, int durationMs = 3000)
    {
        constexpr quint32 sampleRate = 44100;
        constexpr quint16 channels = 2;
        constexpr quint16 bitsPerSample = 16;
        constexpr quint32 bytesPerFrame = channels * bitsPerSample / 8;
        const quint32 dataSize = static_cast<quint32>(
            quint64(sampleRate) * bytesPerFrame * quint64(durationMs) / 1000);

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        QDataStream stream(&file);
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
            return false;
        return file.write(QByteArray(static_cast<qsizetype>(dataSize), '\0'))
                   == static_cast<qint64>(dataSize)
               && file.flush();
    }

    static Song makeSong(const QString &track1, const QString &track2 = {})
    {
        Song song;
        song.t1Path = track1;
        song.t2Path = track2;
        return song;
    }

    static QModelIndex proxyIndex(SdContentModel &model, const QString &path)
    {
        return model.sdProxyModel()->mapFromSource(model.index(path));
    }

private slots:
    void sdPlaybackUsesTheRequestedProxyIndexNotStaleSelection()
    {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QVERIFY(QDir(workspace.path()).mkdir("SONGS"));
        const QString firstPath = workspace.filePath("SONGS/first.wav");
        const QString secondPath = workspace.filePath("SONGS/second.wav");
        QVERIFY(writeWave(firstPath));
        QVERIFY(writeWave(secondPath));

        SdContentModel model;
        model.setWorkspace(QDir(workspace.path()));
        QTRY_VERIFY_WITH_TIMEOUT(proxyIndex(model, firstPath).isValid(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(proxyIndex(model, secondPath).isValid(), 5000);
        const QModelIndex firstIndex = proxyIndex(model, firstPath);
        const QModelIndex secondIndex = proxyIndex(model, secondPath);

        model.currentSelectionChanged(firstIndex);
        QCOMPARE(model.selectedPath(), QStringLiteral("/SONGS/first.wav"));

        auto output = std::make_unique<FakeAudioOutput>();
        auto *fake = output.get();
        SoundCore core(std::move(output));
        core.PlayPause(SoundPlayer::PlaySd, secondIndex);

        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QCOMPARE(core.wavPlaying(), QStringLiteral("/SONGS/second.wav"));
        QVERIFY(core.playbackActive());
        QVERIFY(!core.playbackPaused());
    }

    void sdSelectionClearsSelectedPathForNonWavAndInvalidIndexes()
    {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QVERIFY(QDir(workspace.path()).mkdir("SONGS"));
        QVERIFY(QDir(workspace.path() + "/SONGS").mkdir("Folder"));
        const QString wavPath = workspace.filePath("SONGS/track.wav");
        QVERIFY(writeWave(wavPath));

        SdContentModel model;
        model.setWorkspace(QDir(workspace.path()));
        QTRY_VERIFY_WITH_TIMEOUT(proxyIndex(model, wavPath).isValid(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(
            proxyIndex(model, workspace.filePath("SONGS/Folder")).isValid(), 5000);
        const QModelIndex wavIndex = proxyIndex(model, wavPath);
        const QModelIndex folderIndex = proxyIndex(model, workspace.filePath("SONGS/Folder"));

        model.currentSelectionChanged(wavIndex);
        QCOMPARE(model.selectedPath(), QStringLiteral("/SONGS/track.wav"));
        QVERIFY(model.canSetTrack());

        model.currentSelectionChanged(folderIndex);
        QVERIFY(model.selectedPath().isEmpty());
        QVERIFY(!model.canSetTrack());

        model.currentSelectionChanged(QModelIndex{});
        QVERIFY(model.selectedPath().isEmpty());
        QVERIFY(!model.canSetTrack());
    }

    void playlistPlaybackTogglesOnlyForTheSameModelAndRow()
    {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QVERIFY(QDir(workspace.path()).mkdir("SONGS"));
        const QString audioPath = workspace.filePath("SONGS/shared.wav");
        QVERIFY(writeWave(audioPath));

        const Song song = makeSong("/SONGS/shared.wav");
        PlaylistModel firstModel({song, song});
        PlaylistModel secondModel({song});
        firstModel.setWorkspacePath(workspace.path());
        secondModel.setWorkspacePath(workspace.path());
        const QModelIndex firstIndex = firstModel.index(0, 0);
        const QModelIndex anotherRowInFirstModel = firstModel.index(1, 0);
        const QModelIndex secondIndex = secondModel.index(0, 0);

        auto output = std::make_unique<FakeAudioOutput>();
        auto *fake = output.get();
        SoundCore core(std::move(output));

        QSignalSpy stateChanges(&core, &SoundCore::playbackStateChanged);
        QVERIFY(stateChanges.isValid());

        core.PlayPause(SoundPlayer::PlayEgo, firstIndex);
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QVERIFY(core.playbackActive());

        core.PlayPause(SoundPlayer::PlayEgo, anotherRowInFirstModel);
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() >= 2, 5000);
        QVERIFY(core.playbackActive());
        QVERIFY(!core.playbackPaused());

        core.PlayPause(SoundPlayer::PlayEgo, secondIndex);
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() >= 3, 5000);
        QVERIFY(core.playbackActive());
        QVERIFY(!core.playbackPaused());
        QCOMPARE(core.egoSongPlaying(), QStringLiteral("shared.wav"));
        QVERIFY(stateChanges.count() >= 3);
    }

    void playPauseSynchronizesQmlPlaybackProperties()
    {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QVERIFY(QDir(workspace.path()).mkdir("SONGS"));
        const QString audioPath = workspace.filePath("SONGS/track.wav");
        QVERIFY(writeWave(audioPath));

        PlaylistModel model({makeSong("/SONGS/track.wav")});
        model.setWorkspacePath(workspace.path());
        const QModelIndex index = model.index(0, 0);

        auto output = std::make_unique<FakeAudioOutput>();
        auto *fake = output.get();
        SoundCore core(std::move(output));
        QSignalSpy stateChanges(&core, &SoundCore::playbackStateChanged);
        QVERIFY(stateChanges.isValid());

        core.PlayPause(SoundPlayer::PlayEgo, index);
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QVERIFY(core.playbackActive());
        QVERIFY(!core.playbackPaused());

        core.PlayPause(SoundPlayer::PlayEgo, index);
        QVERIFY(core.playbackActive());
        QVERIFY(core.playbackPaused());

        core.PlayPause(SoundPlayer::PlayEgo, index);
        QVERIFY(core.playbackActive());
        QVERIFY(!core.playbackPaused());
        QVERIFY(stateChanges.count() >= 3);
    }


    void playbackActiveIsScopedToItsSourcePanel()
    {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QVERIFY(QDir(workspace.path()).mkdir("SONGS"));
        const QString playlistPath = workspace.filePath("SONGS/playlist.wav");
        const QString sdPath = workspace.filePath("SONGS/sd.wav");
        QVERIFY(writeWave(playlistPath));
        QVERIFY(writeWave(sdPath));

        PlaylistModel playlist({makeSong("/SONGS/playlist.wav")});
        playlist.setWorkspacePath(workspace.path());

        SdContentModel sdModel;
        sdModel.setWorkspace(QDir(workspace.path()));
        QTRY_VERIFY_WITH_TIMEOUT(proxyIndex(sdModel, sdPath).isValid(), 5000);
        const QModelIndex sdIndex = proxyIndex(sdModel, sdPath);

        auto output = std::make_unique<FakeAudioOutput>();
        auto *fake = output.get();
        SoundCore core(std::move(output));

        QVERIFY(!core.egoPlaybackActive());
        QVERIFY(!core.sdPlaybackActive());

        core.PlayPause(SoundPlayer::PlayEgo, playlist.index(0, 0));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QVERIFY(core.egoPlaybackActive());
        QVERIFY(!core.sdPlaybackActive());
        QVERIFY(!core.playbackPaused());

        core.PlayPause(SoundPlayer::PlayEgo, playlist.index(0, 0));
        QVERIFY(core.egoPlaybackActive());
        QVERIFY(!core.sdPlaybackActive());
        QVERIFY(core.playbackPaused());

        core.PlayPause(SoundPlayer::PlaySd, sdIndex);
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() >= 2, 5000);
        QVERIFY(!core.egoPlaybackActive());
        QVERIFY(core.sdPlaybackActive());
        QVERIFY(!core.playbackPaused());

        core.PlayPause(SoundPlayer::PlaySd, sdIndex);
        QVERIFY(!core.egoPlaybackActive());
        QVERIFY(core.sdPlaybackActive());
        QVERIFY(core.playbackPaused());

        core.stop();
        QVERIFY(!core.egoPlaybackActive());
        QVERIFY(!core.sdPlaybackActive());
    }

    void playlistPlaybackUpdatesQmlProgressProperties()
    {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QVERIFY(QDir(workspace.path()).mkdir("SONGS"));
        QVERIFY(writeWave(workspace.filePath("SONGS/track.wav")));

        PlaylistModel model({makeSong("/SONGS/track.wav")});
        model.setWorkspacePath(workspace.path());

        auto output = std::make_unique<FakeAudioOutput>();
        auto *fake = output.get();
        SoundCore core(std::move(output));
        QSignalSpy positionChanges(&core, &SoundCore::qmlTrackPositionChanged);
        QVERIFY(positionChanges.isValid());

        core.playEgo(model.index(0, 0));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(core.egoTrackDuration() > 0, 5000);
        QVERIFY(fake->consume(4096) > 0);
        QTRY_VERIFY_WITH_TIMEOUT(core.egoTrackPosition() > 0, 1000);
        QCOMPARE(core.sdTrackPosition(), qint64(0));
        QVERIFY(positionChanges.count() > 0);
    }

    void sdPlaybackUpdatesQmlProgressProperties()
    {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QVERIFY(QDir(workspace.path()).mkdir("SONGS"));
        const QString path = workspace.filePath("SONGS/track.wav");
        QVERIFY(writeWave(path));

        SdContentModel model;
        model.setWorkspace(QDir(workspace.path()));
        QTRY_VERIFY_WITH_TIMEOUT(proxyIndex(model, path).isValid(), 5000);

        auto output = std::make_unique<FakeAudioOutput>();
        auto *fake = output.get();
        SoundCore core(std::move(output));
        QSignalSpy positionChanges(&core, &SoundCore::qmlTrackPositionChanged);
        QVERIFY(positionChanges.isValid());

        core.playContent(proxyIndex(model, path));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(core.sdTrackDuration() > 0, 5000);
        QVERIFY(fake->consume(4096) > 0);
        QTRY_VERIFY_WITH_TIMEOUT(core.sdTrackPosition() > 0, 1000);
        QCOMPARE(core.egoTrackPosition(), qint64(0));
        QVERIFY(positionChanges.count() > 0);
    }

    void switchingFromSdToPlaylistClearsThePreviousSourceMetadata()
    {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QVERIFY(QDir(workspace.path()).mkdir("SONGS"));
        const QString sdPath = workspace.filePath("SONGS/sd.wav");
        const QString egoPath = workspace.filePath("SONGS/ego.wav");
        QVERIFY(writeWave(sdPath));
        QVERIFY(writeWave(egoPath));

        SdContentModel sdModel;
        sdModel.setWorkspace(QDir(workspace.path()));
        QTRY_VERIFY_WITH_TIMEOUT(proxyIndex(sdModel, sdPath).isValid(), 5000);
        const QModelIndex sdIndex = proxyIndex(sdModel, sdPath);

        PlaylistModel playlist({makeSong("/SONGS/ego.wav")});
        playlist.setWorkspacePath(workspace.path());

        auto output = std::make_unique<FakeAudioOutput>();
        auto *fake = output.get();
        SoundCore core(std::move(output));

        core.PlayPause(SoundPlayer::PlaySd, sdIndex);
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() > 0, 5000);
        QCOMPARE(core.wavPlaying(), QStringLiteral("/SONGS/sd.wav"));

        core.PlayPause(SoundPlayer::PlayEgo, playlist.index(0, 0));
        QTRY_VERIFY_WITH_TIMEOUT(fake->startCount() >= 2, 5000);
        QVERIFY(core.wavPlaying().isEmpty());
        QCOMPARE(core.egoSongPlaying(), QStringLiteral("ego.wav"));
        QVERIFY(core.playbackActive());
    }
};

QTEST_MAIN(SoundCoreTests)
#include "soundcore_tests.moc"
