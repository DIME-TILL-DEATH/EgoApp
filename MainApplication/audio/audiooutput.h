#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H
#include <QAudio>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <functional>
class AudioOutput
{
public:
    using StateChangedHandler = std::function<void(QtAudio::State)>;
    virtual ~AudioOutput() = default;
    virtual void setStateChangedHandler(StateChangedHandler handler) = 0;
    virtual QtAudio::State state() const = 0;
    virtual QAudio::Error error() const = 0;
    virtual void start(QIODevice *source) = 0;
    virtual void stop() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
};
class QAudioSinkOutput final : public AudioOutput
{
public:
    explicit QAudioSinkOutput(const QAudioFormat &format);
    void setStateChangedHandler(StateChangedHandler handler) override;
    QtAudio::State state() const override;
    QAudio::Error error() const override;
    void start(QIODevice *source) override;
    void stop() override;
    void suspend() override;
    void resume() override;
private:
    QAudioSink m_sink;
    StateChangedHandler m_stateChangedHandler;
};
#endif // AUDIOOUTPUT_H
