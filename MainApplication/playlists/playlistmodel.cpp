#include "playlistmodel.h"

#include <QFileInfo>
#include <QDebug>

#include <limits>

namespace
{
bool incrementFileNum(quint16 &value)
{
    if (value == std::numeric_limits<quint16>::max())
        return false;
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
    , m_songList{songList}
{
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_songList.size();
}

QString PlaylistModel::workspaceFilePath(const QString &relativePath) const
{
    QString path = relativePath;
    while (path.startsWith('/') || path.startsWith('\\'))
        path.remove(0, 1);

    return QDir(m_workspacePath).absoluteFilePath(path);
}

void PlaylistModel::setWorkspacePath(const QString &workspacePath)
{
    m_workspacePath = QDir(workspacePath).absolutePath();
}

bool PlaylistModel::isTrackMissing(const QString &path) const
{
    if (path.isEmpty())
        return false;

    return !QFileInfo::exists(workspaceFilePath(path));
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount(QModelIndex()))
        return {};

    switch (role)
    {
    case ListRoles::FileNumRole:
        return m_songList.at(index.row()).fileNum;
    case ListRoles::PlayNextRole:
        return m_songList.at(index.row()).playNext;
    case ListRoles::T1PathRole:
        return m_songList.at(index.row()).t1Path;
    case ListRoles::T2PathRole:
        return m_songList.at(index.row()).t2Path;
    case ListRoles::T1NameRole:
    {
        const QString path = m_songList.at(index.row()).t1Path;
        const qsizetype fileNamePos = path.lastIndexOf('/');
        return path.mid(fileNamePos + 1);
    }
    case ListRoles::T2NameRole:
    {
        const QString path = m_songList.at(index.row()).t2Path;
        const qsizetype fileNamePos = path.lastIndexOf('/');
        return path.mid(fileNamePos + 1);
    }
    case ListRoles::MidiExistRole:
    {
        QString midiPath = workspaceFilePath(m_songList.at(index.row()).t1Path);
        midiPath.chop(3);
        midiPath += "mid";
        return QFileInfo::exists(midiPath);
    }
    case ListRoles::FileMissingRole:
        return isTrackMissing(m_songList.at(index.row()).t1Path)
               || isTrackMissing(m_songList.at(index.row()).t2Path);
    case ListRoles::T1MissingRole:
        return isTrackMissing(m_songList.at(index.row()).t1Path);
    case ListRoles::T2MissingRole:
        return isTrackMissing(m_songList.at(index.row()).t2Path);
    default:
        qWarning() << __FUNCTION__ << "no role";
        return true;
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    return {
        {ListRoles::FileNumRole, "fileNum"},
        {ListRoles::PlayNextRole, "playNext"},
        {ListRoles::T1PathRole, "t1Path"},
        {ListRoles::T2PathRole, "t2Path"},
        {ListRoles::T1NameRole, "t1Name"},
        {ListRoles::T2NameRole, "t2Name"},
        {ListRoles::MidiExistRole, "midiExist"},
        {ListRoles::FileMissingRole, "fileMissing"},
        {ListRoles::T1MissingRole, "t1Missing"},
        {ListRoles::T2MissingRole, "t2Missing"}
    };
}

void PlaylistModel::refreshModel(const QList<Song> &songList)
{
    beginResetModel();
    m_songList = songList;
    endResetModel();
}

bool PlaylistModel::insertRows(int position, int count, const QModelIndex &parent)
{
    if (parent.isValid() || count <= 0)
        return false;

    const int insertPosition = qBound(0, position, m_songList.size());
    QList<Song> updatedSongs = m_songList;

    for (int offset = 0; offset < count; ++offset)
    {
        const int currentPosition = insertPosition + offset;
        Song newSong;

        if (updatedSongs.isEmpty())
        {
            newSong.fileNum = 0;
        }
        else if (currentPosition == updatedSongs.size())
        {
            quint16 lastNum = updatedSongs.last().fileNum;
            if (!incrementFileNum(lastNum))
            {
                emit errorOccured(QObject::tr("Can't insert song: playlist numbering limit reached"));
                return false;
            }
            newSong.fileNum = lastNum;
        }
        else if (currentPosition == 0)
        {
            if (updatedSongs.first().fileNum > 0)
            {
                newSong.fileNum = updatedSongs.first().fileNum - 1;
            }
            else
            {
                for (int i = updatedSongs.size() - 1; i >= 0; --i)
                {
                    quint16 value = updatedSongs[i].fileNum;
                    if (!incrementFileNum(value))
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
            if (previousNum == std::numeric_limits<quint16>::max())
            {
                emit errorOccured(QObject::tr("Can't insert song: playlist numbering limit reached"));
                return false;
            }

            if (nextNum > previousNum + 1)
            {
                newSong.fileNum = previousNum + 1;
            }
            else
            {
                for (int i = updatedSongs.size() - 1; i >= currentPosition; --i)
                {
                    quint16 value = updatedSongs[i].fileNum;
                    if (!incrementFileNum(value))
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

    if (!persistSongList(updatedSongs))
        return false;

    beginInsertRows(parent, insertPosition, insertPosition + count - 1);
    m_songList = updatedSongs;
    endInsertRows();
    return true;
}

bool PlaylistModel::removeRows(int position, int count, const QModelIndex &parent)
{
    if (parent.isValid() || count <= 0 || position < 0
        || position + count > m_songList.size())
        return false;

    QList<Song> updatedSongs = m_songList;
    for (int i = 0; i < count; ++i)
        updatedSongs.removeAt(position);

    if (!persistSongList(updatedSongs))
        return false;

    beginRemoveRows(parent, position, position + count - 1);
    m_songList = updatedSongs;
    endRemoveRows();
    return true;
}

void PlaylistModel::moveSong(int fromPosition, int toPosition)
{
    const int currentRowCount = m_songList.size();
    if (fromPosition < 0 || fromPosition >= currentRowCount
        || toPosition < 0 || toPosition >= currentRowCount
        || fromPosition == toPosition)
        return;

    // moveSong uses the final row index; moveRows uses Qt's insertion position
    // before the source rows are removed.
    const int destinationChild = fromPosition < toPosition ? toPosition + 1 : toPosition;
    moveRows(QModelIndex(), fromPosition, 1, QModelIndex(), destinationChild);
}

bool PlaylistModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                             const QModelIndex &destinationParent, int destinationChild)
{
    if (sourceParent.isValid() || destinationParent.isValid())
        return false;

    const int currentRowCount = m_songList.size();
    if (count != 1 || sourceRow < 0 || sourceRow >= currentRowCount
        || destinationChild < 0 || destinationChild > currentRowCount)
        return false;

    // Qt disallows destinations inside or immediately adjacent to the source range.
    if (destinationChild >= sourceRow && destinationChild <= sourceRow + count)
        return false;

    const int targetRow = destinationChild > sourceRow
                              ? destinationChild - count
                              : destinationChild;

    QList<Song> updatedSongs = m_songList;
    updatedSongs.move(sourceRow, targetRow);

    QList<quint16> numberingScheme;
    numberingScheme.reserve(m_songList.size());
    for (const Song &song : m_songList)
        numberingScheme.append(song.fileNum);

    for (int i = 0; i < updatedSongs.size(); ++i)
        updatedSongs[i].fileNum = numberingScheme.at(i);

    // Persist first so a storage failure cannot leave the in-memory model changed.
    if (!persistSongList(updatedSongs))
        return false;

    if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1,
                       destinationParent, destinationChild))
    {
        // The prevalidated flat-list move should be accepted by Qt. Restore the
        // previous persisted state if Qt rejects the notification unexpectedly.
        persistSongList(m_songList);
        emit errorOccured(QObject::tr("Can't move song: the model rejected the move"));
        return false;
    }

    m_songList = updatedSongs;
    endMoveRows();
    return true;
}

void PlaylistModel::setLink(const QModelIndex &index, quint8 trackNum, QString path)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_songList.size()
        || trackNum > 1)
        return;

    QList<Song> updatedSongs = m_songList;
    updatedSongs[index.row()].setPath(trackNum, path);
    if (!persistSongList(updatedSongs))
        return;

    m_songList = updatedSongs;

    QList<int> changedRoles;
    if (trackNum == 0)
    {
        changedRoles = {
            T1PathRole,
            T1NameRole,
            MidiExistRole,
            FileMissingRole,
            T1MissingRole
        };
    }
    else
    {
        changedRoles = {
            T2PathRole,
            T2NameRole,
            FileMissingRole,
            T2MissingRole
        };
    }

    emit dataChanged(index, index, changedRoles);
}

void PlaylistModel::setPlayNext(const QModelIndex &index, bool state)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_songList.size())
        return;

    QList<Song> updatedSongs = m_songList;
    updatedSongs[index.row()].playNext = state;
    if (!persistSongList(updatedSongs))
        return;

    m_songList = updatedSongs;
    emit dataChanged(index, index, {PlayNextRole});
}

QDir PlaylistModel::dir() const
{
    return m_dir;
}

void PlaylistModel::setDir(const QDir &newDir)
{
    m_dir = newDir;
}

bool PlaylistModel::persistSongList(const QList<Song> &songList)
{
    QString errorString;
    if (!m_storage.saveSongs(m_dir.absolutePath(), songList, &errorString))
    {
        emit errorOccured(errorString);
        return false;
    }

    return true;
}
