#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);    app.setOrganizationName("AMT");

    app.setOrganizationName("AMT electronics");
    app.setOrganizationDomain("amtelectronics.com");
    app.setApplicationName("EgoGig");

    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("EgoApp", "Main");

    return app.exec();
}
