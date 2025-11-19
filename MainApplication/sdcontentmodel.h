#ifndef SDCONTENTMODEL_H
#define SDCONTENTMODEL_H

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QObject>
#include <QQmlEngine>
#include <QAudioDecoder>

class SdProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    SdProxyModel(QObject* parent) : QSortFilterProxyModel{parent} {};

private:

    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);

        QFileSystemModel* srcModel = qobject_cast<QFileSystemModel*>(sourceModel());
        QModelIndex rootIndex = srcModel->index(srcModel->rootPath());

        if(rootIndex == sourceParent)
        {
            return sourceModel()->data(index0).toString().contains(filterRegularExpression());
        }
        else
        {
            return true;
        }

    };
};

class SdContentModel : public QFileSystemModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SdProxyModel* sdProxyModel READ sdProxyModel CONSTANT)
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex NOTIFY rootIndexChanged FINAL)

    Q_PROPERTY(bool canSetTrack READ canSetTrack WRITE setCanSetTrack NOTIFY canSetTrackChanged FINAL)
public:
    explicit SdContentModel(QObject *parent = nullptr);

    SdProxyModel* sdProxyModel() {return &m_sdProxyModel;};
    QModelIndex rootIndex() const;

    void setWorkspace(const QDir& workspaceDir);

    Q_INVOKABLE void currentSelectionChanged(QModelIndex currentIndex);

    Q_INVOKABLE void addContent(QModelIndex parentIndex, QList<QUrl> filesPathList);
    Q_INVOKABLE void addFolder(QModelIndex parentIndex, QString name);
    Q_INVOKABLE void deleteObject(QModelIndex index);
    bool canSetTrack() const;
    void setCanSetTrack(bool newCanSetTrack);

signals:
    void rootIndexChanged();

    void decodingStarted(quint16 filesLeft);
    void decodingUpdated(quint64 durationProcessed, quint64 fileDuration);
    void decodingFinished();

    void errorOccured(QString errorDescription);

    void canSetTrackChanged();

public slots:
    void slBufferReady();
    void slDecodingChanged(bool isDecoding);
    void slDecodingError(QAudioDecoder::Error error);

private:
    SdProxyModel m_sdProxyModel{this};
    QAudioDecoder decoder{this};

    void startDecoding();

    QList<QUrl> m_wavsListToUpload;
    QString dstWavFolderPath;
    QFile dstWavFile;

    void addWav(QString dstPath, QList<QUrl> filesPathList);
    void addMidi(QString dstPath, QList<QUrl> filesPathList);
    bool m_canSetTrack{false};
};

#endif // SDCONTENTMODEL_H
