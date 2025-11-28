#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QDir>
#include <QQmlEngine>

#include "song.h"

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum ListRoles{
        FileNumRole = Qt::UserRole + 1,
        PlayNextRole,
        T1PathRole,
        T2PathRole,
        T1NameRole,
        T2NameRole,
        MidiExistRole
    };
    Q_ENUM(ListRoles)

    PlaylistModel(QObject *parent = nullptr);
    PlaylistModel(const QList<Song>& songList, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refreshModel(const QList<Song>& songList);

    QString playlistName() const {return m_playlistName;};
    void setPlaylistName(const QString& newName) {m_playlistName = newName;};

    Q_INVOKABLE bool insertRows(int position, int count, const QModelIndex &parent = QModelIndex()) override;
    Q_INVOKABLE bool removeRows(int position, int count, const QModelIndex &parent = QModelIndex()) override;
    Q_INVOKABLE void moveSong(quint16 fromPosition, quint16 toPosition);

    Q_INVOKABLE void setLink(const QModelIndex &index, quint8 trackNum, QString path);
    Q_INVOKABLE void setPlayNext(const QModelIndex &index, bool state);

    QDir dir() const;
    void setDir(const QDir &newDir);

private:
    QString m_playlistName;
    QList<Song> m_songList;

    QDir m_dir;

    void writeEgoFile(const Song& song);

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
};

#endif // PLAYLISTMODEL_H
