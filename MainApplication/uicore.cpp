#include "uicore.h"

#include <QResource>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QFileInfo>

#include <QProcess>
#include <QCoreApplication>
#include <QUrl>

#include <QCollator>

UiCore::UiCore(QObject *parent)
    : QObject{parent}
{
    QString workspacePath = m_settings.value("workspace_path").toString();

    if(workspacePath.isEmpty())
    {
#ifdef Q_OS_MACOS
        workspacePath = QDir::homePath() + "/Documents";
#else
        workspacePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif
        workspacePath += "/AMT/EgoGIG/Default_workspace/";

        QDir dir(workspacePath);
        dir.mkpath(workspacePath);
    }

    m_workspaceDir = QDir(workspacePath);
    m_sdContentModel.setWorkspace(workspacePath);

    connect(&m_sdContentModel, &SdContentModel::errorOccured, this, &UiCore::errorOccured);

    readPlaylistFolder();
    setCurrentPlaylistIndex(0);

    QString appLanguage = m_settings.value("application_language", "autoselect").toString();

    if(appLanguage=="autoselect")
    {
        appLanguage = QLocale().name().left(2);
    }
    setLanguage(appLanguage);
}

void UiCore::setWorkspace(QUrl workspaceFolderPath)
{
    QString path = workspaceFolderPath.path();
#ifdef Q_OS_WINDOWS
    path = path.removeFirst();
#endif
    m_workspaceDir = QDir(path);
    m_sdContentModel.setWorkspace(path);

    m_settings.setValue("workspace_path", path);

    readPlaylistFolder();
    setCurrentPlaylistIndex(0);

    emit workspaceChanged();
}

void UiCore::addPlaylist(QString plsName)
{
    QFileInfo dirInfo(m_workspaceDir.absolutePath() + "/PLAYLIST/" + plsName);
    if(dirInfo.exists())
    {
        emit errorOccured(QObject::tr("Playlist already exist"));
        return;
    }

    m_workspaceDir.mkpath("PLAYLIST/" + plsName);
    readPlaylistFolder();

    for(int i=0; i < m_avaliablePlaylists.size(); i++)
    {
        if(m_avaliablePlaylists.at(i)->playlistName() == plsName)
        {
            emit workspaceChanged();
            setCurrentPlaylistIndex(i);
        }
    }
}

void UiCore::deletePlaylist(quint16 index)
{
    QDir playlistDir = m_workspaceDir;
    if(playlistDir.cd("PLAYLIST"))
    {
        if(index < m_avaliablePlaylists.size())
        {
            QDir plsDir(playlistDir.absolutePath() + "/" + m_avaliablePlaylists.at(index)->playlistName());
            plsDir.removeRecursively();
            readPlaylistFolder();
            emit workspaceChanged();

            setCurrentPlaylistIndex(0);
        }
    }
}

QString UiCore::workspacePath() const
{
    return m_workspaceDir.absolutePath();
}

QStringList UiCore::avaliablePlaylists() const
{
    QStringList playlists;

    foreach (PlaylistModel* plsModel, m_avaliablePlaylists)
    {
        playlists.append(plsModel->playlistName());
    }

    return playlists;
}

PlaylistModel* UiCore::currentPlaylist() const
{
    return m_currentPlaylist;
}

