#include "song.h"

Song::Song() {}

void Song::setPath(quint16 trackNum, const QString &path)
{
    switch(trackNum)
    {
    case 0:
    {
        t1Path = path;
        break;
    }

    case 1:
    {
        t2Path = path;
        break;
    }

    default: return;
    }
}

void Song::setFileNum(quint16 newFileNum)
{
    fileNum = newFileNum;
}

QByteArray Song::generateFileContent() const
{
    QByteArray result;

    if(playNext) result.append(">");

    result.append(t1Path.toUtf8());
    result.append("\n");
    result.append(t2Path.toUtf8());

    return result;
}
