#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <utils-qt/qml/qml-controls.h>
#include <utils-qt/qml-cpp/ListModelItemProxy.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main3.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    QmlControls::init(engine);
    ListModelItemProxy::registerTypes("CppTypes");

    engine.load(url);

    return app.exec();
}
