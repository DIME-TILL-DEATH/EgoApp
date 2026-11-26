#ifndef MEDIAPATH_H
#define MEDIAPATH_H

#include <QDir>
#include <QFileInfo>
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
        if (character.isNull() || character.category() == QChar::Other_Control)
            return false;
    }

    const QStringList components = path.split('/', Qt::KeepEmptyParts);
    for (const QString &component : components)
    {
        if (component.isEmpty() || component == QStringLiteral(".")
            || component == QStringLiteral(".."))
            return false;
    }

    QString candidatePath = QDir(workspacePath).absolutePath();
    for (const QString &component : components)
    {
        candidatePath = QDir(candidatePath).filePath(component);
        const QFileInfo componentInfo(candidatePath);
        if (componentInfo.isSymLink())
            return false; // Symbolic links on the target device are not supported.
    }

    *resolvedPath = candidatePath;
    return true;
}

}

#endif // MEDIAPATH_H
