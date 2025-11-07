#include "wavutils.h"
#include <QDataStream>
#include <QIODevice>

WavUtils::WavUtils(QObject *parent) : QObject(parent)
{
}

QByteArray WavUtils::getHeader(quint16 numberOfChannels, quint32 sampleRate, quint16 bitPerSample, quint32 sampleLength)
{
    QByteArray retBa;
    WavFile wavFile;

    wavFile.riff.fileSize = sampleLength+36;
    if(wavFile.riff.fileSize&1)
        wavFile.riff.fileSize++;

    wavFile.wave.fmt.numberOfChannels = numberOfChannels;
    wavFile.wave.fmt.sampleRate = sampleRate;
    wavFile.wave.fmt.significantBitPerSample = bitPerSample;
    wavFile.wave.fmt.blockAlign = wavFile.wave.fmt.significantBitPerSample/8 * wavFile.wave.fmt.numberOfChannels;
    wavFile.wave.fmt.aveBitPerSec = wavFile.wave.fmt.sampleRate * wavFile.wave.fmt.blockAlign;


    retBa.append(wavFile.riff.id.toUtf8());
    retBa.append(IntToBa(wavFile.riff.fileSize));

    retBa.append(wavFile.wave.id.toUtf8());

    retBa.append(wavFile.wave.fmt.id.toUtf8());
    retBa.append(IntToBa(wavFile.wave.fmt.formatInfoSize));
    retBa.append(IntToBa(wavFile.wave.fmt.compressionCode));
    retBa.append(IntToBa(wavFile.wave.fmt.numberOfChannels));
    retBa.append(IntToBa(wavFile.wave.fmt.sampleRate));

    retBa.append(IntToBa(wavFile.wave.fmt.aveBitPerSec));
    retBa.append(IntToBa(wavFile.wave.fmt.blockAlign));

    retBa.append(IntToBa(wavFile.wave.fmt.significantBitPerSample));

    retBa.append("data");
    retBa.append(IntToBa(sampleLength));

    return retBa;
}

template<typename T>
QByteArray WavUtils::IntToBa (T val)
{
    QByteArray retBa;
    QDataStream out(&retBa, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out << val;
    return retBa;
}
