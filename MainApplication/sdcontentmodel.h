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

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

class SdContentModel : public QFileSystemModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SdProxyModel* sdProxyModel READ sdProxyModel CONSTANT)
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex NOTIFY rootIndexChanged FINAL)

    Q_PROPERTY(bool canSetTrack READ canSetTrack WRITE setCanSetTrack NOTIFY canSetTrackChanged FINAL)
    Q_PROPERTY(QString selectedPath READ selectedPath NOTIFY selectedPathChanged FINAL)

    Q_PROPERTY(QString filterString READ filterString WRITE setFilterString NOTIFY filterStringChanged FINAL)
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

    QString selectedPath() const;

    QString filterString() const;
    void setFilterString(const QString &newFilterString);

signals:
    void rootIndexChanged();

    void decodingStarted(quint16 filesLeft);
    void decodingUpdated(quint64 durationProcessed, quint64 fileDuration);
    void decodingFinished();

    void errorOccured(QString errorDescription);

    void canSetTrackChanged();

    void selectedPathChanged();

    void filterStringChanged();

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

    bool m_canSetTrack{false};

    void addWav(QString dstPath, QList<QUrl> filesPathList);
    void addMidi(QString dstPath, QList<QUrl> filesPathList);
    QString m_selectedPath;
    QString m_filterString;
};

#endif // SDCONTENTMODEL_H
