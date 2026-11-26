#ifndef MEDIAPATH_H
#define MEDIAPATH_H

#include <QDir>
#include <QString>
#include <QStringList>

// Media paths in .ego are rooted at the selected workspace, which represents
// the target SD-card root. The optional leading '/' is a workspace-root marker.
namespace MediaPath
{
inline bool resolveWorkspacePath(const QString &workspacePath,
                                 const QString &storedPath,
                                 QString *resolvedPath)
{
    if (!resolvedPath || workspacePath.isEmpty() || storedPath.isEmpty())
        return false;

    QString path = storedPath;
    if (path.startsWith('/'))
        path.remove(0, 1);

    // Reject malformed separators and host-specific absolute/drive paths.
    if (path.isEmpty() || path.startsWith('/') || path.startsWith('\\')
        || path.contains('\\') || path.contains(':') || path.endsWith('/'))
        return false;

    for (const QChar character : path)
    {
        if (character.isNull() || character.isControl())
            return false;
    }

    const QStringList components = path.split('/', Qt::KeepEmptyParts);
    for (const QString &component : components)
    {
        if (component.isEmpty() || component == QStringLiteral(".")
            || component == QStringLiteral(".."))
            return false;
    }

    *resolvedPath = QDir(workspacePath).filePath(path);
    return true;
}

inline QString workspaceFilePath(const QString &workspacePath, const QString &storedPath)
{
    QString resolvedPath;
    return resolveWorkspacePath(workspacePath, storedPath, &resolvedPath)
               ? resolvedPath
               : QString();
}
}

#endif // MEDIAPATH_H
