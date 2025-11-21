#include "playlistmodel.h"

#include <QFile>

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
        case ListRoles::FileNumRole: return m_songList.at(index.row()).fileNum;
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
    roles[ListRoles::FileNumRole] = "fileNum";
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

bool PlaylistModel::insertRows(int position, int count, const QModelIndex &parent)
{
    if(position < 0) position = 0;

    beginInsertRows(parent, position, position + count - 1);
    for(int rowNumber = 0; rowNumber < count; ++rowNumber)
    {
        Song newSong;
        Song* oldSong = m_songList.data();
        // newSong.fileNum = 0;

        if(position < m_songList.size()) //!m_songList.empty() &&
        {
            newSong.fileNum = m_songList.at(position).fileNum + rowNumber;
        }
        else
        {
            newSong.fileNum = m_songList.size();
        }

        if(position < m_songList.size())
        {
            for(int i = position + rowNumber; i < m_songList.size(); ++i)
            {
                if(m_songList.size() == i+1)
                {
                    oldSong[i].fileNum++;
                    break;
                }

                if((oldSong[i+1].fileNum - oldSong[i].fileNum) == 1)
                {
                    oldSong[i].fileNum++;
                }
                else
                {
                    oldSong[i].fileNum++;
                    break;
                }
            }

            m_songList.insert(position, newSong);
        }
        else
        {
            m_songList.append(newSong);
        }
    }

    for(int i=0; i < m_songList.size(); ++i)
    {
        writeEgoFile(m_songList.at(i));
    }

    endInsertRows();
    return true;
}

bool PlaylistModel::removeRows(int position, int count, const QModelIndex &parent)
{
    if(position < 0 || position > m_songList.size() - 1) return false;

    beginRemoveRows(parent, position, position + count - 1);
    foreach(auto song, m_songList)
    {
        QFile file(m_dir.absolutePath() + "/" + QString::number(song.fileNum) + ".ego");
        file.remove();
    }

    for(int rowNumber = 0; rowNumber < count; ++rowNumber)
    {
        m_songList.remove(position, 1);

        Song* oldSong = m_songList.data();
        for(int i = position + rowNumber; i < m_songList.size(); ++i)
        {
            if(m_songList.size() == i+1)
            {
                oldSong[i].fileNum--;
                break;
            }

            if((oldSong[i+1].fileNum - oldSong[i].fileNum) == 1)
            {
                oldSong[i].fileNum--;
            }
            else
            {
                oldSong[i].fileNum--;
                break;
            }
        }
    }

    for(int i=0; i < m_songList.size(); ++i)
    {
        writeEgoFile(m_songList.at(i));
    }

    endRemoveRows();
    return true;
}

void PlaylistModel::moveSong(quint16 fromPosition, quint16 toPosition)
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

    writeEgoFile(song[songNum]);

}

void PlaylistModel::setPlayNext(const QModelIndex &index, bool state)
{
    if(!index.isValid()) return;
    quint16 songNum = index.row();

    if(songNum > m_songList.size()) return;

    Song* song = m_songList.data();
    song[songNum].playNext = state;

    emit dataChanged(index, index);

    writeEgoFile(song[songNum]);
}

QDir PlaylistModel::dir() const
{
    return m_dir;
}

void PlaylistModel::setDir(const QDir &newDir)
{
    m_dir = newDir;
}

void PlaylistModel::writeEgoFile(const Song &song)
{
    QFile egoFile(m_dir.absolutePath() + "/" + QString::number(song.fileNum) + ".ego");
    if(egoFile.open(QIODevice::WriteOnly))
    {
        egoFile.write(song.generateFileContent());
        egoFile.close();
    }
}
