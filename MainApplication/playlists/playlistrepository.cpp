#include "playlistrepository.h"

#include <QCollator>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <algorithm>

bool PlaylistRepository::isValidPlaylistName(const QString &name)
{
    return !name.isEmpty()
           && name != "."
           && name != ".."
           && !name.startsWith('.')
           && !name.contains('/')
           && !name.contains('\\');
}

QList<PlaylistData> PlaylistRepository::load(const QString &workspacePath) const
{
    QList<PlaylistData> result;

    QDir playlistRoot(workspacePath);
    if (!playlistRoot.cd("PLAYLIST"))
        return result;

    const QStringList directories =
        playlistRoot.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

    for (const QString &directoryName : directories)
    {
        QDir playlistDirectory(playlistRoot.absoluteFilePath(directoryName));

        QStringList egoFiles =
            playlistDirectory.entryList({"*.ego"}, QDir::Files, QDir::Name);

        QCollator collator;
        collator.setNumericMode(true);
        std::sort(egoFiles.begin(), egoFiles.end(), collator);

        PlaylistData playlist;
        playlist.name = directoryName;
        playlist.directoryPath = playlistDirectory.absolutePath();

        for (const QString &egoFileName : egoFiles)
        {
            Song song;
            QFile file(playlistDirectory.absoluteFilePath(egoFileName));

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
            playlist.songs.append(song);
        }

        result.append(playlist);
    }

    return result;
}

bool PlaylistRepository::create(const QString &workspacePath, const QString &name) const
{
    if (!isValidPlaylistName(name))
        return false;

    QDir workspace(workspacePath);
    const QString relativePath = QStringLiteral("PLAYLIST/") + name;

    if (QFileInfo(workspace.absoluteFilePath(relativePath)).exists())
        return false;

    return workspace.mkpath(relativePath);
}

bool PlaylistRepository::remove(const QString &workspacePath, const QString &name) const
{
    if (!isValidPlaylistName(name))
        return false;

    const QString playlistRoot =
        QFileInfo(QDir(workspacePath).absoluteFilePath("PLAYLIST")).canonicalFilePath();
    if (playlistRoot.isEmpty())
        return false;

    const QString target =
        QFileInfo(QDir(playlistRoot).absoluteFilePath(name)).canonicalFilePath();
    if (target.isEmpty())
        return false;

    const QString normalizedRoot = QDir::fromNativeSeparators(QDir::cleanPath(playlistRoot));
    const QString normalizedTarget = QDir::fromNativeSeparators(QDir::cleanPath(target));
    if (!normalizedTarget.startsWith(normalizedRoot + '/'))
        return false;

    return QDir(target).removeRecursively();
}
