#include "playliststorage.h"

#include <QObject>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>
#include <QHash>
#include <QStringDecoder>
#include <QRegularExpression>
#include <limits>

#include <algorithm>

PlaylistLoadResult PlaylistStorage::loadSongs(const QString &directoryPath)
{
    PlaylistLoadResult result;
    const QDir directory(directoryPath);
    const QFileInfo directoryInfo(directoryPath);

    if (!directoryInfo.exists() || !directoryInfo.isDir() || !directoryInfo.isReadable())
    {
        result.errors.append(QObject::tr("Can't read playlist directory: ") + directoryPath);
        return result;
    }

    struct EgoFile
    {
        QString name;
        quint16 number;
    };

    QList<EgoFile> validFiles;
    QHash<quint16, int> numberOccurrences;

    const QStringList egoFiles =
        directory.entryList({"*.ego"}, QDir::Files, QDir::Name);

    for (const QString &fileName : egoFiles)
    {
        QString numberText = fileName;
        numberText.chop(4); // ".ego"

        static const QRegularExpression numericName(QStringLiteral("^[0-9]+$"));
        if (!numericName.match(numberText).hasMatch())
        {
            result.errors.append(fileName + QObject::tr(": invalid .ego filename (expected a number from 0 to 65535)"));
            continue;
        }

        bool numberOk = false;
        const qulonglong parsedNumber = numberText.toULongLong(&numberOk, 10);
        if (!numberOk || parsedNumber > std::numeric_limits<quint16>::max())
        {
            result.errors.append(fileName + QObject::tr(": invalid .ego filename (expected a number from 0 to 65535)"));
            continue;
        }

        const quint16 number = static_cast<quint16>(parsedNumber);
        ++numberOccurrences[number];
        validFiles.append({fileName, number});
    }

    for (auto it = numberOccurrences.cbegin(); it != numberOccurrences.cend(); ++it)
    {
        if (it.value() > 1)
        {
            for (const EgoFile &egoFile : validFiles)
            {
                if (egoFile.number == it.key())
                    result.errors.append(egoFile.name + QObject::tr(": duplicate .ego file number"));
            }
        }
    }

    std::sort(validFiles.begin(), validFiles.end(),
              [](const EgoFile &left, const EgoFile &right) {
                  if (left.number != right.number)
                      return left.number < right.number;
                  return left.name < right.name;
              });

    const auto decodeLine = [](QByteArray line, QString *decoded) {
        if (line.endsWith('\n'))
            line.chop(1);
        if (line.endsWith('\r'))
            line.chop(1);

        QStringDecoder decoder(QStringDecoder::Utf8);
        *decoded = decoder.decode(line);
        return !decoder.hasError();
    };

    for (const EgoFile &egoFile : validFiles)
    {
        if (numberOccurrences.value(egoFile.number) > 1)
            continue;

        const QString filePath = directory.absoluteFilePath(egoFile.name);
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            result.errors.append(egoFile.name + QObject::tr(": can't open file: ") + file.errorString());
            continue;
        }

        const QByteArray firstLine = file.readLine();
        if (firstLine.isEmpty())
        {
            result.errors.append(egoFile.name + QObject::tr(": empty or unreadable file"));
            continue;
        }

        if (!firstLine.endsWith('\n'))
        {
            result.errors.append(egoFile.name + QObject::tr(": incomplete first line (missing newline)"));
            continue;
        }

        QByteArray track1Bytes = firstLine;
        bool playNext = false;
        if (track1Bytes.startsWith('>'))
        {
            playNext = true;
            track1Bytes.remove(0, 1);
        }

        Song song;
        if (!decodeLine(track1Bytes, &song.t1Path))
        {
            result.errors.append(egoFile.name + QObject::tr(": invalid UTF-8 in first line"));
            continue;
        }

        const QByteArray secondLine = file.readLine();
        if (!secondLine.isEmpty() && !decodeLine(secondLine, &song.t2Path))
        {
            result.errors.append(egoFile.name + QObject::tr(": invalid UTF-8 in second line"));
            continue;
        }

        if (!file.readAll().isEmpty())
        {
            result.errors.append(egoFile.name + QObject::tr(": unexpected data after second line"));
            continue;
        }

        if (file.error() != QFile::NoError)
        {
            result.errors.append(egoFile.name + QObject::tr(": read error: ") + file.errorString());
            continue;
        }

        song.playNext = playNext;
        song.fileNum = egoFile.number;
        result.songs.append(song);
    }

    return result;
}

bool PlaylistStorage::saveSongs(const QString &directoryPath, const QList<Song> &songs,
                                QString *errorString)
{
    const QDir directory(directoryPath);
    if (!directory.exists())
    {
        if (errorString)
            *errorString = QObject::tr("Playlist directory does not exist: ") + directoryPath;
        return false;
    }

    QSet<QString> expectedFiles;
    for (const Song &song : songs)
    {
        const QString fileName = QString::number(song.fileNum) + ".ego";
        expectedFiles.insert(fileName);

        const QString filePath = directory.absoluteFilePath(fileName);
        QSaveFile egoFile(filePath);
        if (!egoFile.open(QIODevice::WriteOnly))
        {
            if (errorString)
                *errorString = QObject::tr("Can't open playlist file for writing: ") + filePath;
            return false;
        }

        const QByteArray content = song.generateFileContent();
        if (egoFile.write(content) != content.size())
        {
            egoFile.cancelWriting();
            if (errorString)
                *errorString = QObject::tr("Can't write playlist file: ") + filePath;
            return false;
        }

        if (!egoFile.commit())
        {
            if (errorString)
                *errorString = QObject::tr("Can't finalize playlist file: ") + filePath;
            return false;
        }
    }

    const QStringList existingFiles =
        directory.entryList({"*.ego"}, QDir::Files, QDir::Name);
    for (const QString &fileName : existingFiles)
    {
        if (expectedFiles.contains(fileName))
            continue;

        const QString filePath = directory.absoluteFilePath(fileName);
        if (!QFile::remove(filePath))
        {
            if (errorString)
                *errorString = QObject::tr("Can't remove stale playlist file: ") + filePath;
            return false;
        }
    }

    return true;
}
