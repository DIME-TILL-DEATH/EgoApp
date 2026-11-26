#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractListModel>
#include <QDir>
#include <QObject>
#include <QQmlEngine>

#include "playliststorage.h"
#include "song.h"

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum ListRoles {
        FileNumRole = Qt::UserRole + 1,
        PlayNextRole,
        T1PathRole,
        T2PathRole,
        T1NameRole,
        T2NameRole,
        MidiExistRole,
        FileMissingRole,
        T1MissingRole,
        T2MissingRole
    };
    Q_ENUM(ListRoles)

    explicit PlaylistModel(QObject *parent = nullptr);
    PlaylistModel(const QList<Song> &songList, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refreshModel(const QList<Song> &songList);

    QString playlistName() const { return m_playlistName; }
    void setPlaylistName(const QString &newName) { m_playlistName = newName; }

    QString playlistPath() const { return m_playlistPath; }
    void setPlaylistPath(const QString &newPath) { m_playlistPath = newPath; }

    void setWorkspacePath(const QString &workspacePath);
    void setPersistenceEnabled(bool enabled);

    Q_INVOKABLE bool insertRows(int position, int count,
                                const QModelIndex &parent = QModelIndex()) override;
    Q_INVOKABLE bool removeRows(int position, int count,
                                const QModelIndex &parent = QModelIndex()) override;
    Q_INVOKABLE bool moveSong(int fromPosition, int toPosition);

    Q_INVOKABLE void setLink(const QModelIndex &index, quint8 trackNum, QString path);
    Q_INVOKABLE void setPlayNext(const QModelIndex &index, bool state);

    QDir dir() const;
    void setDir(const QDir &newDir);

signals:
    void errorOccured(QString errorString);

private:
    QString m_playlistName;
    QString m_playlistPath;
    QString m_workspacePath;
    QList<Song> m_songList;
    QDir m_dir;
    PlaylistStorage m_storage;
    bool m_persistenceEnabled{true};

    bool isTrackMissing(const QString &path) const;
    bool persistSongList(const QList<Song> &songList);

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent, int destinationChild) override;
};

#endif // PLAYLISTMODEL_H
