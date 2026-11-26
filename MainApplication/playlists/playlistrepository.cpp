#include "playlistrepository.h"

#include "playliststorage.h"

#include <QDir>
#include <QFileInfo>

namespace
{
bool isPathInside(const QString &rootPath, const QString &targetPath)
{
    const QString root = QDir::fromNativeSeparators(QDir::cleanPath(rootPath));
    const QString target = QDir::fromNativeSeparators(QDir::cleanPath(targetPath));
    const QString rootPrefix = root.endsWith('/') ? root : root + '/';

#ifdef Q_OS_WIN
    return target.compare(root, Qt::CaseInsensitive) != 0
           && target.startsWith(rootPrefix, Qt::CaseInsensitive);
#else
    return target != root && target.startsWith(rootPrefix);
#endif
}

QString canonicalWorkspacePath(const QString &workspacePath)
{
    return QFileInfo(QDir(workspacePath).absolutePath()).canonicalFilePath();
}

QString canonicalPlaylistRoot(const QString &workspacePath)
{
    const QString workspace = canonicalWorkspacePath(workspacePath);
    if (workspace.isEmpty())
        return {};

    const QString rootCandidate = QDir(workspace).filePath(QStringLiteral("PLAYLIST"));
    const QFileInfo rootInfo(rootCandidate);
    if (!rootInfo.exists() || !rootInfo.isDir() || rootInfo.isSymLink())
        return {};

    const QString root = rootInfo.canonicalFilePath();
    if (root.isEmpty() || !isPathInside(workspace, root))
        return {};

    return root;
}

void loadDirectories(const QString &rootPath,
                     const QString &directoryPath,
                     const QString &relativeDirectoryPath,
                     QList<PlaylistData> &result)
{
    const QDir directory(directoryPath);
    const QFileInfoList children =
        directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                                QDir::Name);

    for (const QFileInfo &child : children)
    {
        if (child.isSymLink() || !child.isDir())
            continue;

        const QString canonicalPath = child.canonicalFilePath();
        if (canonicalPath.isEmpty() || !isPathInside(rootPath, canonicalPath))
            continue;

        const QString relativePath = relativeDirectoryPath.isEmpty()
                                         ? child.fileName()
                                         : relativeDirectoryPath + '/' + child.fileName();

        PlaylistData playlist;
        playlist.name = child.fileName();
        playlist.relativePath = relativePath;
        playlist.directoryPath = canonicalPath;
        playlist.songs = PlaylistStorage::loadSongs(canonicalPath);
        result.append(playlist);

        loadDirectories(rootPath, canonicalPath, relativePath, result);
    }
}
}

bool PlaylistRepository::isValidRelativePath(const QString &relativePath)
{
    if (relativePath.isEmpty()
        || relativePath.startsWith('/')
        || relativePath.startsWith('\\')
        || relativePath.contains('\\')
        || relativePath.contains(':'))
    {
        return false;
    }

    const QStringList components = relativePath.split('/', Qt::KeepEmptyParts);
    for (const QString &component : components)
    {
        if (component.isEmpty() || component == "." || component == ".."
            || component.startsWith('.'))
        {
            return false;
        }

        for (const QChar character : component)
        {
            if (character.isNull() || character.category() == QChar::Other_Control)
                return false;
        }
    }

    return true;
}

QList<PlaylistData> PlaylistRepository::load(const QString &workspacePath) const
{
    QList<PlaylistData> result;
    const QString rootPath = canonicalPlaylistRoot(workspacePath);
    if (rootPath.isEmpty())
        return result;

    loadDirectories(rootPath, rootPath, {}, result);
    return result;
}

bool PlaylistRepository::loadPlaylist(const QString &workspacePath,
                                      const QString &relativePath,
                                      PlaylistData *playlist) const
{
    if (!playlist || !isValidRelativePath(relativePath))
        return false;

    const QString rootPath = canonicalPlaylistRoot(workspacePath);
    if (rootPath.isEmpty())
        return false;

    QString currentPath = rootPath;
    const QStringList components = relativePath.split('/');
    for (const QString &component : components)
    {
        currentPath = QDir(currentPath).filePath(component);
        const QFileInfo componentInfo(currentPath);
        if (componentInfo.isSymLink() || !componentInfo.exists() || !componentInfo.isDir())
            return false;

        const QString canonicalPath = componentInfo.canonicalFilePath();
        if (canonicalPath.isEmpty() || !isPathInside(rootPath, canonicalPath))
            return false;

        currentPath = canonicalPath;
    }

    PlaylistData loadedPlaylist;
    loadedPlaylist.name = components.constLast();
    loadedPlaylist.relativePath = relativePath;
    loadedPlaylist.directoryPath = currentPath;
    loadedPlaylist.songs = PlaylistStorage::loadSongs(currentPath);
    *playlist = loadedPlaylist;
    return true;
}

bool PlaylistRepository::create(const QString &workspacePath, const QString &relativePath) const
{
    if (!isValidRelativePath(relativePath))
        return false;

    const QString workspace = canonicalWorkspacePath(workspacePath);
    if (workspace.isEmpty())
        return false;

    const QString rootCandidate = QDir(workspace).filePath(QStringLiteral("PLAYLIST"));
    QFileInfo rootInfo(rootCandidate);

    if (rootInfo.isSymLink())
        return false;

    if (!rootInfo.exists())
    {
        if (!QDir(workspace).mkdir(QStringLiteral("PLAYLIST")))
            return false;
        rootInfo.setFile(rootCandidate);
    }

    if (!rootInfo.isDir())
        return false;

    const QString rootPath = rootInfo.canonicalFilePath();
    if (rootPath.isEmpty() || !isPathInside(workspace, rootPath))
        return false;

    const QStringList components = relativePath.split('/');
    QString currentPath = rootPath;

    for (qsizetype i = 0; i < components.size(); ++i)
    {
        currentPath = QDir(currentPath).filePath(components.at(i));
        const QFileInfo componentInfo(currentPath);

        if (componentInfo.isSymLink())
            return false;

        const bool isFinalComponent = (i == components.size() - 1);
        if (componentInfo.exists())
        {
            if (!componentInfo.isDir() || isFinalComponent)
                return false;
        }
        else if (!QDir(QFileInfo(currentPath).absolutePath()).mkdir(components.at(i)))
        {
            return false;
        }

        const QString canonicalPath = QFileInfo(currentPath).canonicalFilePath();
        if (canonicalPath.isEmpty() || !isPathInside(rootPath, canonicalPath))
            return false;
        currentPath = canonicalPath;
    }

    return true;
}

bool PlaylistRepository::remove(const QString &workspacePath, const QString &relativePath) const
{
    if (!isValidRelativePath(relativePath))
        return false;

    const QString rootPath = canonicalPlaylistRoot(workspacePath);
    if (rootPath.isEmpty())
        return false;

    QString currentPath = rootPath;
    const QStringList components = relativePath.split('/');
    for (const QString &component : components)
    {
        currentPath = QDir(currentPath).filePath(component);
        const QFileInfo componentInfo(currentPath);
        if (componentInfo.isSymLink() || !componentInfo.exists() || !componentInfo.isDir())
            return false;

        const QString canonicalPath = componentInfo.canonicalFilePath();
        if (canonicalPath.isEmpty() || !isPathInside(rootPath, canonicalPath))
            return false;
        currentPath = canonicalPath;
    }

    if (!isPathInside(rootPath, currentPath))
        return false;

    return QDir(currentPath).removeRecursively();
}
