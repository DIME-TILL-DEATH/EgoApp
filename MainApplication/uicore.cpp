#include "uicore.h"

#include <QResource>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QFileInfo>

#include <QProcess>
#include <QCoreApplication>
#include <QUrl>

#include <QCollator>

// Рабочая папка не должна быть корнем, домашней папкой или системной/библиотечной
// директорией: приложение создаёт там SONGS/PLAYLIST и умеет удалять их содержимое.
static bool isSafeWorkspace(const QString &path)
{
    const QString p = QDir::cleanPath(path);
    if(p.isEmpty() || p == "/" || p == ".") return false;

    const QString home = QDir::cleanPath(QDir::homePath());
    if(p == home) return false;

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    static const QStringList forbidden = {"/System", "/Library", "/Applications", "/usr",
                                          "/bin", "/sbin", "/etc", "/var", "/private", "/opt"};
    for(const QString &f : forbidden)
        if(p == f || p.startsWith(f + "/")) return false;

    if(p == home + "/Library" || p.startsWith(home + "/Library/")) return false;
#endif
    return true;
}

UiCore::UiCore(QObject *parent)
    : QObject{parent}
{
    connect(&m_sdContentModel, &SdContentModel::errorOccured, this, &UiCore::errorOccured);
    connect(&m_playlistService, &PlaylistService::errorOccurred, this, &UiCore::errorOccured);
    connect(&m_playlistService, &PlaylistService::loadIssuesFound, this, &UiCore::playlistLoadIssuesFound);
}

void UiCore::initialize()
{
    if (m_initialized)
        return;

    m_initialized = true;

    QString workspacePath = m_settings.value("workspace_path").toString();

    if (!workspacePath.isEmpty() && !isSafeWorkspace(workspacePath))
        workspacePath.clear(); // Unsafe path from old settings -> default workspace.

    if (workspacePath.isEmpty())
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

    const QDir workspaceDir(workspacePath);
    m_sdContentModel.setWorkspace(workspaceDir);
    m_playlistService.setWorkspacePath(workspaceDir.absolutePath());
    setCurrentPlaylistIndex(0);

    QString appLanguage = m_settings.value("application_language", "autoselect").toString();
    if (appLanguage == "autoselect")
        appLanguage = QLocale().name().left(2);

    setLanguage(appLanguage);

    emit workspaceChanged();
}

void UiCore::setWorkspace(QUrl workspaceFolderPath)
{
    QString path = workspaceFolderPath.path();
#ifdef Q_OS_WINDOWS
    path = path.removeFirst();
#endif
    if(!isSafeWorkspace(path))
    {
        emit errorOccured(QObject::tr("This folder can't be used as a workspace"));
        return;
    }

    const QDir workspaceDir(path);
    m_sdContentModel.setWorkspace(workspaceDir);
    m_settings.setValue("workspace_path", path);
    m_playlistService.setWorkspacePath(workspaceDir.absolutePath());

    setCurrentPlaylistIndex(0);

    emit workspaceChanged();
}

void UiCore::addPlaylist(QString playlistPath)
{
    QString errorString;
    if (!m_playlistService.createPlaylist(playlistPath, &errorString))
    {
        emit errorOccured(errorString);
        return;
    }

    emit workspaceChanged();

    const int playlistIndex = m_playlistService.indexOfPath(playlistPath);
    if (playlistIndex >= 0)
        setCurrentPlaylistIndex(playlistIndex);
}

void UiCore::deletePlaylist(quint16 index)
{
    PlaylistModel *playlist = m_playlistService.playlistAt(index);
    if (!playlist)
        return;

    const QString playlistPath = playlist->playlistPath();
    QString errorString;
    if (!m_playlistService.removePlaylist(playlistPath, &errorString))
    {
        emit errorOccured(errorString);
        return;
    }

    emit workspaceChanged();

    setCurrentPlaylistIndex(0);
}

QString UiCore::workspacePath() const
{
    return m_playlistService.workspacePath();
}

QStringList UiCore::avaliablePlaylists() const
{
    return m_playlistService.playlistPaths();
}

PlaylistModel* UiCore::currentPlaylist() const
{
    return m_currentPlaylist;
}

int UiCore::currentPlaylistIndex() const
{
    return m_currentPlaylistIndex;
}

void UiCore::setCurrentPlaylistIndex(int newCurrentPlaylistIndex)
{
    const int playlistCount = m_playlistService.playlists().size();
    if (playlistCount == 0)
    {
        m_currentPlaylist = nullptr;
        m_currentPlaylistIndex = -1;
    }
    else
    {
        m_currentPlaylistIndex = qBound(0, newCurrentPlaylistIndex, playlistCount - 1);
        m_currentPlaylist = m_playlistService.playlistAt(m_currentPlaylistIndex);
    }

    emit currentPlaylistChanged();
    emit currentPlaylistIndexChanged();
}

void UiCore::checkPlaylist(const QString &playlistPath)
{
    emit checkingPlaylistsFinished(m_playlistService.checkPlaylist(playlistPath));
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
    // Second executable inside EgoGIG.app/Contents/MacOS (shares the app's Qt).
    const QString converterPath = QCoreApplication::applicationDirPath() + "/wavconverter";
    if(!QFile::exists(converterPath))
    {
        qWarning() << "WavConverter executable not found:" << converterPath;
        return;
    }

    QProcess wavConvertorProcess;
    wavConvertorProcess.setProgram(converterPath);
    wavConvertorProcess.startDetached();
#endif

#ifdef Q_OS_LINUX
    QProcess irConvertorProcess;
    QString path = QCoreApplication::applicationDirPath() + "/IrConverter";
#endif
}
