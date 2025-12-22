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

        case ListRoles::MidiExistRole:
        {
            QDir wrkSpaceDir = m_dir;
            wrkSpaceDir.cd("../..");
            QString wrkSpacePath = wrkSpaceDir.absolutePath();

            QString midiPath = wrkSpacePath +  m_songList.at(index.row()).t1Path;
            midiPath.chop(3);
            midiPath += "mid";

            QFileInfo midiFileInfo(midiPath);
            return midiFileInfo.exists();
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
    roles[ListRoles::MidiExistRole] = "midiExist";

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
                oldSong[i].fileNum++;

                if(m_songList.size() == i+1) break;
                if((oldSong[i+1].fileNum - oldSong[i].fileNum) != 1) continue;
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
            oldSong[i].fileNum--;

            if(m_songList.size() == i+1) break;
            if((oldSong[i+1].fileNum - oldSong[i].fileNum) != 1) continue;
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
    moveRows(QModelIndex(), fromPosition, 1, QModelIndex(), toPosition);
}

bool PlaylistModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    if(sourceRow == destinationChild) return false;


    Song* song = m_songList.data();
    if(sourceRow < destinationChild)
    {
        beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild + 1);

        for(int i = sourceRow; i <= destinationChild; i++)
        {
            if(i > m_songList.size() - 1) continue;
            QFile file(m_dir.absolutePath() + "/" + QString::number(m_songList.at(i).fileNum) + ".ego");

            if(i == sourceRow)
            {
                file.remove();
                song[i].fileNum = song[destinationChild].fileNum;
            }
            else
            {
                if(i != destinationChild) file.remove();
                song[i].fileNum--;
            }
            writeEgoFile(song[i]);
        }
    }
    else
    {
        beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild);
        for(int i = sourceRow; i >= destinationChild; i--)
        {
            if(i < 0) continue;
            QFile file(m_dir.absolutePath() + "/" + QString::number(m_songList.at(i).fileNum) + ".ego");

            if(i == sourceRow)
            {
                file.remove();
                song[i].fileNum = song[destinationChild].fileNum;
            }
            else
            {
                if(i != destinationChild) file.remove();
                song[i].fileNum++;
            }
            writeEgoFile(song[i]);
        }
    }
    m_songList.move(sourceRow, destinationChild);

    endMoveRows();
    return true;
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
