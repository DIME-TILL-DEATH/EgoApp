#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

#include "song.h"

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistModel(QObject *parent = nullptr);
    PlaylistModel(const QList<Song>& songList, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refreshModel(const QList<Song>& songList);

    QString playlistName() const {return m_playlistName;};
    void setPlaylistName(const QString& newName) {m_playlistName = newName;};

    enum ListRoles{
        PlayNextRole = Qt::UserRole + 1,
        T1PathRole,
        T2PathRole,
        T1NameRole,
        T2NameRole
    };
    Q_ENUM(ListRoles)

private:
    QString m_playlistName;
    QList<Song> m_songList;

};

#endif // PLAYLISTMODEL_H
