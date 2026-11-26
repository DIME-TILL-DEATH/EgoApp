#include "audiooutput.h"
#include <QMediaDevices>
#include <utility>
QAudioSinkOutput::QAudioSinkOutput(const QAudioFormat &format)
    : m_sink(QMediaDevices::defaultAudioOutput(), format)
{
    QObject::connect(&m_sink, &QAudioSink::stateChanged,
                     [this](QtAudio::State state) {
        if (m_stateChangedHandler)
            m_stateChangedHandler(state);
    });
}
void QAudioSinkOutput::setStateChangedHandler(StateChangedHandler handler)
{
    m_stateChangedHandler = std::move(handler);
}
QtAudio::State QAudioSinkOutput::state() const { return m_sink.state(); }
QAudio::Error QAudioSinkOutput::error() const { return m_sink.error(); }
void QAudioSinkOutput::start(QIODevice *source) { m_sink.start(source); }
void QAudioSinkOutput::stop() { m_sink.stop(); }
void QAudioSinkOutput::suspend() { m_sink.suspend(); }
void QAudioSinkOutput::resume() { m_sink.resume(); }
