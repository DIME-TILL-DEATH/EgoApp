#include "sdcontentmodel.h"

#include <QAudioDecoder>
#include <QThread>

SdContentModel::SdContentModel(QObject *parent)
    : QFileSystemModel{parent}
{
    setRootPath("C:/");

    QStringList filter;
    filter.append("*.wav");
    filter.append("*.mid");
    setNameFilters(filter);
    setNameFilterDisables(false);
    setReadOnly(false);

    m_sdProxyModel.setSourceModel(this);
    m_sdProxyModel.setFilterRegularExpression("SONGS");

    QAudioFormat format;
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(44100);

    decoder.setAudioFormat(format);
    connect(&decoder, &QAudioDecoder::isDecodingChanged, this, &SdContentModel::slDecodingChanged);
    connect(&decoder, &QAudioDecoder::bufferReady, this, &SdContentModel::slBufferReady);
    connect(&decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this, &SdContentModel::slDecodingError);
}

QModelIndex SdContentModel::rootIndex() const
{
    return m_sdProxyModel.mapFromSource(index(rootPath()));
}

void SdContentModel::setWorkspace(const QDir &workspaceDir)
{
    setRootPath(workspaceDir.absolutePath());
    emit rootIndexChanged();
}

void SdContentModel::addContent(QModelIndex parentIndex, QList<QUrl> filesPathList)
{
    QModelIndex realIndex = m_sdProxyModel.mapToSource(parentIndex);
    if(realIndex.row() == -1) realIndex = index(rootPath() + "/SONGS");

    QString dstPath = this->filePath(realIndex);

    QFileInfo info(dstPath);

    if(info.isFile()) dstPath = info.absoluteDir().absolutePath();

    QList<QUrl> sortedMidiList;
    QList<QUrl> sortedAudioList;
    foreach(QUrl fileUrl, filesPathList)
    {
        QString filePath = fileUrl.toLocalFile();
        QFileInfo fileInfo(filePath);

        if(fileInfo.suffix().compare("mid", Qt::CaseSensitivity::CaseInsensitive) == 0)
        {
            sortedMidiList.append(fileUrl);
        }

        if(fileInfo.suffix().compare("wav", Qt::CaseSensitivity::CaseInsensitive) == 0
            || fileInfo.suffix().compare("mp3", Qt::CaseSensitivity::CaseInsensitive) == 0)
        {
            sortedAudioList.append(fileUrl);
        }
    }

    addMidi(dstPath, sortedMidiList);
    addWav(dstPath, sortedAudioList);
}

void SdContentModel::addFolder(QModelIndex parentIndex, QString name)
{
    QModelIndex realIndex = m_sdProxyModel.mapToSource(parentIndex);

    QDir rootDir(rootPath());

    if(!rootDir.exists("SONGS"))
    {
        realIndex = mkdir(index(rootPath()), "SONGS");
    }

    mkdir(realIndex, name);
}

void SdContentModel::addWav(QString dstPath, QList<QUrl> filesPathList)
{
    m_wavsListToUpload = filesPathList;
    dstWavFolderPath = dstPath + "/";

    startDecoding();
}

void SdContentModel::startDecoding()
{
    if(m_wavsListToUpload.size() > 0)
    {
        emit decodingStarted(m_wavsListToUpload.count());

        QUrl wavUrl = m_wavsListToUpload.first();
        m_wavsListToUpload.pop_front();

        QString filePath = wavUrl.toLocalFile();
        QFileInfo fileInfo(filePath);

        dstWavFile.setFileName(dstWavFolderPath + fileInfo.fileName());

        if(dstWavFile.open(QIODevice::WriteOnly)) dstWavFile.close();

        decoder.setSource(wavUrl);
        decoder.start();
    }
    else
    {
        dstWavFile.setFileName("");
        emit decodingFinished();
    }
}

void SdContentModel::slBufferReady()
{
    QAudioBuffer buffer = decoder.read();
    emit decodingUpdated(buffer.duration()/1000, decoder.duration()); // buffer duration in us!

    if(dstWavFile.open(QIODeviceBase::Append))
    {
        dstWavFile.write(buffer.constData<char>(), buffer.byteCount());
        dstWavFile.close();
    }
    else
    {
        qWarning() << __FUNCTION__ << "Can't open file: " << dstWavFile.fileName();
    }
}

void SdContentModel::slDecodingChanged(bool isDecoding)
{
    if(!isDecoding)
    {
        if(dstWavFile.exists())
        {
            quint64 finalFileSize = dstWavFile.size();

            QString tmpFilePath = dstWavFile.fileName() + "_tmp";
            dstWavFile.copy(tmpFilePath);
            QFile tmpFile(tmpFilePath);

            if(dstWavFile.open(QIODeviceBase::WriteOnly) && tmpFile.open(QIODeviceBase::ReadOnly))
            {
                QByteArray baFileSize;
                baFileSize.append((finalFileSize&0xff));
                baFileSize.append((finalFileSize&0xff00)>>8);
                baFileSize.append((finalFileSize&0xff0000)>>16);
                baFileSize.append((finalFileSize&0xff000000)>>24);

                quint64 chunkSize = finalFileSize + 44 - 8;

                QByteArray baChunkSize;
                baChunkSize.append((chunkSize&0xff));
                baChunkSize.append((chunkSize&0xff00)>>8);
                baChunkSize.append((chunkSize&0xff0000)>>16);
                baChunkSize.append((chunkSize&0xff000000)>>24);

                QByteArray fileData;

                fileData.append(QString("RIFF").toUtf8());                        //chunkId
                fileData.append(baChunkSize);                                     //chunkSize
                fileData.append(QString("WAVE").toUtf8());                        //format
                fileData.append(QString("fmt ").toUtf8());                        //subchunk1Id
                fileData.append(QByteArray::fromRawData("\x10\x00\x00\x00", 4));  //subchunk1Size
                fileData.append(QByteArray::fromRawData("\x01\x00", 2));          //audioFormat
                fileData.append(QByteArray::fromRawData("\x02\x00", 2));          //numChannels
                fileData.append(QByteArray::fromRawData("\x44\xAC\x00\x00", 4));  //sampleRate
                fileData.append(QByteArray::fromRawData("\x10\xB1\x02\x00", 4));  //byteRate
                fileData.append(QByteArray::fromRawData("\x04\x00", 2));          //blockAlign
                fileData.append(QByteArray::fromRawData("\x10\x00", 2));          //bitsPerSample
                fileData.append(QString("data").toUtf8());                        //subchunk2Id
                fileData.append(baFileSize);    //subchunk2Size

                dstWavFile.write(fileData);

                while(!tmpFile.atEnd())
                {
                    QByteArray data = tmpFile.read(64 * 1024);
                    dstWavFile.write(data);
                }

                dstWavFile.close();
                tmpFile.close();
                tmpFile.remove();
            }
        }

        startDecoding();
    }
}

void SdContentModel::slDecodingError(QAudioDecoder::Error error)
{
    emit errorOccured(decoder.errorString());
}

void SdContentModel::addMidi(QString dstPath, QList<QUrl> filesPathList)
{
    foreach(QUrl fileUrl, filesPathList)
    {
        QString filePath = fileUrl.toLocalFile();
        QFileInfo fileInfo(filePath);
        QFile::copy(filePath, dstPath + "/" + fileInfo.fileName());
    }
}

void SdContentModel::deleteObject(QModelIndex index)
{
    QModelIndex realIndex = m_sdProxyModel.mapToSource(index);
    remove(realIndex);
}
