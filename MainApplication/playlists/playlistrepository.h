#ifndef PLAYLISTREPOSITORY_H
#define PLAYLISTREPOSITORY_H

#include <QList>
#include <QString>

#include "song.h"

// Data returned by the persistence layer. It deliberately contains no
// QAbstractItemModel or QML-facing objects.
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

    // Returns false for an invalid name, an existing directory, or an I/O
    // failure. Playlist names are single directory components.
    bool create(const QString &workspacePath, const QString &name) const;

    // Removes only a playlist directory canonically located under PLAYLIST.
    bool remove(const QString &workspacePath, const QString &name) const;

private:
    static bool isValidPlaylistName(const QString &name);
};

#endif // PLAYLISTREPOSITORY_H
