#ifndef PLAYLISTSERVICE_H
#define PLAYLISTSERVICE_H

#include <QObject>
#include <QStringList>

#include "playlistmodel.h"
#include "playlistrepository.h"

// Coordinates playlist discovery, lifecycle, and model construction.
// It deliberately contains no QML-facing API.
class PlaylistService : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistService(QObject *parent = nullptr);

    void setWorkspacePath(const QString &workspacePath);
    QString workspacePath() const;

    QList<PlaylistModel *> playlists() const;
    QStringList playlistPaths() const;
    PlaylistModel *playlistAt(int index) const;
    int indexOfPath(const QString &relativePath) const;

    bool createPlaylist(const QString &relativePath, QString *errorString = nullptr);
    bool removePlaylist(const QString &relativePath, QString *errorString = nullptr);
    QStringList checkPlaylist(const QString &relativePath) const;
    void reload();

signals:
    void errorOccurred(const QString &errorString);
    void loadIssuesFound(const QStringList &issues);

private:
    void clearModels();

    QString m_workspacePath;
    PlaylistRepository m_repository;
    QList<PlaylistModel *> m_playlists;
};

#endif // PLAYLISTSERVICE_H
