#include "playlistrepository.h"

#include "playliststorage.h"

#include <QDir>
#include <QFileInfo>

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
        const QDir playlistDirectory(playlistRoot.absoluteFilePath(directoryName));

        PlaylistData playlist;
        playlist.name = directoryName;
        playlist.directoryPath = playlistDirectory.absolutePath();
        playlist.songs = PlaylistStorage::loadSongs(playlist.directoryPath);
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

    if (QFileInfo::exists(workspace.absoluteFilePath(relativePath)))
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
