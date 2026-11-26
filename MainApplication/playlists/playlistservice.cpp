#include "playlistservice.h"
#include "mediapath.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QtAlgorithms>


PlaylistService::PlaylistService(QObject *parent)
    : QObject{parent}
{
}

void PlaylistService::setWorkspacePath(const QString &workspacePath)
{
    m_workspacePath = workspacePath.isEmpty()
                          ? QString()
                          : QDir(workspacePath).absolutePath();
    reload();
}

QString PlaylistService::workspacePath() const
{
    return m_workspacePath;
}

QList<PlaylistModel *> PlaylistService::playlists() const
{
    return m_playlists;
}

QStringList PlaylistService::playlistPaths() const
{
    QStringList paths;
    paths.reserve(m_playlists.size());

    for (const PlaylistModel *playlist : m_playlists)
        paths.append(playlist->playlistPath());

    return paths;
}

PlaylistModel *PlaylistService::playlistAt(int index) const
{
    if (index < 0 || index >= m_playlists.size())
        return nullptr;

    return m_playlists.at(index);
}

int PlaylistService::indexOfPath(const QString &relativePath) const
{
    for (int i = 0; i < m_playlists.size(); ++i)
    {
        if (m_playlists.at(i)->playlistPath() == relativePath)
            return i;
    }

    return -1;
}

bool PlaylistService::createPlaylist(const QString &relativePath, QString *errorString)
{
    if (m_workspacePath.isEmpty())
    {
        if (errorString)
            *errorString = QCoreApplication::translate("UiCore", "Can't create playlist");
        return false;
    }

    if (!m_repository.create(m_workspacePath, relativePath))
    {
        const QString targetPath =
            QDir(m_workspacePath).filePath(QStringLiteral("PLAYLIST/") + relativePath);

        if (errorString)
        {
            *errorString = QFileInfo::exists(targetPath)
                               ? QCoreApplication::translate("UiCore", "Playlist already exist")
                               : QCoreApplication::translate("UiCore", "Can't create playlist");
        }
        return false;
    }

    reload();
    return true;
}

bool PlaylistService::removePlaylist(const QString &relativePath, QString *errorString)
{
    if (m_workspacePath.isEmpty())
    {
        if (errorString)
            *errorString = QCoreApplication::translate(
                "UiCore", "Refusing to delete a folder outside of PLAYLIST");
        return false;
    }

    if (!m_repository.remove(m_workspacePath, relativePath))
    {
        if (errorString)
            *errorString = QCoreApplication::translate("UiCore", "Refusing to delete a folder outside of PLAYLIST");
        return false;
    }

    reload();
    return true;
}


QStringList PlaylistService::checkPlaylist(const QString &relativePath) const
{
    PlaylistData playlist;
    if (m_workspacePath.isEmpty()
        || !m_repository.loadPlaylist(m_workspacePath, relativePath, &playlist))
    {
        return {QCoreApplication::translate("UiCore", "Playlist not found: %1")
                    .arg(relativePath)};
    }

    const QRegularExpression englishRegex(QStringLiteral("^[a-zA-Z0-9\\\\:/._\\-\\s()&]+$"));
    QStringList result;
    QStringList errors;
    result.append(playlist.relativePath + QStringLiteral(":"));

    for (const QString &loadError : playlist.loadErrors)
        errors.append(QStringLiteral("\t") + QCoreApplication::translate(
            "UiCore", "Cannot load .ego record: ") + loadError);

    for (int i = 0; i < playlist.songs.size(); ++i)
    {
        const Song &song = playlist.songs.at(i);
        const QString songNumber = QString::number(i + 1);

        if (song.t1Path.isEmpty())
        {
            errors.append(QStringLiteral("\t")
                          + QCoreApplication::translate("UiCore", "Song ")
                          + songNumber
                          + QCoreApplication::translate("UiCore", " track 1 not settled"));
        }
        else
        {
            const QString track1Path = MediaPath::workspaceFilePath(m_workspacePath, song.t1Path);
            const QFileInfo track1Info(track1Path);
            if (!track1Info.exists() || !track1Info.isFile())
            {
                errors.append(QStringLiteral("\t")
                              + QCoreApplication::translate("UiCore", "Song ")
                              + songNumber
                              + QCoreApplication::translate("UiCore", " track 1 not found: ")
                              + song.t1Path);
            }

            if (!englishRegex.match(track1Path).hasMatch())
            {
                errors.append(QStringLiteral("\t")
                              + QCoreApplication::translate("UiCore", "Song ")
                              + songNumber
                              + QCoreApplication::translate(
                                  "UiCore", " track1 contains non latin symbols."));
            }
        }

        if (!song.t2Path.isEmpty())
        {
            const QString track2Path = MediaPath::workspaceFilePath(m_workspacePath, song.t2Path);
            const QFileInfo track2Info(track2Path);
            if (!track2Info.exists() || !track2Info.isFile())
            {
                errors.append(QStringLiteral("\t")
                              + QCoreApplication::translate("UiCore", "Song ")
                              + songNumber
                              + QCoreApplication::translate("UiCore", " track 2 not found: ")
                              + song.t2Path);
            }

            if (!englishRegex.match(track2Path).hasMatch())
            {
                errors.append(QStringLiteral("\t")
                              + QCoreApplication::translate("UiCore", "Song ")
                              + songNumber
                              + QCoreApplication::translate(
                                  "UiCore", " track2 contains non latin symbols."));
            }
        }
    }

    if (errors.isEmpty())
        result.append(QStringLiteral("\t")
                      + QCoreApplication::translate("UiCore", "Playlist is OK"));
    else
        result.append(errors);

    result.append(QString());
    return result;
}

void PlaylistService::reload()
{
    clearModels();

    if (m_workspacePath.isEmpty())
        return;

    const QList<PlaylistData> playlistData = m_repository.load(m_workspacePath);
    m_playlists.reserve(playlistData.size());
    QStringList loadIssues;

    for (const PlaylistData &playlist : playlistData)
    {
        for (const QString &loadError : playlist.loadErrors)
            loadIssues.append(playlist.relativePath + QStringLiteral(": ") + loadError);

        auto *model = new PlaylistModel(playlist.songs, this);
        model->setPlaylistName(playlist.name);
        model->setPlaylistPath(playlist.relativePath);
        model->setWorkspacePath(m_workspacePath);
        model->setPersistenceEnabled(playlist.loadErrors.isEmpty());
        model->setDir(QDir(playlist.directoryPath));

        connect(model, &PlaylistModel::errorOccured,
                this, &PlaylistService::errorOccurred);

        m_playlists.append(model);
    }

    if (!loadIssues.isEmpty())
        emit loadIssuesFound(loadIssues);
}

void PlaylistService::clearModels()
{
    qDeleteAll(m_playlists);
    m_playlists.clear();
}

