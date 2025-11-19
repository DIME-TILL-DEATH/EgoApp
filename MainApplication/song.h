#ifndef SONG_H
#define SONG_H

#include <QString>

class Song
{
public:
    Song();

    QString t1Path;
    QString t2Path;

    bool playNext{false};

    QByteArray generateFileContent();
};

#endif // SONG_H
