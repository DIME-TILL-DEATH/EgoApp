#ifndef UICORE_H
#define UICORE_H

#include <QObject>
#include <QSettings>

#include "sdcontentmodel.h"

class UiCore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(SdContentModel* sdContentModel READ sdContentModel CONSTANT)
    Q_PROPERTY(QStringList avaliablePlaylists READ avaliablePlaylists NOTIFY workspaceChanged FINAL)
    Q_PROPERTY(QString workspacePath READ workspacePath NOTIFY workspaceChanged FINAL)
public:
    explicit UiCore(QObject *parent = nullptr);

    SdContentModel* sdContentModel() {return &m_sdContentModel;};

    Q_INVOKABLE void setWorkspace(QUrl workspaceFolderPath);

    QString workspacePath() const;
    QStringList avaliablePlaylists() const;
signals:

    void workspaceChanged();
    void errorOccured(QString errorString);

private:
    SdContentModel m_sdContentModel{this};
    QDir m_workspaceDir;
    QSettings m_settings;
};

#endif // UICORE_H
