#include "playlistmodel.h"
#include "mediapath.h"

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

void PlaylistModel::setWorkspacePath(const QString &workspacePath)
{
    m_workspacePath = QDir(workspacePath).absolutePath();
}

void PlaylistModel::setCorrupted(bool corrupted)
{
    m_corrupted = corrupted;
    m_persistenceEnabled = !corrupted;
}

bool PlaylistModel::resolveMediaPath(const QString &storedPath, QString *resolvedPath) const
{
    return MediaPath::resolveWorkspacePath(m_workspacePath, storedPath, resolvedPath);
}

bool PlaylistModel::isTrackMissing(const QString &path) const
{
    if (path.isEmpty())
        return false;

    QString resolvedPath;
    if (!MediaPath::resolveWorkspacePath(m_workspacePath, path, &resolvedPath))
        return true;

    const QFileInfo fileInfo(resolvedPath);
    return !fileInfo.exists() || !fileInfo.isFile();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0
        || index.row() < 0 || index.row() >= rowCount(QModelIndex()))
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
        QString trackPath;
        if (!MediaPath::resolveWorkspacePath(
                m_workspacePath, m_songList.at(index.row()).t1Path, &trackPath))
            return false;

        const QFileInfo trackInfo(trackPath);
        if (!trackInfo.exists() || !trackInfo.isFile()
            || trackInfo.suffix().compare(QStringLiteral("wav"), Qt::CaseInsensitive) != 0)
            return false;

        const QString midiPath =
            trackInfo.dir().filePath(trackInfo.completeBaseName() + QStringLiteral(".mid"));
        const QFileInfo midiInfo(midiPath);
        return midiInfo.exists() && midiInfo.isFile();
    }

    case ListRoles::FileMissingRole:
        return isTrackMissing(m_songList.at(index.row()).t1Path)
               || isTrackMissing(m_songList.at(index.row()).t2Path);

    case ListRoles::T1MissingRole:
        return isTrackMissing(m_songList.at(index.row()).t1Path);

    case ListRoles::T2MissingRole:
        return isTrackMissing(m_songList.at(index.row()).t2Path);

    default:
        return {};
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

bool PlaylistModel::insertRows(int position, int count, const QModelIndex &parent)
{
    if (!ensureEditable())
        return false;

    const int currentRowCount = m_songList.size();
    if (parent.isValid() || count <= 0 || position < 0
        || position > currentRowCount
        || count > std::numeric_limits<int>::max() - currentRowCount)
        return false;

    const int insertPosition = position;
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

    if (!persistSongRange(updatedSongs, insertPosition, updatedSongs.size() - 1))
        return false;

    beginInsertRows(parent, insertPosition, insertPosition + count - 1);
    m_songList = updatedSongs;
    endInsertRows();
    return true;
}

bool PlaylistModel::removeRows(int position, int count, const QModelIndex &parent)
{
    if (!ensureEditable())
        return false;

    const int currentRowCount = m_songList.size();
    if (parent.isValid() || count <= 0 || position < 0
        || position >= currentRowCount || count > currentRowCount - position)
        return false;

    QList<Song> updatedSongs = m_songList;
    for (int i = 0; i < count; ++i)
        updatedSongs.removeAt(position);

    // Keep the remaining file numbers contiguous after deletion. Only the
    // suffix beginning at the first removed row can need new .ego numbers.
    quint32 nextFileNum = position > 0
                              ? static_cast<quint32>(updatedSongs.at(position - 1).fileNum) + 1
                              : 0;
    for (int i = position; i < updatedSongs.size(); ++i)
    {
        if (nextFileNum > std::numeric_limits<quint16>::max())
        {
            emit errorOccured(QObject::tr("Can't remove song: playlist numbering limit reached"));
            return false;
        }
        updatedSongs[i].fileNum = static_cast<quint16>(nextFileNum++);
    }

    if (!persistSongRange(updatedSongs, position,
                          qMax(m_songList.size(), updatedSongs.size()) - 1))
        return false;

    beginRemoveRows(parent, position, position + count - 1);
    m_songList = updatedSongs;
    endRemoveRows();
    return true;
}

