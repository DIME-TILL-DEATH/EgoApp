#ifndef UICORE_H
#define UICORE_H

#include <QSettings>
#include <QTranslator>

#include "sdcontentmodel.h"
#include "playlists/playlistservice.h"

class UiCore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(SdContentModel* sdContentModel READ sdContentModel CONSTANT)
    Q_PROPERTY(PlaylistModel* currentPlaylist READ currentPlaylist NOTIFY currentPlaylistChanged FINAL)
    Q_PROPERTY(QStringList avaliablePlaylists READ avaliablePlaylists NOTIFY workspaceChanged FINAL)
    Q_PROPERTY(QString workspacePath READ workspacePath NOTIFY workspaceChanged FINAL)
    Q_PROPERTY(int currentPlaylistIndex READ currentPlaylistIndex WRITE setCurrentPlaylistIndex NOTIFY currentPlaylistIndexChanged FINAL)

    Q_PROPERTY(QString appLanguageCode READ appLanguageCode NOTIFY translatorChanged)
public:
    explicit UiCore(QObject *parent = nullptr);

    SdContentModel* sdContentModel() {return &m_sdContentModel;};

    Q_INVOKABLE void setWorkspace(QUrl workspaceFolderPath);

    // Accepts a slash-separated path relative to PLAYLIST, e.g. "Collections/Rock".
    Q_INVOKABLE void addPlaylist(QString playlistPath);
    Q_INVOKABLE void deletePlaylist(quint16 index);

    QString workspacePath() const;
    QStringList avaliablePlaylists() const;
    PlaylistModel* currentPlaylist() const;

    int currentPlaylistIndex() const;
    void setCurrentPlaylistIndex(int newCurrentPlylistInex);

    Q_INVOKABLE void openManualExternally(QString fileName);
    Q_INVOKABLE void runWavConvertor();

    Q_INVOKABLE void checkPlaylist(const QString &playlistPath);

    Q_INVOKABLE void setLanguage(QString languageCode);
    QString appLanguageCode();

signals:
    void workspaceChanged();
    void errorOccured(QString errorString);

    void currentPlaylistChanged();
    void currentPlaylistIndexChanged();

    void checkingPlaylistsFinished(QStringList result);

    void translatorChanged(QString langaugeCode);

private:
    SdContentModel m_sdContentModel{this};
    QSettings m_settings;
    PlaylistService m_playlistService;

    QTranslator m_translator;
    QMap<QString, QString> pathFromCode
    {
        {"en", ":/translations/EgoGIG_en.qm"},
        {"ru", ":/translations/EgoGIG_ru.qm"},
    };

    PlaylistModel* m_currentPlaylist{nullptr};

    int m_currentPlaylistIndex{0};

    void loadTranslator(QString languageCode);
    void loadDefaultTranslator();
};

#endif // UICORE_H
