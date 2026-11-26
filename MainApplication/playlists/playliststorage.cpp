#include "playliststorage.h"

#include <QObject>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QSaveFile>
#include <QTemporaryDir>
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

    // QDir does not expose a distinct enumeration error status. Verify the
    // directory both before and after listing; an empty readable directory is
    // valid, while a directory lost during enumeration is reported.
    const QStringList egoFiles =
        directory.entryList({"*.ego"}, QDir::Files | QDir::NoSymLinks, QDir::Name);

    QFileInfo directoryInfoAfterListing(directoryPath);
    directoryInfoAfterListing.refresh();
    if (!directoryInfoAfterListing.exists() || !directoryInfoAfterListing.isDir()
        || !directoryInfoAfterListing.isReadable())
    {
        result.errors.append(QObject::tr("Playlist directory became unavailable while listing .ego files: ")
                             + directoryPath);
        return result;
    }

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
        QFileInfo fileInfoBefore(filePath);
        fileInfoBefore.refresh();
        if (!fileInfoBefore.exists() || !fileInfoBefore.isFile() || fileInfoBefore.isSymLink())
        {
            result.errors.append(egoFile.name
                                 + QObject::tr(": file became unavailable after directory listing"));
            continue;
        }

        const qint64 sizeBeforeRead = fileInfoBefore.size();
        const QDateTime modifiedBeforeRead = fileInfoBefore.lastModified();

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            result.errors.append(egoFile.name + QObject::tr(": can't open file: ") + file.errorString());
            continue;
        }

        const QByteArray contents = file.readAll();
        if (file.error() != QFile::NoError)
        {
            result.errors.append(egoFile.name + QObject::tr(": read error: ") + file.errorString());
            continue;
        }

        QFileInfo fileInfoAfter(filePath);
        fileInfoAfter.refresh();
        if (!fileInfoAfter.exists() || !fileInfoAfter.isFile() || fileInfoAfter.isSymLink())
        {
            result.errors.append(egoFile.name
                                 + QObject::tr(": file disappeared or changed type while being read"));
            continue;
        }

        if (fileInfoAfter.size() != sizeBeforeRead
            || fileInfoAfter.lastModified() != modifiedBeforeRead)
        {
            result.errors.append(egoFile.name
                                 + QObject::tr(": file changed while being read"));
            continue;
        }

        if (contents.isEmpty())
        {
            result.errors.append(egoFile.name + QObject::tr(": empty file"));
            continue;
        }

        const qsizetype firstLineEnd = contents.indexOf('\n');
        if (firstLineEnd < 0)
        {
            result.errors.append(egoFile.name + QObject::tr(": incomplete first line (missing newline)"));
            continue;
        }

        QByteArray firstLine = contents.left(firstLineEnd + 1);
        const QByteArray remaining = contents.mid(firstLineEnd + 1);

        QByteArray secondLine;
        bool hasTrailingData = false;
        if (!remaining.isEmpty())
        {
            const qsizetype secondLineEnd = remaining.indexOf('\n');
            if (secondLineEnd < 0)
            {
                secondLine = remaining;
            }
            else
            {
                secondLine = remaining.left(secondLineEnd + 1);
                hasTrailingData = secondLineEnd + 1 < remaining.size();
            }
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

        if (!secondLine.isEmpty() && !decodeLine(secondLine, &song.t2Path))
        {
            result.errors.append(egoFile.name + QObject::tr(": invalid UTF-8 in second line"));
            continue;
        }

        if (hasTrailingData)
        {
            result.errors.append(egoFile.name + QObject::tr(": unexpected data after second line"));
            continue;
        }

        song.playNext = playNext;
        song.fileNum = egoFile.number;
        result.songs.append(song);
    }

    return result;
}

