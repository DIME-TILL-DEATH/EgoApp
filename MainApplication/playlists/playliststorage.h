#ifndef PLAYLISTSTORAGE_H
#define PLAYLISTSTORAGE_H

#include <QList>
#include <QString>

#include "song.h"

// Reads and writes the existing numbered .ego files for one playlist directory.
class PlaylistStorage
{
public:
    static QList<Song> loadSongs(const QString &directoryPath);
    static bool saveSongs(const QString &directoryPath, const QList<Song> &songs,
                          QString *errorString = nullptr);
};

#endif // PLAYLISTSTORAGE_H
