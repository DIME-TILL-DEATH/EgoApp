#include "uicore.h"

UiCore::UiCore(QObject *parent)
    : QObject{parent}
{
    QString workspacePath = m_settings.value("workspace_path").toString();
    m_workspaceDir = QDir(workspacePath);
    m_sdContentModel.setWorkspace(workspacePath);

    connect(&m_sdContentModel, &SdContentModel::errorOccured, this, &UiCore::errorOccured);

    readPlaylistFolder();
    setCurrentPlaylistIndex(0);
}

void UiCore::setWorkspace(QUrl workspaceFolderPath)
{
    qDebug() << __FUNCTION__;
    QString path = workspaceFolderPath.path();
#ifdef Q_OS_WINDOWS
    path = path.removeFirst();
#endif
    m_workspaceDir = QDir(path);
    m_sdContentModel.setWorkspace(path);

    m_settings.setValue("workspace_path", path);

    readPlaylistFolder();
    setCurrentPlaylistIndex(0);

    emit workspaceChanged();
}

void UiCore::addPlaylist(QString plsName)
{
    m_workspaceDir.mkpath("PLAYLIST/" + plsName);
    readPlaylistFolder();

    for(int i=0; i < m_avaliablePlaylists.size(); i++)
    {
        qDebug() << i << m_avaliablePlaylists.at(i)->playlistName() << plsName;
        if(m_avaliablePlaylists.at(i)->playlistName() == plsName)
        {
            emit workspaceChanged();
            setCurrentPlaylistIndex(i);
        }
    }
}

void UiCore::deletePlaylist(quint16 index)
{
    QDir playlistDir = m_workspaceDir;
    if(playlistDir.cd("PLAYLIST"))
    {
        if(index < m_avaliablePlaylists.size())
        {
            QDir plsDir(playlistDir.absolutePath() + "/" + m_avaliablePlaylists.at(index)->playlistName());
            plsDir.removeRecursively();
            readPlaylistFolder();
            emit workspaceChanged();

            setCurrentPlaylistIndex(0);
        }
    }
}

QString UiCore::workspacePath() const
{
    return m_workspaceDir.absolutePath();
}

QStringList UiCore::avaliablePlaylists() const
{
    QStringList playlists;

    foreach (PlaylistModel* plsModel, m_avaliablePlaylists)
    {
        playlists.append(plsModel->playlistName());
    }

    return playlists;
}

PlaylistModel* UiCore::currentPlaylist() const
{
    return m_currentPlaylist;
}

void UiCore::readPlaylistFolder()
{
    qDeleteAll(m_avaliablePlaylists);
    m_avaliablePlaylists.clear();
    m_currentPlaylist = nullptr;

    QDir playlistDir = m_workspaceDir;
    if(playlistDir.cd("PLAYLIST"))
    {
        QStringList allDirs = playlistDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

        foreach(QString plsDirPath, allDirs)
        {
            QDir plsDir(playlistDir.absolutePath() + "/" + plsDirPath);

            QStringList filters;
            filters.append("*.ego");
            QStringList egoFiles = plsDir.entryList(filters);

            QList<Song> songList;
            if(!egoFiles.isEmpty())
            {
                foreach (QString egoFilePath, egoFiles)
                {
                    Song song;

                    QFile file(plsDir.absoluteFilePath(egoFilePath));
                    if(file.open(QIODevice::ReadOnly))
                    {
                        QByteArray track1Line = file.readLine();
                        if(track1Line.size() != 0)
                        {
                            if(track1Line.front() == '>')
                            {
                                song.playNext = true;
                                track1Line = track1Line.removeFirst();
                            }
                            song.t1Path = track1Line;
                            song.t1Path.remove("\n");
                        }
                        song.t2Path = file.readLine();
                        song.t2Path.remove("\n");
                    }
                    songList.append(song);
                }

            }
            PlaylistModel* playListModel = new PlaylistModel(songList, this);
            playListModel->setPlaylistName(plsDirPath);
            m_avaliablePlaylists.append(playListModel);
        }
    }
}


quint16 UiCore::currentPlaylistIndex() const
{
    return m_currentPlaylistIndex;
}

void UiCore::setCurrentPlaylistIndex(qint16 newCurrentPlaylistIndex)
{
    qDebug() << __FUNCTION__ << newCurrentPlaylistIndex;

    m_currentPlaylistIndex = newCurrentPlaylistIndex;

    if(m_avaliablePlaylists.size() == 0)
    {
        m_currentPlaylist = nullptr;
        m_currentPlaylistIndex = -1;
    }
    else
    {
        if(m_currentPlaylistIndex <  m_avaliablePlaylists.size())
            m_currentPlaylist = m_avaliablePlaylists.at(m_currentPlaylistIndex);
    }

    emit currentPlaylistChanged();
    emit currentPlaylistIndexChanged();
}
