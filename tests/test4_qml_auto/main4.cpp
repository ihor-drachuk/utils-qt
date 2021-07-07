#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <cassert>
#include <utils-qt/qml.h>


// 'Registrator' is created for compatibility with Qt 5.9

class Registrator : public QObject
{
    Q_OBJECT
public:
    static void registerTypes(const char* url)
    {
        qmlRegisterType<Registrator>(url, 1, 0, "Registrator");
    }

    explicit Registrator(QObject* parent = nullptr)
        : QObject(parent)
    { }

    Q_INVOKABLE void registerAll()
    {
        auto context = QQmlEngine::contextForObject(this);
        assert(context);

        auto engine = QQmlEngine::contextForObject(this)->engine();
        assert(engine);

        UtilsQt::Qml::registerAll(*engine);
    }
};

struct RegistratorKickstart
{
    RegistratorKickstart() {
        Registrator::registerTypes("Registrator");
#if QTCORE_VERSION >= QT_VERSION_CHECK(5,14,0)
        QQuickWindow::setSceneGraphBackend(QSGRendererInterface::NullRhi);
#endif
    }
} _reg;

QUICK_TEST_MAIN(utils_qt_test4)

#include "main4.moc"
