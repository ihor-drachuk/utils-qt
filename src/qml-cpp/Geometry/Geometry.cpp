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


QPolygonF Geometry::polygonScale(const QPolygonF& polygon, qreal factor) const
{
    auto result = polygon;
    polygonScaleRef(result, factor);
    return result;
}

QVector<QPolygonF> Geometry::polygonsScale(const QVector<QPolygonF>& polygons, qreal factor) const
{
    auto result = polygons;
    polygonsScaleRef(result, factor);
    return result;
}

void Geometry::polygonScaleRef(QPolygonF& polygon, qreal factor) const
{
    for (auto& x : polygon)
        x *= factor;
}

void Geometry::polygonsScaleRef(QVector<QPolygonF>& polygons, qreal factor) const
{
    for (auto& polygon : polygons)
        for (auto& x : polygon)
            x *= factor;
}

Geometry::Geometry()
{
    createImpl();
}

Geometry::~Geometry()
{
}
