#include "playlistmodel.h"

#include <QFile>
#include <limits>
#include <QSaveFile>
#include <QSet>

namespace
{
bool incrementFileNum(quint16 &value)
{
    if(value == std::numeric_limits<quint16>::max()) return false;
    ++value;
    return true;
}
}

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

bool PlaylistModel::isTrackMissing(const QString &path) const
{
    if(path.isEmpty()) return false;

    QDir wrkSpaceDir = m_dir;
    wrkSpaceDir.cd("../..");
    return !QFileInfo::exists(wrkSpaceDir.absolutePath() + path);
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= rowCount(QModelIndex()))
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

        case ListRoles::FileMissingRole: return isTrackMissing(m_songList.at(index.row()).t1Path)
                                             || isTrackMissing(m_songList.at(index.row()).t2Path);
        case ListRoles::T1MissingRole: return isTrackMissing(m_songList.at(index.row()).t1Path);
        case ListRoles::T2MissingRole: return isTrackMissing(m_songList.at(index.row()).t2Path);

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
    roles[ListRoles::FileMissingRole] = "fileMissing";
    roles[ListRoles::T1MissingRole] = "t1Missing";
    roles[ListRoles::T2MissingRole] = "t2Missing";

    return roles;
}

void PlaylistModel::refreshModel(const QList<Song> &songList)
{

}

bool PlaylistModel::insertRows(int position, int count, const QModelIndex &parent)
{
    if(parent.isValid() || count <= 0) return false;

    const int insertPosition = qBound(0, position, m_songList.size());
    QList<Song> updatedSongs = m_songList;

    for(int offset = 0; offset < count; ++offset)
    {
        const int currentPosition = insertPosition + offset;
        Song newSong;

        if(updatedSongs.isEmpty())
        {
            newSong.fileNum = 0;
        }
        else if(currentPosition == updatedSongs.size())
        {
            quint16 lastNum = updatedSongs.last().fileNum;
            if(!incrementFileNum(lastNum))
            {
                emit errorOccured(QObject::tr("Can't insert song: playlist numbering limit reached"));
                return false;
            }
            newSong.fileNum = lastNum;
        }
        else if(currentPosition == 0)
        {
            if(updatedSongs.first().fileNum > 0)
            {
                newSong.fileNum = updatedSongs.first().fileNum - 1;
            }
            else
            {
                for(int i = updatedSongs.size() - 1; i >= 0; --i)
                {
                    quint16 value = updatedSongs[i].fileNum;
                    if(!incrementFileNum(value))
                    {
                        emit errorOccured(QObject::tr("Can't insert song: playlist numbering limit reached"));
                        return false;
                    }
                    updatedSongs[i].fileNum = value;
                }
                newSong.fileNum = 0;
            }
        }
        else
        {
            const quint16 previousNum = updatedSongs.at(currentPosition - 1).fileNum;
            const quint16 nextNum = updatedSongs.at(currentPosition).fileNum;
            if(previousNum == std::numeric_limits<quint16>::max())
            {
                emit errorOccured(QObject::tr("Can't insert song: playlist numbering limit reached"));
                return false;
            }

            if(nextNum > previousNum + 1)
            {
                newSong.fileNum = previousNum + 1;
            }
            else
            {
                for(int i = updatedSongs.size() - 1; i >= currentPosition; --i)
                {
                    quint16 value = updatedSongs[i].fileNum;
                    if(!incrementFileNum(value))
                    {
                        emit errorOccured(QObject::tr("Can't insert song: playlist numbering limit reached"));
                        return false;
                    }
                    updatedSongs[i].fileNum = value;
                }
                newSong.fileNum = previousNum + 1;
            }
        }

        updatedSongs.insert(currentPosition, newSong);
    }

    if(!persistSongList(updatedSongs)) return false;

    beginInsertRows(parent, insertPosition, insertPosition + count - 1);
    m_songList = updatedSongs;
    endInsertRows();
    return true;
}

