#include <QDebug>
#include <QStandardPaths>

#include <QtConcurrent/QtConcurrent>

#include "core.h"

Core::Core(QObject *parent)
    : QObject{parent}
{

    QString folderPath = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0)+"/AMT/pangaeaCPPA/convertedIR";

    if(!QDir(folderPath).exists())
    {
        QDir().mkpath(folderPath);
    }

}

void Core::setOutputFormat(QVariant sampleRate, QVariant bitsDepth, QVariant channels, QVariant maxFileSize)
{
    m_outputWavFormat.sampleRate = sampleRate.toUInt();
    m_outputWavFormat.bitsPerSample = bitsDepth.toUInt();
    m_outputWavFormat.channels = channels.toUInt();
    m_outputWavFormat.maxFileSize = maxFileSize.toUInt();

    //resampler.setOutputFormat(m_outputWavFormat);

    qDebug() << "Output wav format: "   << m_outputWavFormat.sampleRate
                                        << m_outputWavFormat.bitsPerSample
                                        << m_outputWavFormat.channels
                                        << m_outputWavFormat.maxFileSize;
}

void Core::setPrefixSuffix(QString prefix, QString suffix)
{
    m_prefix = prefix;
    m_suffix = suffix;

    qDebug() << "Prefix:" << m_prefix << "Suffix:" << m_suffix;
}

bool Core::setDestinationPath(QString newDestinationPath)
{
    QDir destinationDir(newDestinationPath);

    if(destinationDir.exists())
    {
        m_destinationDir = destinationDir;
        return true;
    }
    else
    {
        emit sgError(QObject::tr("Destination directory doesn't exists!"));
        return false;
    }
}

void Core::startConvert(QStringList fileList)
{
//        QtConcurrent::run(this, &Core::convertFiles, fileList);
    QtConcurrent::run(&Core::convertFiles, this, fileList);
}

void Core::convertFiles(QStringList inputFiles)
{
    Resampler resampler;
    resampler.setOutputFormat(m_outputWavFormat);

    for(QString& file : inputFiles)
    {
        QString destinationPath = constructOutputPath(file);
        bool result = resampler.convertFile(file, destinationPath);

        if(result) emit sgResamplingFinished(file);
    }

    emit sgOperationFinished();
}

QString Core::constructOutputPath(const QString &inputFilePath)
{
    QFileInfo fileInfo(inputFilePath);
    QString fileName = fileInfo.fileName();
    QString fileSuffix = fileInfo.suffix();
    int suffixPosition = fileName.lastIndexOf(fileSuffix);
    if(suffixPosition > 0)
    {
        fileName.truncate(suffixPosition-1); // "." also
    }
    fileName = m_prefix + fileName + m_suffix + "." + fileSuffix;
    return m_destinationDir.absolutePath() + "/" + fileName;
}

