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
        PlayNextRole = Qt::UserRole + 1,
        T1PathRole,
        T2PathRole,
        T1NameRole,
        T2NameRole
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

    Q_INVOKABLE void setLink(const QModelIndex &index, quint8 trackNum, QString path);

    QDir dir() const;
    void setDir(const QDir &newDir);

private:
    QString m_playlistName;
    QList<Song> m_songList;

    QDir m_dir;
};

#endif // PLAYLISTMODEL_H
