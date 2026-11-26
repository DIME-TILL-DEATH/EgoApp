#ifndef PLAYLISTREPOSITORY_H
#define PLAYLISTREPOSITORY_H

#include <QList>
#include <QString>

#include "song.h"

// Data returned to the application layer; independent from QML and item models.
struct PlaylistData
{
    QString name;
    QString directoryPath;
    QList<Song> songs;
};

class PlaylistRepository
{
public:
    QList<PlaylistData> load(const QString &workspacePath) const;
    bool create(const QString &workspacePath, const QString &name) const;
    bool remove(const QString &workspacePath, const QString &name) const;

private:
    static bool isValidPlaylistName(const QString &name);
};

#endif // PLAYLISTREPOSITORY_H
