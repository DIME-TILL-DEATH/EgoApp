#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "uicore.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);    app.setOrganizationName("AMT");
    app.setOrganizationDomain("amtelectronics.com");
    app.setApplicationName("EgoGig");

    UiCore uiCore;

    QQmlApplicationEngine engine;

    qmlRegisterSingletonInstance("CppObjects", 1, 0, "UiCore", &uiCore);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("EgoApp", "Main");

    return app.exec();
}
