#ifndef SONG_H
#define SONG_H

#include <QString>

class Song
{
public:
    Song();

    void setPath(quint16 trackNum, const QString& path);
    void setFileNum(quint16 newFileNum);

    QString t1Path;
    QString t2Path;

    bool playNext{false};

    quint16 fileNum{0};

    QByteArray generateFileContent() const;
};

#endif // SONG_H
