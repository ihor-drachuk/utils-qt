#include <utils-qt/qml-cpp/Geometry/Geometry.h>

#include <QQmlEngine>

struct Geometry::impl_t
{
};

Geometry& Geometry::instance()
{
    static Geometry object;
    return object;
}

void Geometry::registerTypes()
{
    qmlRegisterSingletonType<Geometry>("UtilsQt", 1, 0, "Geometry", [] (QQmlEngine *engine, QJSEngine *) -> QObject* {
        auto ret = &Geometry::instance();
        engine->setObjectOwnership(ret, QQmlEngine::CppOwnership);
        return ret;
    });
}

Geometry::Geometry()
{
    createImpl();
}

Geometry::~Geometry()
{
}
