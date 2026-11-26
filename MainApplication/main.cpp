#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QString>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName("AMT electronics");
    app.setOrganizationDomain("amtelectronics.com");
    app.setApplicationName("EgoGig");

    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;
    // QML is compiled into the EgoApp module's Qt resource collection.
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("EgoApp", "Main");

    return app.exec();
}