bool PlaylistModel::removeRows(int position, int count, const QModelIndex &parent)
{
    if(parent.isValid() || count <= 0 || position < 0 || position + count > m_songList.size()) return false;

    QList<Song> updatedSongs = m_songList;
    for(int i = 0; i < count; ++i)
        updatedSongs.removeAt(position);

    if(!persistSongList(updatedSongs)) return false;

    beginRemoveRows(parent, position, position + count - 1);
    m_songList = updatedSongs;
    endRemoveRows();
    return true;
}

void PlaylistModel::moveSong(quint16 fromPosition, quint16 toPosition)
{
    moveRows(QModelIndex(), fromPosition, 1, QModelIndex(), toPosition);
}

bool PlaylistModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    if(sourceParent.isValid() || destinationParent.isValid()) return false;
    if(count != 1) return false;
    if(sourceRow < 0 || sourceRow >= m_songList.size()) return false;
    if(destinationChild < 0 || destinationChild >= m_songList.size()) return false;
    if(sourceRow == destinationChild) return false;

    QList<Song> updatedSongs = m_songList;
    updatedSongs.move(sourceRow, destinationChild);

    QList<quint16> numberingScheme;
    numberingScheme.reserve(m_songList.size());
    for(const Song &song : m_songList)
        numberingScheme.append(song.fileNum);

    for(int i = 0; i < updatedSongs.size(); ++i)
        updatedSongs[i].fileNum = numberingScheme.at(i);

    if(!persistSongList(updatedSongs)) return false;

    const int destinationRow = sourceRow < destinationChild ? destinationChild + 1 : destinationChild;
    beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationRow);
    m_songList = updatedSongs;
    endMoveRows();
    return true;
}

void PlaylistModel::setLink(const QModelIndex &index, quint8 trackNum, QString path)
{
    if(!index.isValid()) return;
    const int songNum = index.row();
    if(songNum < 0 || songNum >= m_songList.size()) return;

    QList<Song> updatedSongs = m_songList;
    updatedSongs[songNum].setPath(trackNum, path);
    if(!persistSongList(updatedSongs)) return;

    m_songList = updatedSongs;
    emit dataChanged(index, index);
}

void PlaylistModel::setPlayNext(const QModelIndex &index, bool state)
{
    if(!index.isValid()) return;
    const int songNum = index.row();
    if(songNum < 0 || songNum >= m_songList.size()) return;

    QList<Song> updatedSongs = m_songList;
    updatedSongs[songNum].playNext = state;
    if(!persistSongList(updatedSongs)) return;

    m_songList = updatedSongs;
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

bool PlaylistModel::writeEgoFile(const Song &song)
{
    const QString filePath = m_dir.absoluteFilePath(QString::number(song.fileNum) + ".ego");
    QSaveFile egoFile(filePath);
    if(!egoFile.open(QIODevice::WriteOnly))
    {
        emit errorOccured(QObject::tr("Can't open playlist file for writing: ") + filePath);
        return false;
    }

    const QByteArray content = song.generateFileContent();
    if(egoFile.write(content) != content.size())
    {
        egoFile.cancelWriting();
        emit errorOccured(QObject::tr("Can't write playlist file: ") + filePath);
        return false;
    }

    if(!egoFile.commit())
    {
        emit errorOccured(QObject::tr("Can't finalize playlist file: ") + filePath);
        return false;
    }

    return true;
}

bool PlaylistModel::persistSongList(const QList<Song> &songList)
{
    QSet<QString> expectedFiles;
    for(const Song &song : songList)
    {
        expectedFiles.insert(QString::number(song.fileNum) + ".ego");
        if(!writeEgoFile(song)) return false;
    }

    const QStringList existingFiles = m_dir.entryList(QStringList() << "*.ego", QDir::Files, QDir::Name);
    for(const QString &fileName : existingFiles)
    {
        if(expectedFiles.contains(fileName)) continue;
        if(!QFile::remove(m_dir.absoluteFilePath(fileName)))
            emit errorOccured(QObject::tr("Can't remove stale playlist file: ") + fileName);
    }

    return true;
}
