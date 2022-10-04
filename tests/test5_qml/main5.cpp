#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <UtilsQt/qml.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main5.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    UtilsQt::Qml::registerAll(engine);

    engine.load(url);

    QTimer::singleShot(150, &app, &QGuiApplication::quit);

    return app.exec();
}
