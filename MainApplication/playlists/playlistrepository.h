#ifndef PLAYLISTREPOSITORY_H
#define PLAYLISTREPOSITORY_H

#include <QList>
#include <QString>

#include "song.h"

struct PlaylistData
{
    QString name;
    QString relativePath;
    QString directoryPath;
    QList<Song> songs;
};

class PlaylistRepository
{
public:
    // Every directory below PLAYLIST is exposed as a playlist. Directories may
    // contain both their own .ego files and child playlist directories.
    QList<PlaylistData> load(const QString &workspacePath) const;
    bool create(const QString &workspacePath, const QString &relativePath) const;
    bool remove(const QString &workspacePath, const QString &relativePath) const;

private:
    static bool isValidRelativePath(const QString &relativePath);
};

#endif // PLAYLISTREPOSITORY_H
