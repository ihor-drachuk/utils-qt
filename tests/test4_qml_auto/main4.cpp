#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>
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
    }
} _reg;

QUICK_TEST_MAIN(test_utils_qt_4)

#include "main4.moc"
