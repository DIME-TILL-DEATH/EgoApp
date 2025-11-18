#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QtMath>

#include "sox.h"
#include "resampler.h"

#include "wavutils.h"

Resampler::Resampler()
{
    enumResultSox = QMetaEnum::fromType<soxRtnCodes>();

    m_wavOutputFormat.channels = 2;
    m_wavOutputFormat.sampleRate = 44100;
    m_wavOutputFormat.bitsPerSample = 16;
    m_wavOutputFormat.maxFileSize = 0;
}


bool Resampler::convertFile(QString inputFilePath, QString outputFilePath)
{
    QFileInfo inFileInfo(inputFilePath);

    if(outputFilePath=="")
    {
        QString folderPath = inFileInfo.absoluteDir().path() + "/converted/";
        if(!QDir(folderPath).exists())
        {
            QDir().mkpath(folderPath);
        }
        outputFilePath = folderPath + inFileInfo.fileName();
    }

    qDebug() << "Resampler. Input file path:" << inputFilePath;
    qDebug() << "Resampler. Output file path:" << outputFilePath;

    QString outTempFile;
    outTempFile.append(QDir::tempPath());
    outTempFile.append("/");
    outTempFile.append(inFileInfo.fileName() + "_ir_tmp.wav"); //"WavConvertor_tmp.wav");
    qDebug() << "temporally file:" << outTempFile;

    static sox_format_t * in, * out; /* input and output files */
    sox_signalinfo_t soxSignalInfo;
    sox_effects_chain_t * soxChain;
    sox_effect_t * soxEffect;

    sox_init();
    in = sox_open_read(inputFilePath.toLocal8Bit().constData(), nullptr, nullptr, nullptr);
    if(in==nullptr)
    {
        sox_quit();
        qDebug() << "SOX cannot open input file:" << inputFilePath;
        return false;
    }
    soxSignalInfo = in->signal;

    out = (sox_format_t *) malloc(sizeof (sox_format_t));
    memcpy(out, in, sizeof (sox_format_t));

    out->encoding.encoding = SOX_ENCODING_SIGN2;
    out->encoding.bits_per_sample = m_wavOutputFormat.bitsPerSample;
    out->signal.rate = m_wavOutputFormat.sampleRate;
    out->signal.channels = m_wavOutputFormat.channels;
    out->signal.length = SOX_UNSPEC;

    out = sox_open_write(outTempFile.toLocal8Bit().constData(), &out->signal, &out->encoding, nullptr, nullptr, nullptr);

    soxChain = sox_create_effects_chain(&in->encoding, &out->encoding);

    char* args[10];
    int soxResult;
    soxEffect = sox_create_effect(sox_find_effect("input"));
    args[0] = (char *)in;
    sox_effect_options(soxEffect, 1, args);
    soxResult = sox_add_effect(soxChain, soxEffect, &soxSignalInfo, &in->signal);
    qDebug() << "SOX 'input' effect result:" << enumResultSox.valueToKey(soxResult);
    free(soxEffect);

    if (qFabs(in->signal.rate - out->signal.rate) > static_cast<qreal>(1))
    {
        soxEffect = sox_create_effect(sox_find_effect("rate"));
        sox_effect_options(soxEffect, 0, nullptr);
        soxResult = sox_add_effect(soxChain, soxEffect, &soxSignalInfo, &out->signal);
        qDebug()<< "SOX 'rate' result:" << enumResultSox.valueToKey(soxResult);
        free(soxEffect);
    }

    if (in->signal.channels != out->signal.channels)
    {
        soxEffect = sox_create_effect(sox_find_effect("channels"));
        sox_effect_options(soxEffect, 0, nullptr);
        soxResult = sox_add_effect(soxChain, soxEffect, &soxSignalInfo, &out->signal);
        qDebug()<< "SOX 'channels' result:" << enumResultSox.valueToKey(soxResult);
        free(soxEffect);
    }

    soxEffect = sox_create_effect(sox_find_effect("output"));
    args[0] = (char *)out;
    sox_effect_options(soxEffect, 1, args);
    soxResult = sox_add_effect(soxChain, soxEffect, &soxSignalInfo, &out->signal);
    qDebug() << "SOX 'output' result:" << enumResultSox.valueToKey(soxResult);
    free(soxEffect);

    sox_flow_effects(soxChain, nullptr, nullptr);

    sox_delete_effects_chain(soxChain);
    sox_close(out);
    sox_close(in);
    sox_quit();

    QFile file(outTempFile);
    if (file.open(QIODevice::ReadWrite))
    {
        QByteArray fileData;
        QByteArray headerData  = "data";

        fileData = file.read(1024);
        int headerDataPos = fileData.indexOf(headerData);
        if (headerDataPos>0)
        {
            QByteArray header;
            QByteArray data;
            file.seek(headerDataPos + 4 + 4);

            if(m_wavOutputFormat.maxFileSize > 44)
                data = file.read(m_wavOutputFormat.maxFileSize-44);
            else
                data = file.readAll();

            qDebug() << "Result data size:" << data.size() << "Max data size:" << m_wavOutputFormat.maxFileSize;

            header = WavUtils::getHeader(m_wavOutputFormat.channels, m_wavOutputFormat.sampleRate, m_wavOutputFormat.bitsPerSample, data.size());

            QFile sFile(outputFilePath);
            if (sFile.open(QIODevice::WriteOnly))
            {
                sFile.write(header);
                sFile.write(data);
                sFile.close();
            }
            else
            {
                qWarning() << "Can't open/create output file!";
            }
        }
        file.close();
    }
    QDir().remove(outTempFile);
    return true;
}

void Resampler::setOutputFormat(const WavFormat &wavFormat)
{
    m_wavOutputFormat = wavFormat;
}

const WavFormat &Resampler::wavOutputFormat() const
{
    return m_wavOutputFormat;
}
