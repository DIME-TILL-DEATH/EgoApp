#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QQmlContext>

#include <QQuickStyle>

#include "core.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);

    app.setOrganizationName("AMT");
    app.setApplicationName("IR convertor");

    QQmlApplicationEngine engine;

    Core core;
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("_core", &core);

#ifdef Q_OS_MACOS
    QQuickStyle::setStyle("Basic");
#endif

    engine.addImportPath(":/");

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