bool PlaylistModel::moveSong(int fromPosition, int toPosition)
{
    if (!ensureEditable())
        return false;

    const int currentRowCount = m_songList.size();
    if (fromPosition < 0 || fromPosition >= currentRowCount
        || toPosition < 0 || toPosition >= currentRowCount)
        return false;

    if (fromPosition == toPosition)
        return true;

    // moveSong uses the final row index; moveRows uses Qt's insertion position
    // before the source rows are removed.
    const int destinationChild = fromPosition < toPosition ? toPosition + 1 : toPosition;
    return moveRows(QModelIndex(), fromPosition, 1, QModelIndex(), destinationChild);
}

bool PlaylistModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                             const QModelIndex &destinationParent, int destinationChild)
{
    if (!ensureEditable() || sourceParent.isValid() || destinationParent.isValid())
        return false;

    const int currentRowCount = m_songList.size();
    // Playlist editing supports moving one song at a time; grouped moves are
    // deliberately not part of the public model contract.
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
    if (!persistSongRange(updatedSongs, qMin(sourceRow, targetRow), qMax(sourceRow, targetRow)))
        return false;

    if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1,
                       destinationParent, destinationChild))
    {
        // The prevalidated flat-list move should be accepted by Qt. Restore the
        // previous persisted state if Qt rejects the notification unexpectedly.
        QString rollbackError;
        if (!m_storage.saveChanges(m_dir.absolutePath(), updatedSongs, m_songList,
                                   qMin(sourceRow, targetRow), qMax(sourceRow, targetRow),
                                   &rollbackError))
        {
            emit errorOccured(QObject::tr("Can't move song: the model rejected the move, and restoring the previous playlist state failed: %1. Check the SD card.")
                                  .arg(rollbackError));
        }
        else
        {
            emit errorOccured(QObject::tr("Can't move song: the model rejected the move. The previous playlist state was restored."));
        }
        return false;
    }

    m_songList = updatedSongs;
    endMoveRows();
    return true;
}

bool PlaylistModel::setLink(const QModelIndex &index, quint8 trackNum, QString path)
{
    if (!ensureEditable())
        return false;

    if (!index.isValid() || index.model() != this
        || index.row() < 0 || index.row() >= m_songList.size() || trackNum > 1)
        return false;

    QList<Song> updatedSongs = m_songList;
    updatedSongs[index.row()].setPath(trackNum, path);
    if (!persistSongRange(updatedSongs, index.row(), index.row()))
        return false;

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
    return true;
}

bool PlaylistModel::setPlayNext(const QModelIndex &index, bool state)
{
    if (!ensureEditable())
        return false;

    if (!index.isValid() || index.model() != this
        || index.row() < 0 || index.row() >= m_songList.size())
        return false;

    QList<Song> updatedSongs = m_songList;
    updatedSongs[index.row()].playNext = state;
    if (!persistSongRange(updatedSongs, index.row(), index.row()))
        return false;

    m_songList = updatedSongs;
    emit dataChanged(index, index, {PlayNextRole});
    return true;
}

QDir PlaylistModel::dir() const
{
    return m_dir;
}

void PlaylistModel::setDir(const QDir &newDir)
{
    m_dir = newDir;
}

bool PlaylistModel::ensureEditable()
{
    if (m_persistenceEnabled)
        return true;

    emit errorOccured(QObject::tr("Playlist editing is disabled because one or more .ego files could not be loaded. Fix the reported files before editing to avoid data loss."));
    return false;
}

bool PlaylistModel::persistSongRange(const QList<Song> &songList, int firstIndex, int lastIndex)
{
    // Keep this final guard even though public mutation methods check before
    // preparing their changes. It protects future call paths as well.
    if (!ensureEditable())
        return false;

    QString errorString;
    if (!m_storage.saveChanges(m_dir.absolutePath(), m_songList, songList,
                               firstIndex, lastIndex, &errorString))
    {
        emit errorOccured(errorString);
        return false;
    }

    return true;
}
