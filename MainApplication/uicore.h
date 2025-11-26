#ifndef UICORE_H
#define UICORE_H

#include <QObject>
#include <QSettings>

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

signals:

    void workspaceChanged();
    void errorOccured(QString errorString);

    void currentPlaylistChanged();

    void currentPlaylistIndexChanged();

private:
    SdContentModel m_sdContentModel{this};
    QDir m_workspaceDir;
    QSettings m_settings;

    QList<PlaylistModel*> m_avaliablePlaylists;
    PlaylistModel* m_currentPlaylist{nullptr};

    void readPlaylistFolder();
    quint16 m_currentPlaylistIndex{0};
};

#endif // UICORE_H
