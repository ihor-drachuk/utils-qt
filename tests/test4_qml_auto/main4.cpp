#include <QtQuickTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <cassert>
#include <UtilsQt/qml.h>


// 'Registrator' is created for compatibility with Qt 5.9

class Registrator : public QObject
{
    Q_OBJECT
public:
    static void registerTypes(const char* url)
    {
        qmlRegisterType<Registrator>(url, 1, 0, "Registrator");
        qputenv("QML2_IMPORT_PATH", UTILS_QT_ROOT_DIR"/src/qml");
    }

    explicit Registrator(QObject* parent = nullptr)
        : QObject(parent)
    { }

    Q_INVOKABLE void registerAll()
    {
        auto context = QQmlEngine::contextForObject(this);
        assert(context);

        auto engine = context->engine();
        assert(engine);

        UtilsQt::Qml::registerAll(*engine);
    }
};

static void Registrator_register()
{
    Registrator::registerTypes("Registrator");
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::NullRhi);
#elif QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::NullRhi);
#endif
}

Q_COREAPP_STARTUP_FUNCTION(Registrator_register)

QUICK_TEST_MAIN(utils_qt_test4)

#include "main4.moc"
