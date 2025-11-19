#include "song.h"

Song::Song() {}

QByteArray Song::generateFileContent()
{
    QByteArray result;

    if(playNext) result.append(">");

    result.append(t1Path.toUtf8());
    result.append("\n");
    result.append(t2Path.toUtf8());

    return result;
}
