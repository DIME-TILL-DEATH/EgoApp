#include <QAbstractItemModelTester>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "playlistmodel.h"
#include "playlistrepository.h"
#include "playlistservice.h"
#include "playliststorage.h"

#include <memory>

class PlaylistTests : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_temporaryDirectory;
    QString m_workspacePath;
    QString m_playlistDirectory;

    bool writeFile(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        return file.write(contents) == contents.size();
    }

    Song song(quint16 number, const QString &track1, const QString &track2 = {})
    {
        Song result;
        result.fileNum = number;
        result.t1Path = track1;
        result.t2Path = track2;
        return result;
    }

    bool writeSongs(const QList<Song> &songs)
    {
        if (songs.isEmpty())
            return true;
        QString error;
        return PlaylistStorage::saveChanges(m_playlistDirectory, {}, songs,
                                             0, songs.size() - 1, &error);
    }

    QDateTime setOldModificationTime(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite))
            return {};
        const QDateTime oldTime(QDate(2001, 1, 1), QTime(0, 0), Qt::UTC);
        if (!file.setFileTime(oldTime, QFileDevice::FileModificationTime))
            return {};
        file.close();

        QFileInfo info(path);
        info.refresh();
        return info.lastModified();
    }

private slots:
    void init()
    {
        m_temporaryDirectory = std::make_unique<QTemporaryDir>();
        QVERIFY(m_temporaryDirectory->isValid());

        m_workspacePath = m_temporaryDirectory->path();
        m_playlistDirectory = QDir(m_workspacePath).filePath("PLAYLIST/Test");
        QVERIFY(QDir().mkpath(m_playlistDirectory));
    }

    void storageLoadsNumericOrderAndExistingFormat()
    {
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("10.ego"),
                          "> /SONGS/Ten.wav\r\n/SONGS/Ten-B.wav\r\n"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("2.ego"),
                          "/SONGS/Two.wav\n"));

        const PlaylistLoadResult result = PlaylistStorage::loadSongs(m_playlistDirectory);
        QVERIFY2(result.errors.isEmpty(), qPrintable(result.errors.join("\n")));
        QCOMPARE(result.songs.size(), 2);
        QCOMPARE(result.songs.at(0).fileNum, quint16(2));
        QCOMPARE(result.songs.at(0).t1Path, QString("/SONGS/Two.wav"));
        QVERIFY(result.songs.at(0).t2Path.isEmpty());
        QCOMPARE(result.songs.at(1).fileNum, quint16(10));
        QVERIFY(result.songs.at(1).playNext);
        QCOMPARE(result.songs.at(1).t2Path, QString("/SONGS/Ten-B.wav"));
    }

    void storageRejectsMalformedNamesAndRecords()
    {
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("0.ego"), "/SONGS/Good.wav\n"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("bad.ego"), "/SONGS/Bad.wav\n"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("65536.ego"), "/SONGS/TooMany.wav\n"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("1.ego"), "/SONGS/NoNewline.wav"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("2.ego"),
                          "/SONGS/Trailing.wav\n/SONGS/Second.wav\nextra"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("3.ego"),
                          QByteArray("/SONGS/") + char(0xff) + QByteArray(".wav\n")));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("4.ego"), "/SONGS/Four.wav\n"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("04.ego"), "/SONGS/FourAlias.wav\n"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("5.ego"), QByteArray()));

        const PlaylistLoadResult result = PlaylistStorage::loadSongs(m_playlistDirectory);
        QCOMPARE(result.songs.size(), 1);
        QCOMPARE(result.songs.first().fileNum, quint16(0));
        QVERIFY(result.errors.size() >= 8);
    }

    void storageAcceptsAnEmptyPlaylistDirectory()
    {
        const PlaylistLoadResult result = PlaylistStorage::loadSongs(m_playlistDirectory);
        QVERIFY(result.songs.isEmpty());
        QVERIFY(result.errors.isEmpty());
    }

    void modelInsertAndRemoveRenumberTheAffectedSuffix()
    {
        const QList<Song> initial{
            song(0, "/SONGS/A.wav"),
            song(1, "/SONGS/B.wav"),
            song(2, "/SONGS/C.wav")
        };
        QVERIFY(writeSongs(initial));
        const QString firstFile = QDir(m_playlistDirectory).filePath("0.ego");
        const QDateTime untouchedPrefixTime = setOldModificationTime(firstFile);
        QVERIFY(!untouchedPrefixTime.isNull());

        PlaylistModel model(initial);
        model.setDir(QDir(m_playlistDirectory));
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        QVERIFY(model.insertRows(1, 1));
        QCOMPARE(QFileInfo(firstFile).lastModified(), untouchedPrefixTime);
        QCOMPARE(model.rowCount(QModelIndex()), 4);
        QCOMPARE(model.data(model.index(0, 0), PlaylistModel::FileNumRole).toUInt(), 0u);
        QCOMPARE(model.data(model.index(1, 0), PlaylistModel::FileNumRole).toUInt(), 1u);
        QCOMPARE(model.data(model.index(2, 0), PlaylistModel::FileNumRole).toUInt(), 2u);
        QCOMPARE(model.data(model.index(3, 0), PlaylistModel::FileNumRole).toUInt(), 3u);
        QVERIFY(QFileInfo::exists(QDir(m_playlistDirectory).filePath("3.ego")));

        QVERIFY(model.removeRows(1, 1));
        QCOMPARE(QFileInfo(firstFile).lastModified(), untouchedPrefixTime);
        QCOMPARE(model.rowCount(QModelIndex()), 3);
        for (int row = 0; row < model.rowCount(QModelIndex()); ++row)
            QCOMPARE(model.data(model.index(row, 0), PlaylistModel::FileNumRole).toUInt(),
                     static_cast<uint>(row));
        QVERIFY(!QFileInfo::exists(QDir(m_playlistDirectory).filePath("3.ego")));
    }

    void modelInsertUsesAnAvailableNumberGap()
    {
        const QList<Song> initial{song(0, "A"), song(2, "C")};
        QVERIFY(writeSongs(initial));
        const QDateTime untouchedFirstTime =
            setOldModificationTime(QDir(m_playlistDirectory).filePath("0.ego"));
        const QDateTime untouchedLastTime =
            setOldModificationTime(QDir(m_playlistDirectory).filePath("2.ego"));
        QVERIFY(!untouchedFirstTime.isNull());
        QVERIFY(!untouchedLastTime.isNull());

        PlaylistModel model(initial);
        model.setDir(QDir(m_playlistDirectory));
        QVERIFY(model.insertRows(1, 1));
        QCOMPARE(QFileInfo(QDir(m_playlistDirectory).filePath("0.ego")).lastModified(),
                 untouchedFirstTime);
        QCOMPARE(QFileInfo(QDir(m_playlistDirectory).filePath("2.ego")).lastModified(),
                 untouchedLastTime);

        QCOMPARE(model.rowCount(QModelIndex()), 3);
        QCOMPARE(model.data(model.index(0, 0), PlaylistModel::FileNumRole).toUInt(), 0u);
        QCOMPARE(model.data(model.index(1, 0), PlaylistModel::FileNumRole).toUInt(), 1u);
        QCOMPARE(model.data(model.index(2, 0), PlaylistModel::FileNumRole).toUInt(), 2u);
        QCOMPARE(PlaylistStorage::loadSongs(m_playlistDirectory).songs.at(2).t1Path,
                 QString("C"));
    }

    void modelRejectsInvalidRowBoundariesAndParents()
    {
        const QList<Song> initial{song(0, "A")};
        QVERIFY(writeSongs(initial));
        PlaylistModel model(initial);
        model.setDir(QDir(m_playlistDirectory));

        QCOMPARE(model.rowCount(model.index(0, 0)), 0);
        QVERIFY(!model.insertRows(-1, 1));
        QVERIFY(!model.insertRows(2, 1));
        QVERIFY(!model.insertRows(0, 0));
        QVERIFY(!model.insertRows(0, 1, model.index(0, 0)));
        QVERIFY(!model.removeRows(-1, 1));
        QVERIFY(!model.removeRows(1, 1));
        QVERIFY(!model.removeRows(0, 2));
        QVERIFY(!model.removeRows(0, 1, model.index(0, 0)));
        QCOMPARE(model.rowCount(QModelIndex()), 1);
    }

    void modelMoveSongMovesOneRowAndPreservesNumbering()
    {
        const QList<Song> initial{
            song(0, "A"), song(1, "B"), song(2, "C"), song(3, "D")
        };
        QVERIFY(writeSongs(initial));

        PlaylistModel model(initial);
        model.setDir(QDir(m_playlistDirectory));
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        QVERIFY(model.moveSong(0, 3));
        QCOMPARE(model.data(model.index(3, 0), PlaylistModel::T1PathRole).toString(), QString("A"));
        QVERIFY(model.moveSong(3, 1));
        QCOMPARE(model.data(model.index(0, 0), PlaylistModel::T1PathRole).toString(), QString("B"));
        QCOMPARE(model.data(model.index(1, 0), PlaylistModel::T1PathRole).toString(), QString("A"));
        QCOMPARE(model.data(model.index(2, 0), PlaylistModel::T1PathRole).toString(), QString("C"));
        QCOMPARE(model.data(model.index(3, 0), PlaylistModel::T1PathRole).toString(), QString("D"));

        QVERIFY(model.moveSong(2, 2));
        QVERIFY(!model.moveSong(-1, 0));
        QVERIFY(!model.moveSong(0, 4));
        for (int row = 0; row < model.rowCount(QModelIndex()); ++row)
            QCOMPARE(model.data(model.index(row, 0), PlaylistModel::FileNumRole).toUInt(),
                     static_cast<uint>(row));
    }

    void modelMovePersistsOnlyTheMovedInterval()
    {
        const QList<Song> initial{
            song(0, "A"), song(1, "B"), song(2, "C"), song(3, "D")
        };
        QVERIFY(writeSongs(initial));
        const QDateTime untouchedFirstTime =
            setOldModificationTime(QDir(m_playlistDirectory).filePath("0.ego"));
        const QDateTime untouchedLastTime =
            setOldModificationTime(QDir(m_playlistDirectory).filePath("3.ego"));
        QVERIFY(!untouchedFirstTime.isNull());
        QVERIFY(!untouchedLastTime.isNull());

        PlaylistModel model(initial);
        model.setDir(QDir(m_playlistDirectory));
        QVERIFY(model.moveSong(1, 2));

        QCOMPARE(model.data(model.index(0, 0), PlaylistModel::T1PathRole).toString(), QString("A"));
        QCOMPARE(model.data(model.index(1, 0), PlaylistModel::T1PathRole).toString(), QString("C"));
        QCOMPARE(model.data(model.index(2, 0), PlaylistModel::T1PathRole).toString(), QString("B"));
        QCOMPARE(model.data(model.index(3, 0), PlaylistModel::T1PathRole).toString(), QString("D"));
        QCOMPARE(QFileInfo(QDir(m_playlistDirectory).filePath("0.ego")).lastModified(),
                 untouchedFirstTime);
        QCOMPARE(QFileInfo(QDir(m_playlistDirectory).filePath("3.ego")).lastModified(),
                 untouchedLastTime);
    }

    void modelSingleSongEditsPersistOnlyTheirRecordAndNotifyRoles()
    {
        const QList<Song> initial{song(0, "/SONGS/A.wav"), song(1, "/SONGS/B.wav")};
        QVERIFY(writeSongs(initial));

        const QString file0 = QDir(m_playlistDirectory).filePath("0.ego");
        const QString file1 = QDir(m_playlistDirectory).filePath("1.ego");
        const QDateTime untouchedFile1Time = setOldModificationTime(file1);
        QVERIFY(!untouchedFile1Time.isNull());

        PlaylistModel model(initial);
        model.setDir(QDir(m_playlistDirectory));
        QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);
        QVERIFY(changedSpy.isValid());

        QVERIFY(model.setLink(model.index(0, 0), 0, "/SONGS/A1.wav"));
        QCOMPARE(model.data(model.index(0, 0), PlaylistModel::T1PathRole).toString(),
                 QString("/SONGS/A1.wav"));
        QCOMPARE(changedSpy.size(), 1);
        const QList<int> track1Roles = qvariant_cast<QList<int>>(changedSpy.takeFirst().at(2));
        QVERIFY(track1Roles.contains(PlaylistModel::T1PathRole));
        QVERIFY(track1Roles.contains(PlaylistModel::T1NameRole));
        QVERIFY(track1Roles.contains(PlaylistModel::MidiExistRole));

        QVERIFY(model.setLink(model.index(0, 0), 1, "/SONGS/A2.wav"));
        QCOMPARE(model.data(model.index(0, 0), PlaylistModel::T2PathRole).toString(),
                 QString("/SONGS/A2.wav"));
        QCOMPARE(changedSpy.size(), 1);
        const QList<int> track2Roles = qvariant_cast<QList<int>>(changedSpy.takeFirst().at(2));
        QVERIFY(track2Roles.contains(PlaylistModel::T2PathRole));
        QVERIFY(track2Roles.contains(PlaylistModel::T2NameRole));
        QCOMPARE(QFileInfo(file1).lastModified(), untouchedFile1Time);

        const QDateTime untouchedFile0Time = setOldModificationTime(file0);
        QVERIFY(!untouchedFile0Time.isNull());
        QVERIFY(model.setPlayNext(model.index(1, 0), true));
        QVERIFY(model.data(model.index(1, 0), PlaylistModel::PlayNextRole).toBool());
        QCOMPARE(changedSpy.size(), 1);
        const QList<int> nextRoles = qvariant_cast<QList<int>>(changedSpy.takeFirst().at(2));
        QCOMPARE(nextRoles, QList<int>{PlaylistModel::PlayNextRole});
        QCOMPARE(QFileInfo(file0).lastModified(), untouchedFile0Time);

        const PlaylistLoadResult reloaded = PlaylistStorage::loadSongs(m_playlistDirectory);
        QVERIFY(reloaded.errors.isEmpty());
        QCOMPARE(reloaded.songs.at(0).t1Path, QString("/SONGS/A1.wav"));
        QCOMPARE(reloaded.songs.at(0).t2Path, QString("/SONGS/A2.wav"));
        QVERIFY(reloaded.songs.at(1).playNext);
    }

    void modelRejectsInvalidIndexesAndLeavesMemoryUntouchedOnSaveFailure()
    {
        const QList<Song> initial{song(0, "/SONGS/A.wav")};
        QVERIFY(writeSongs(initial));
        PlaylistModel model(initial);
        model.setDir(QDir(m_temporaryDirectory->filePath("missing-directory")));
        PlaylistModel other(initial);

        QVERIFY(!model.setLink(other.index(0, 0), 0, "/SONGS/Changed.wav"));
        QVERIFY(!model.setPlayNext(other.index(0, 0), true));
        QVERIFY(!model.data(other.index(0, 0), PlaylistModel::T1PathRole).isValid());
        QVERIFY(!model.data(model.index(0, 0), Qt::UserRole + 100).isValid());

        QVERIFY(!model.setLink(model.index(0, 0), 0, "/SONGS/Changed.wav"));
        QCOMPARE(model.data(model.index(0, 0), PlaylistModel::T1PathRole).toString(),
                 QString("/SONGS/A.wav"));
    }

    void repositoryCreatesDiscoversAndRemovesNestedPlaylists()
    {
        PlaylistRepository repository;
        QVERIFY(repository.create(m_workspacePath, "Collections/Rock"));
        QVERIFY(repository.create(m_workspacePath, "Collections/Jazz"));

        const QString nestedDir = QDir(m_workspacePath).filePath("PLAYLIST/Collections/Rock");
        QVERIFY(writeFile(QDir(nestedDir).filePath("0.ego"), "/SONGS/Rock.wav\n"));

        QString error;
        const QList<PlaylistData> playlists = repository.load(m_workspacePath, &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(playlists.size(), 3);
        PlaylistData loaded;
        QVERIFY(repository.loadPlaylist(m_workspacePath, "Collections/Rock", &loaded));
        QCOMPARE(loaded.songs.size(), 1);
        QVERIFY(!repository.create(m_workspacePath, "../Outside"));
        QVERIFY(!repository.remove(m_workspacePath, "../Outside"));

        QVERIFY(repository.remove(m_workspacePath, "Collections"));
        QVERIFY(!QFileInfo::exists(QDir(m_workspacePath).filePath("PLAYLIST/Collections")));
    }

    void modelTreatsTrackTwoOnlyAsAValidLinkedSong()
    {
        QVERIFY(QDir().mkpath(QDir(m_workspacePath).filePath("SONGS")));
        QVERIFY(writeFile(QDir(m_workspacePath).filePath("SONGS/OnlySecond.wav"), "audio"));

        PlaylistModel model({song(0, {}, "/SONGS/OnlySecond.wav")});
        model.setWorkspacePath(m_workspacePath);

        QVERIFY(!model.data(model.index(0, 0), PlaylistModel::T1MissingRole).toBool());
        QVERIFY(!model.data(model.index(0, 0), PlaylistModel::T2MissingRole).toBool());
        QVERIFY(!model.data(model.index(0, 0), PlaylistModel::FileMissingRole).toBool());

        QString resolved;
        QVERIFY(model.resolveMediaPath("/SONGS/OnlySecond.wav", &resolved));
        QCOMPARE(QFileInfo(resolved).absoluteFilePath(),
                 QFileInfo(QDir(m_workspacePath).filePath("SONGS/OnlySecond.wav")).absoluteFilePath());
        QVERIFY(!model.resolveMediaPath("../outside.wav", &resolved));
    }

    void serviceAcceptsTrackTwoOnlyAndRejectsSongsWithoutTracks()
    {
        QVERIFY(QDir().mkpath(QDir(m_workspacePath).filePath("SONGS")));
        QVERIFY(writeFile(QDir(m_workspacePath).filePath("SONGS/Second.wav"), "dummy"));
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("0.ego"),
                          "\n/SONGS/Second.wav"));

        PlaylistService service;
        service.setWorkspacePath(m_workspacePath);
        QCOMPARE(service.playlistPaths(), QStringList{"Test"});

        const QStringList validResult = service.checkPlaylist("Test");
        QVERIFY(validResult.join("\n").contains("Playlist is OK"));

        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("0.ego"), "\n"));
        service.reload();
        const QStringList invalidResult = service.checkPlaylist("Test");
        QVERIFY(invalidResult.join("\n").contains("has no linked tracks"));
    }

    void serviceMarksStructurallyDamagedPlaylistsReadOnly()
    {
        QVERIFY(writeFile(QDir(m_playlistDirectory).filePath("0.ego"), QByteArray()));
        PlaylistService service;
        service.setWorkspacePath(m_workspacePath);

        PlaylistModel *model = service.playlistAt(0);
        QVERIFY(model);
        QVERIFY(model->isCorrupted());
        QVERIFY(!model->insertRows(0, 1));
        QCOMPARE(model->rowCount(QModelIndex()), 0);
    }
};

QTEST_GUILESS_MAIN(PlaylistTests)
#include "playlist_tests.moc"