bool PlaylistStorage::saveChanges(const QString &directoryPath,
                                  const QList<Song> &oldSongs,
                                  const QList<Song> &newSongs,
                                  int firstIndex, int lastIndex,
                                  QString *errorString)
{
    const QDir directory(directoryPath);
    if (!directory.exists())
    {
        if (errorString)
            *errorString = QObject::tr("Playlist directory does not exist: ") + directoryPath;
        return false;
    }

    QHash<QString, QByteArray> oldContents;
    QHash<QString, QByteArray> newContents;
    for (const Song &song : oldSongs)
        oldContents.insert(QString::number(song.fileNum) + ".ego", song.generateFileContent());
    for (const Song &song : newSongs)
        newContents.insert(QString::number(song.fileNum) + ".ego", song.generateFileContent());

    QStringList filesToWrite;
    QStringList filesToRemove;
    QSet<QString> affectedFiles;

    const int rowCount = qMax(oldSongs.size(), newSongs.size());
    if (rowCount == 0)
        return true;
    if (firstIndex < 0 || lastIndex < firstIndex || firstIndex >= rowCount || lastIndex >= rowCount)
    {
        if (errorString)
            *errorString = QObject::tr("Invalid playlist persistence range.");
        return false;
    }

    for (int i = firstIndex; i <= lastIndex && i < newSongs.size(); ++i)
    {
        const Song &song = newSongs.at(i);
        const QString fileName = QString::number(song.fileNum) + ".ego";
        const QByteArray content = newContents.value(fileName);
        if (oldContents.contains(fileName) && oldContents.value(fileName) == content)
            continue;
        filesToWrite.append(fileName);
        affectedFiles.insert(fileName);
    }

    for (int i = firstIndex; i <= lastIndex && i < oldSongs.size(); ++i)
    {
        const Song &song = oldSongs.at(i);
        const QString fileName = QString::number(song.fileNum) + ".ego";
        if (newContents.contains(fileName))
            continue;
        filesToRemove.append(fileName);
        affectedFiles.insert(fileName);
    }

    if (affectedFiles.isEmpty())
        return true;

    // A one-record replacement is already protected by QSaveFile. Avoid
    // creating a backup directory for this common operation.
    if (affectedFiles.size() == 1 && filesToWrite.size() == 1 && filesToRemove.isEmpty())
    {
        const QString fileName = filesToWrite.first();
        const QString filePath = directory.absoluteFilePath(fileName);
        QSaveFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
        {
            if (errorString)
                *errorString = QObject::tr("Can't open playlist file for writing: %1 (%2)")
                                   .arg(fileName, file.errorString());
            return false;
        }

        const QByteArray content = newContents.value(fileName);
        if (file.write(content) != content.size() || !file.commit())
        {
            const QString reason = file.errorString();
            file.cancelWriting();
            if (errorString)
                *errorString = QObject::tr("Can't save playlist file: %1 (%2). Check the SD card.")
                                   .arg(fileName, reason);
            return false;
        }
        return true;
    }

    // Removing a single obsolete file is itself the only operation. If it
    // fails, the original file remains available and no rollback is needed.
    if (affectedFiles.size() == 1 && filesToWrite.isEmpty() && filesToRemove.size() == 1)
    {
        const QString filePath = directory.absoluteFilePath(filesToRemove.first());
        if (QFile::remove(filePath))
            return true;
        if (errorString)
            *errorString = QObject::tr("Can't remove playlist file: %1. Check the SD card.")
                               .arg(filesToRemove.first());
        return false;
    }

    // Keep temporary backups outside PLAYLIST. If the process is interrupted,
    // a leftover backup directory must not be discovered as a user playlist.
    QTemporaryDir backupDirectory;
    if (!backupDirectory.isValid())
    {
        if (errorString)
            *errorString = QObject::tr("Can't create playlist backup. Check the SD card and its available space.");
        return false;
    }

    QSet<QString> backedUpFiles;
    for (const QString &fileName : affectedFiles)
    {
        const QString sourcePath = directory.absoluteFilePath(fileName);
        if (!QFileInfo::exists(sourcePath))
            continue;

        const QString backupPath = backupDirectory.filePath(fileName);
        if (!QFile::copy(sourcePath, backupPath))
        {
            if (errorString)
                *errorString = QObject::tr("Can't back up playlist file: %1. No playlist files were changed. Check the SD card.")
                                   .arg(fileName);
            return false;
        }
        backedUpFiles.insert(fileName);
    }

    QString failure;
    for (const QString &fileName : filesToWrite)
    {
        const QString filePath = directory.absoluteFilePath(fileName);
        QSaveFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
        {
            failure = QObject::tr("Can't open playlist file for writing: %1 (%2)")
                          .arg(fileName, file.errorString());
            break;
        }

        const QByteArray content = newContents.value(fileName);
        if (file.write(content) != content.size())
        {
            failure = QObject::tr("Can't write playlist file: %1 (%2)")
                          .arg(fileName, file.errorString());
            file.cancelWriting();
            break;
        }

        if (!file.commit())
        {
            failure = QObject::tr("Can't finalize playlist file: %1 (%2)")
                          .arg(fileName, file.errorString());
            break;
        }
    }

    if (failure.isEmpty())
    {
        for (const QString &fileName : filesToRemove)
        {
            const QString filePath = directory.absoluteFilePath(fileName);
            if (!QFile::remove(filePath))
            {
                failure = QObject::tr("Can't remove obsolete playlist file: %1").arg(fileName);
                break;
            }
        }
    }

    if (failure.isEmpty())
        return true;

    QStringList restorationErrors;
    for (const QString &fileName : affectedFiles)
    {
        const QString destinationPath = directory.absoluteFilePath(fileName);
        if (backedUpFiles.contains(fileName))
        {
            QFile backupFile(backupDirectory.filePath(fileName));
            if (!backupFile.open(QIODevice::ReadOnly))
            {
                restorationErrors.append(QObject::tr("%1: can't read backup").arg(fileName));
                continue;
            }

            QSaveFile restoredFile(destinationPath);
            if (!restoredFile.open(QIODevice::WriteOnly))
            {
                restorationErrors.append(QObject::tr("%1: can't open for restoration").arg(fileName));
                continue;
            }

            const QByteArray backupContent = backupFile.readAll();
            if (backupFile.error() != QFile::NoError
                || restoredFile.write(backupContent) != backupContent.size()
                || !restoredFile.commit())
            {
                restoredFile.cancelWriting();
                restorationErrors.append(QObject::tr("%1: restoration failed").arg(fileName));
            }
        }
        else if (QFileInfo::exists(destinationPath) && !QFile::remove(destinationPath))
        {
            restorationErrors.append(QObject::tr("%1: can't remove newly created file").arg(fileName));
        }
    }

    if (errorString)
    {
        if (restorationErrors.isEmpty())
        {
            *errorString = QObject::tr("Playlist update failed: %1. The previous playlist state was restored. Check the SD card, its connection, and available space.")
                               .arg(failure);
        }
        else
        {
            // Preserve the only remaining copy of the old records if rollback
            // could not complete, and expose its location for manual recovery.
            backupDirectory.setAutoRemove(false);
            *errorString = QObject::tr("Playlist update failed: %1. The previous state could not be fully restored: %2. Backup files were kept at: %3. Stop editing and check the SD card and its connection.")
                               .arg(failure,
                                    restorationErrors.join(QStringLiteral("; ")),
                                    backupDirectory.path());
        }
    }
    return false;
}
