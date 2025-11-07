#ifndef WAVHEADER_H
#define WAVHEADER_H

#include <QObject>
#include <QString>
#include <QByteArray>

//http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
//http://microsin.ru/content/view/1197/44/

struct idRiff
{
    const QString id = "RIFF";
    quint32 fileSize   = 0;     // realFileSize - 8
};

struct idFmt
{
    const QString id = "fmt ";
    const quint32 formatInfoSize = 16; // + extra format byte
    const quint16 compressionCode = 1;
    quint16 numberOfChannels;
    quint32 sampleRate;
    quint32 aveBitPerSec;
    quint16 blockAlign;
    quint16 significantBitPerSample;
    quint16 extraFormatByte; // absent if compressionCode = 1;
    QByteArray *ExtData;     // absent if compressionCode = 1;
};

struct idData
{
    const QString id = "data";
    quint32 audioSamplesLength;
    QByteArray *audioSamples;
};

struct idWave
{
    const QString id = "WAVE";
    idFmt fmt;
    idData data;
};

struct WavFile
{
    idRiff riff;
    idWave wave;
};


class WavUtils : public QObject
{
    Q_OBJECT
public:
    explicit WavUtils(QObject *parent = nullptr);
    static QByteArray getHeader(quint16 numberOfChannels, quint32 sampleRate, quint16 bitPerSample, quint32 sampleLength);

private:
    template<typename T> static QByteArray IntToBa (T val);


signals:

public slots:
};

#endif // WAVHEADER_H
