#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <QString>
#include <QMetaEnum>

struct WavFormat
{
    quint8  channels;
    quint16 sampleRate;
    quint8  bitsPerSample;
    quint32 maxFileSize;
};

#if defined(WAVCONVERTERLIB_LIBRARY)
#  define WAVCONVERTERLIB_LIBRARY_EXPORT Q_DECL_EXPORT
#else
#  define WAVCONVERTERLIB_LIBRARY_EXPORT Q_DECL_IMPORT
#endif

class WAVCONVERTERLIB_LIBRARY_EXPORT Resampler
{
    Q_GADGET
public:
    Resampler();

    void setOutputFormat(const WavFormat& wavFormat);
    bool convertFile(QString inputFilePath, QString outputFilePath="");
    const WavFormat &wavOutputFormat() const;

    // copy from header
    enum soxRtnCodes{
        SOX_SUCCESS = 0,     /**< Function succeeded = 0 */
        SOX_EOF = -1,        /**< End Of File or other error = -1 */
        SOX_EHDR = 2000,     /**< Invalid Audio Header = 2000 */
        SOX_EFMT,            /**< Unsupported data format = 2001 */
        SOX_ENOMEM,          /**< Can't alloc memory = 2002 */
        SOX_EPERM,           /**< Operation not permitted = 2003 */
        SOX_ENOTSUP,         /**< Operation not supported = 2004 */
        SOX_EINVAL           /**< Invalid argument = 2005 */
    };
    Q_ENUM(soxRtnCodes)

private:
    WavFormat m_wavOutputFormat;
    QMetaEnum enumResultSox;

};
#endif // RESAMPLER_H
