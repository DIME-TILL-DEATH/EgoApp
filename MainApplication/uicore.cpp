#include "uicore.h"

UiCore::UiCore(QObject *parent)
    : QObject{parent}
{
    QString workspacePath = m_settings.value("workspace_path").toString();
    m_workspaceDir = QDir(workspacePath);
    m_sdContentModel.setWorkspace(workspacePath);
}

void UiCore::setWorkspace(QUrl workspaceFolderPath)
{
    QString path = workspaceFolderPath.path();
#ifdef Q_OS_WINDOWS
    path = path.removeFirst();
#endif
    m_workspaceDir = QDir(path);
    m_sdContentModel.setWorkspace(path);

    m_settings.setValue("workspace_path", path);

    emit workspaceChanged();
}

QString UiCore::workspacePath() const
{
    return m_workspaceDir.absolutePath();
}

QStringList UiCore::avaliablePlaylists() const
{
    QStringList playlists;

    QDir playlistDir = m_workspaceDir;
    playlistDir.cd("PLAYLIST");

    QStringList allDirs = playlistDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

    foreach(QString plsDirPath, allDirs)
    {
        QDir plsDir(playlistDir.absolutePath() + "/" + plsDirPath);

        QStringList filters;
        filters.append("*.ego");
        QStringList egoFiles = plsDir.entryList(filters);

        if(!egoFiles.isEmpty()) playlists.append(plsDirPath);
    }

    return playlists;
}
