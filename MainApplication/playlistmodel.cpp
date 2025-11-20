#include "playlistmodel.h"

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractListModel{parent}
{

}

PlaylistModel::PlaylistModel(const QList<Song> &songList, QObject *parent)
    : QAbstractListModel{parent}
{
    m_songList = songList;
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_songList.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() > rowCount(index))
    {
        return {};
    }

    switch(role)
    {
        case ListRoles::PlayNextRole: return m_songList.at(index.row()).playNext;
        case ListRoles::T1PathRole: return m_songList.at(index.row()).t1Path;
        case ListRoles::T2PathRole: return m_songList.at(index.row()).t2Path;
        case ListRoles::T1NameRole:
        {
            QString path =  m_songList.at(index.row()).t1Path;
            quint64 fileNamePos = path.lastIndexOf("/");
            return path.mid(fileNamePos+1);
        }

        case ListRoles::T2NameRole:
        {
            QString path =  m_songList.at(index.row()).t2Path;
            quint64 fileNamePos = path.lastIndexOf("/");
            return path.mid(fileNamePos+1);
        }

        default:
        {
            qWarning() << __FUNCTION__ << "no role";
            return true;
        }
    }

}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ListRoles::PlayNextRole] = "playNext";
    roles[ListRoles::T1PathRole] = "t1Path";
    roles[ListRoles::T2PathRole] = "t2Path";
    roles[ListRoles::T1NameRole] = "t1Name";
    roles[ListRoles::T2NameRole] = "t2Name";

    return roles;
}

void PlaylistModel::refreshModel(const QList<Song> &songList)
{

}

void PlaylistModel::setLink(const QModelIndex &index, quint8 trackNum, QString path)
{
    if(!index.isValid()) return;
    quint16 songNum = index.row();

    if(songNum > m_songList.size()) return;

    Song* song = m_songList.data();
    song[songNum].setPath(trackNum, path);

    emit dataChanged(index, index);
}

QDir PlaylistModel::dir() const
{
    return m_dir;
}

void PlaylistModel::setDir(const QDir &newDir)
{
    m_dir = newDir;
}
