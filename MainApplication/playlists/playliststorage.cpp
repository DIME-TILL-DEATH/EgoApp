#include "playliststorage.h"

#include <QObject>

#include <QCollator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>

#include <algorithm>

QList<Song> PlaylistStorage::loadSongs(const QString &directoryPath)
{
    QList<Song> songs;
    const QDir directory(directoryPath);

    QStringList egoFiles =
        directory.entryList({"*.ego"}, QDir::Files, QDir::Name);

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(egoFiles.begin(), egoFiles.end(), collator);

    for (const QString &egoFileName : egoFiles)
    {
        Song song;
        QFile file(directory.absoluteFilePath(egoFileName));

        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray track1Line = file.readLine();
            if (!track1Line.isEmpty())
            {
                if (track1Line.front() == '>')
                {
                    song.playNext = true;
                    track1Line.remove(0, 1);
                }

                song.t1Path = track1Line;
                song.t1Path.remove('\n');
            }

            song.t2Path = file.readLine();
            song.t2Path.remove('\n');
        }

        QString fileNumber = QFileInfo(file).fileName();
        fileNumber.remove(".ego");
        song.fileNum = fileNumber.toInt();
        songs.append(song);
    }

    return songs;
}

bool PlaylistStorage::saveSongs(const QString &directoryPath, const QList<Song> &songs,
                                QString *errorString)
{
    const QDir directory(directoryPath);
    if (!directory.exists())
    {
        if (errorString)
            *errorString = QObject::tr("Playlist directory does not exist: ") + directoryPath;
        return false;
    }

    QSet<QString> expectedFiles;
    for (const Song &song : songs)
    {
        const QString fileName = QString::number(song.fileNum) + ".ego";
        expectedFiles.insert(fileName);

        const QString filePath = directory.absoluteFilePath(fileName);
        QSaveFile egoFile(filePath);
        if (!egoFile.open(QIODevice::WriteOnly))
        {
            if (errorString)
                *errorString = QObject::tr("Can't open playlist file for writing: ") + filePath;
            return false;
        }

        const QByteArray content = song.generateFileContent();
        if (egoFile.write(content) != content.size())
        {
            egoFile.cancelWriting();
            if (errorString)
                *errorString = QObject::tr("Can't write playlist file: ") + filePath;
            return false;
        }

        if (!egoFile.commit())
        {
            if (errorString)
                *errorString = QObject::tr("Can't finalize playlist file: ") + filePath;
            return false;
        }
    }

    const QStringList existingFiles =
        directory.entryList({"*.ego"}, QDir::Files, QDir::Name);
    for (const QString &fileName : existingFiles)
    {
        if (expectedFiles.contains(fileName))
            continue;

        const QString filePath = directory.absoluteFilePath(fileName);
        if (!QFile::remove(filePath))
        {
            if (errorString)
                *errorString = QObject::tr("Can't remove stale playlist file: ") + filePath;
            return false;
        }
    }

    return true;
}
