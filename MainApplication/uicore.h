#ifndef UICORE_H
#define UICORE_H

#include <QSettings>
#include <QTranslator>

#include "sdcontentmodel.h"
#include "playlistmodel.h"

class UiCore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(SdContentModel* sdContentModel READ sdContentModel CONSTANT)
    Q_PROPERTY(PlaylistModel* currentPlaylist READ currentPlaylist NOTIFY currentPlaylistChanged FINAL)
    Q_PROPERTY(QStringList avaliablePlaylists READ avaliablePlaylists NOTIFY workspaceChanged FINAL)
    Q_PROPERTY(QString workspacePath READ workspacePath NOTIFY workspaceChanged FINAL)
    Q_PROPERTY(qint16 currentPlaylistIndex READ currentPlaylistIndex WRITE setCurrentPlaylistIndex NOTIFY currentPlaylistIndexChanged FINAL)

    Q_PROPERTY(QString appLanguageCode READ appLanguageCode NOTIFY translatorChanged)
public:
    explicit UiCore(QObject *parent = nullptr);

    SdContentModel* sdContentModel() {return &m_sdContentModel;};

    Q_INVOKABLE void setWorkspace(QUrl workspaceFolderPath);

    Q_INVOKABLE void addPlaylist(QString plsName);
    Q_INVOKABLE void deletePlaylist(quint16 index);

    QString workspacePath() const;
    QStringList avaliablePlaylists() const;
    PlaylistModel* currentPlaylist() const;

    quint16 currentPlaylistIndex() const;
    void setCurrentPlaylistIndex(qint16 newCurrentPlylistInex);

    Q_INVOKABLE void openManualExternally(QString fileName);
    Q_INVOKABLE void runWavConvertor();

    Q_INVOKABLE void checkPlaylists();

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
    QDir m_workspaceDir;
    QSettings m_settings;

    QTranslator m_translator;
    QMap<QString, QString> pathFromCode
    {
        {"en", ":/translations/EgoGIG_en.qm"},
        {"ru", ":/translations/EgoGIG_ru.qm"},
    };

    QList<PlaylistModel*> m_avaliablePlaylists;
    PlaylistModel* m_currentPlaylist{nullptr};

    void readPlaylistFolder();
    quint16 m_currentPlaylistIndex{0};

    void loadTranslator(QString languageCode);
    void loadDefaultTranslator();
};

#endif // UICORE_H