void UiCore::readPlaylistFolder()
{
    qDeleteAll(m_avaliablePlaylists);
    m_avaliablePlaylists.clear();
    m_currentPlaylist = nullptr;

    QDir roorPlstDir = m_workspaceDir;
    if(roorPlstDir.cd("PLAYLIST"))
    {
        QStringList allDirs = roorPlstDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

        foreach(QString plsDirName, allDirs)
        {
            QDir plsDir(roorPlstDir.absolutePath() + "/" + plsDirName);

            QStringList filters;
            filters.append("*.ego");
            QStringList egoFiles = plsDir.entryList(filters, QDir::Files, QDir::Name);
            QCollator collator;
            collator.setNumericMode(true);
            std::sort(egoFiles.begin(), egoFiles.end(), collator);

            QList<Song> songList;
            if(!egoFiles.isEmpty())
            {
                foreach (QString egoFilePath, egoFiles)
                {
                    Song song;

                    QFile file(plsDir.absoluteFilePath(egoFilePath));
                    if(file.open(QIODevice::ReadOnly))
                    {
                        QByteArray track1Line = file.readLine();
                        if(track1Line.size() != 0)
                        {
                            if(track1Line.front() == '>')
                            {
                                song.playNext = true;
                                track1Line = track1Line.removeFirst();
                            }
                            song.t1Path = track1Line;
                            song.t1Path.remove("\n");
                        }
                        song.t2Path = file.readLine();
                        song.t2Path.remove("\n");
                    }

                    QFileInfo fileInfo(file);
                    QString fileName = fileInfo.fileName();
                    fileName.remove(".ego");
                    song.fileNum = fileName.toInt();

                    songList.append(song);
                }

            }
            PlaylistModel* playListModel = new PlaylistModel(songList, this);
            playListModel->setPlaylistName(plsDirName);

            QDir playlistDir(roorPlstDir);
            playlistDir.cd(plsDirName);

            playListModel->setDir(playlistDir);
            m_avaliablePlaylists.append(playListModel);
        }
    }
}


quint16 UiCore::currentPlaylistIndex() const
{
    return m_currentPlaylistIndex;
}

void UiCore::setCurrentPlaylistIndex(qint16 newCurrentPlaylistIndex)
{
    m_currentPlaylistIndex = newCurrentPlaylistIndex;

    if(m_avaliablePlaylists.size() == 0)
    {
        m_currentPlaylist = nullptr;
        m_currentPlaylistIndex = -1;
    }
    else
    {
        if(m_currentPlaylistIndex <  m_avaliablePlaylists.size())
            m_currentPlaylist = m_avaliablePlaylists.at(m_currentPlaylistIndex);
    }

    emit currentPlaylistChanged();
    emit currentPlaylistIndexChanged();
}

void UiCore::checkPlaylists()
{
    QStringList result;

    QRegularExpression englishRegex("^[a-zA-Z0-9\\\\:/._\\-\\s()&]+$");


    foreach(PlaylistModel* playlist, m_avaliablePlaylists)
    {
        quint32 songsCount = playlist->rowCount(QModelIndex());

        result.append(playlist->playlistName() + ":");
        QStringList errors;
        for(int i=0; i < songsCount; i++)
        {
            QModelIndex index0 = playlist->index(i, 0, QModelIndex());

            QString t1Path = m_workspaceDir.absolutePath() + playlist->data(index0, PlaylistModel::T1PathRole).toString();
            QString t2Path = m_workspaceDir.absolutePath() + playlist->data(index0, PlaylistModel::T2PathRole).toString();

            QFileInfo t1PathInfo(t1Path);
            QFileInfo t2PathInfo(t2Path);


            QString errString;
            if(!t1PathInfo.exists())
            {
                errString.append("\t");
                errString.append(QObject::tr("Song ") + QString::number(i+1));

                if(m_workspaceDir.absolutePath() + playlist->data(index0, PlaylistModel::T1PathRole).toString() == "")
                    errString.append(QObject::tr(" track 1 not settled"));
                else
                    errString.append(QObject::tr(" track 1 not found: ") + playlist->data(index0, PlaylistModel::T1PathRole).toString());
            }

            if(!errString.isEmpty()) errors.append(errString);

            if (!englishRegex.match(t1Path).hasMatch()) errors.append("\t"
                              + QObject::tr("Song ") + QString::number(i+1) +QObject::tr(" track1 contains non latin symbols."));

            errString.clear();

            if(!t2PathInfo.exists() && m_workspaceDir.absolutePath() + playlist->data(index0, PlaylistModel::T2PathRole).toString() != "")
            {
                errString.append("\t");
                errString.append(QObject::tr("Song ") + QString::number(i+1) + QObject::tr(" track 2 not found: ") + playlist->data(index0, PlaylistModel::T2PathRole).toString());
            }

            if (!englishRegex.match(t2Path).hasMatch()) errors.append("\t"
                              + QObject::tr("Song ") + QString::number(i+1) +QObject::tr(" track2 contains non latin symbols."));

            if(!errString.isEmpty()) errors.append(errString);

        }

        if(errors.isEmpty()) result.append("\t" + QObject::tr("Playlist is OK"));
        else result.append(errors);

        result.append("\n");
    }

    emit checkingPlaylistsFinished(result);
}

