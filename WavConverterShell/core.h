#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QVariant>
#include <QDir>
#include <QFileInfo>

#include "resampler.h"

class Core : public QObject
{
    Q_OBJECT
public:
    explicit Core(QObject *parent = nullptr);

    Q_INVOKABLE void setOutputFormat(QVariant sampleRate, QVariant bitsDepth, QVariant channels, QVariant maxFileSize);
    Q_INVOKABLE void setPrefixSuffix(QString prefix, QString suffix);
    Q_INVOKABLE bool setDestinationPath(QString newDestinationPath);
    Q_INVOKABLE void startConvert(QStringList fileList);


signals:
    void sgResamplingFinished(QString fileName);
    void sgOperationFinished();
    void sgError(QString description);

private:
    QString m_prefix;
    QString m_suffix;
    QDir m_destinationDir;

    WavFormat m_outputWavFormat;

    void convertFiles(QStringList inputFiles);
    QString constructOutputPath(const QString& inputFilePath);
};

#endif // CORE_H
