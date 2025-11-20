#ifndef SONG_H
#define SONG_H

#include <QString>

class Song
{
public:
    Song();

    void setPath(quint16 trackNum, const QString& path);

    QString t1Path;
    QString t2Path;

    bool playNext{false};

    QByteArray generateFileContent();
};

#endif // SONG_H
