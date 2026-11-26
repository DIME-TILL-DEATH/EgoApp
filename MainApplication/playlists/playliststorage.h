#ifndef PLAYLISTSTORAGE_H
#define PLAYLISTSTORAGE_H

#include <QList>
#include <QStringList>
#include <QString>

#include "song.h"

// Reads and writes the existing numbered .ego files for one playlist directory.
struct PlaylistLoadResult
{
    QList<Song> songs;
    QStringList errors;
};

class PlaylistStorage
{
public:
    static PlaylistLoadResult loadSongs(const QString &directoryPath);
    static bool saveChanges(const QString &directoryPath,
                            const QList<Song> &oldSongs,
                            const QList<Song> &newSongs,
                            int firstIndex, int lastIndex,
                            QString *errorString = nullptr);
};

#endif // PLAYLISTSTORAGE_H