void UiCore::setLanguage(QString languageCode)
{
    m_settings.setValue("application_language", languageCode);
    m_settings.sync();

    loadTranslator(languageCode);
}

QString UiCore::appLanguageCode()
{
    return m_settings.value("application_language", "en").toString();
}

void UiCore::loadTranslator(QString languageCode)
{
    QCoreApplication::removeTranslator(&m_translator);

    if(languageCode=="autoselect")
    {
        loadDefaultTranslator();
        return;
    }

    if (m_translator.load(pathFromCode.value(languageCode)))
    {
        QCoreApplication::installTranslator(&m_translator);

        emit translatorChanged(languageCode);
    }

    QQmlEngine* engine = qmlEngine(this);
    if(engine)
    {
        engine->retranslate();
    }
}

void UiCore::loadDefaultTranslator()
{
    if (m_translator.load(QLocale(), QLatin1String("EgoGIG"), QLatin1String("_"), ":/translations/"))
    {
        QCoreApplication::installTranslator(&m_translator);
        emit translatorChanged(QLocale().nativeLanguageName());
    }
}

void UiCore::openManualExternally(QString fileName)
{
    QString appLanguage = m_settings.value("application_language", "autoselect").toString();

    if(appLanguage=="autoselect")
    {
        appLanguage = QLocale().name().left(2);

        if(appLanguage != "ru") appLanguage = "en";
    }

    QString fullFileName =  fileName + "_" + appLanguage + ".pdf";

#ifdef Q_OS_ANDROID
    QString filePath = ":/docs/" + fullFileName;
    QFile pdfFile(filePath);

    if(!pdfFile.exists())
    {
        fullFileName = fileName + ".pdf";
        filePath = ":/docs/" + fullFileName;
        pdfFile.setFileName(filePath);
    }

    QString temporallyPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/" + fullFileName;
    pdfFile.copy(temporallyPath);

    QJniObject::callStaticMethod<void>(
        "com.amtelectronics.utils/JavaFile", "openFileExternally",
        "(Ljava/lang/String;Landroid/content/Context;)V",
        QJniObject::fromString(fullFileName).object<jstring>(),
        QNativeInterface::QAndroidApplication::context());
#elif defined(Q_OS_IOS)
    QString filePath = ":/docs/" + fullFileName;
    QFile pdfFile(filePath);

    if(!pdfFile.exists())
    {
        fullFileName = fileName + ".pdf";
        filePath = ":/docs/" + fullFileName;
        pdfFile.setFileName(filePath);
    }

    QString temporallyPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)+"/" + fullFileName;
    pdfFile.copy(temporallyPath);

    QDesktopServices::openUrl(QUrl::fromLocalFile(temporallyPath));
#elif defined(Q_OS_LINUX)
    QString filePath = QCoreApplication::applicationDirPath() + "/../docs/" + fullFileName;

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("LD_LIBRARY_PATH"); // force evince use system librarys

    proc.setProcessEnvironment(env);
    proc.setProgram("evince");
    proc.setArguments(QStringList(filePath));
    proc.startDetached();
#elif defined(Q_OS_MACOS)
    QString filePath =  QCoreApplication::applicationDirPath() + "/../Resources/docs/" + fullFileName;
    qDebug() << filePath;
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
#else
    QString filePath =  QCoreApplication::applicationDirPath() + "/docs/" + fullFileName;
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
#endif
}

void UiCore::runWavConvertor()
{
#ifdef Q_OS_WIN
    QProcess wavConvertorProcess;
    wavConvertorProcess.setWorkingDirectory(QCoreApplication::applicationDirPath());
    wavConvertorProcess.setProgram("WavConverter.exe");
    wavConvertorProcess.startDetached();
#endif

#ifdef Q_OS_MACOS
    QProcess wavConvertorProcess;
    wavConvertorProcess.setProgram(QCoreApplication::applicationDirPath() + "/wavconverter.app");
    wavConvertorProcess.startDetached();
#endif

#ifdef Q_OS_LINUX
    QProcess irConvertorProcess;
    QString path = QCoreApplication::applicationDirPath() + "/IrConverter";
#endif
}
